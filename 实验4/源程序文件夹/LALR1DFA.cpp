#include "LALR1DFA.h"
#include <fstream>
#include <iostream>

LALR1DFA::LALR1DFA(const Grammar& g, const LR0DFA& lr0)
    : grammar_(&g), lr0_(&lr0), built_(false) {}

const std::set<int>& LALR1DFA::getLookahead(int state, int rule, int pos) const {
    static std::set<int> emptySet;
    auto it = lookaheads_[state].find({rule, pos});
    if(it != lookaheads_[state].end())
        return it->second;
    return emptySet;
}

// ── DeRemer-Pennello 先行符号传播 ──
void LALR1DFA::build() {
    int n = (int)lr0_->getStates().size();
    const auto& prods = grammar_->getProductions();

    // ① 初始化 lookahead 集合
    lookaheads_.resize(n);
    for(int si = 0; si < n; si++) {
        for(auto& item : lr0_->getStates()[si]) {
            lookaheads_[si][{item.rule, item.pos}] = {};
        }
    }

    // ② 播种：S' → ·S$ → lookahead = {$}
    int startRule = (int)prods.size() - 1;
    lookaheads_[0][{startRule, 0}].insert(MaxSetSize);

    // ③ 迭代传播
    bool changed;
    do {
        changed = false;

        for(int si = 0; si < n; si++) {
            for(auto& item : lr0_->getStates()[si]) {
                int r = item.rule, p = item.pos;
                auto& curLA = lookaheads_[si][{r, p}];
                if(curLA.empty()) continue;

                int len = prods[r].length;
                if(p >= len) continue;  // 归约项，不传播

                int X = prods[r].right[p];  // ·后符号

                // ③a GoTo 传播
                for(auto& tr : lr0_->getTransitions()[si]) {
                    if(tr.first != X) continue;
                    int nextState = tr.second;
                    ItemKey nextKey = {r, p + 1};
                    if(lookaheads_[nextState].count(nextKey)) {
                        for(int a : curLA) {
                            if(lookaheads_[nextState][nextKey].insert(a).second)
                                changed = true;
                        }
                    }
                    break;
                }

                // ③b 闭包传播（·后是非终结符时）
                if(X >= Gap) continue;

                // 计算 First(β)
                std::set<int> firstOfBeta;
                bool betaNullable = true;
                for(int k = p + 1; k < len; k++) {
                    int sym = prods[r].right[k];
                    std::set<int> fk = grammar_->getFirst(sym);
                    for(int x : fk)
                        if(x != 100) firstOfBeta.insert(x);
                    if(fk.find(100) == fk.end()) {
                        betaNullable = false;
                        break;
                    }
                }

                // 遍历 X 的所有产生式
                for(int pi = 0; pi < (int)prods.size(); pi++) {
                    if(prods[pi].left != X) continue;
                    ItemKey ck = {pi, 0};
                    if(!lookaheads_[si].count(ck)) continue;

                    // 自发生成
                    for(int a : firstOfBeta) {
                        if(lookaheads_[si][ck].insert(a).second)
                            changed = true;
                    }
                    // 传播：β 可空时复制当前 lookahead
                    if(betaNullable) {
                        for(int a : curLA) {
                            if(lookaheads_[si][ck].insert(a).second)
                                changed = true;
                        }
                    }
                }
            }
        }
    } while(changed);

    built_ = true;
}

// ── 冲突检测 ──
bool LALR1DFA::isLALR1() const {
    if(!built_) return false;
    const auto& prods = grammar_->getProductions();
    int startRule = (int)prods.size() - 1;
    int n = (int)lr0_->getStates().size();

    bool isLALR = true;
    for(int si = 0; si < n; si++) {
        std::map<int,bool> hasShift;
        std::map<int,int> reduceCnt;

        for(auto& item : lr0_->getStates()[si]) {
            int r = item.rule, p = item.pos;
            if(p < (int)prods[r].length) {
                int X = prods[r].right[p];
                if(X >= Gap) hasShift[X] = true;
            } else {
                auto& las = lookaheads_[si].at({r, p});
                for(int a : las) {
                    if(a == MaxSetSize && r == startRule) continue;
                    reduceCnt[a]++;
                }
            }
        }

        for(auto it = hasShift.begin(); it != hasShift.end(); ++it) {
            int sym = it->first;
            if(reduceCnt[sym] > 0) {
                std::cout << "  SR conflict in State " << si
                          << " on symbol " << grammar_->getSignSet()[sym] << std::endl;
                isLALR = false;
            }
        }
        for(auto it = reduceCnt.begin(); it != reduceCnt.end(); ++it) {
            int a = it->first;
            int cnt = it->second;
            if(cnt > 1) {
                std::cout << "  RR conflict in State " << si
                          << " on symbol ";
                if(a == 100) std::cout << "#";
                else if(a == MaxSetSize) std::cout << "$";
                else std::cout << grammar_->getSignSet()[a];
                std::cout << std::endl;
                isLALR = false;
            }
        }
    }
    return isLALR;
}

// ── 输出到控制台 ──
void LALR1DFA::print(std::ostream& out) const {
    const auto& signs = grammar_->getSignSet();

    out << "\n========== LALR(1) DFA =========" << std::endl;
    out << lr0_->getStates().size() << " states (same core as LR(0))" << std::endl << std::endl;

    for(int si = 0; si < (int)lr0_->getStates().size(); si++) {
        out << "--- State " << si << " ---" << std::endl;
        for(auto& item : lr0_->getStates()[si]) {
            // 核心项目文本
            int r = item.rule, p = item.pos;
            int len = grammar_->getProductions()[r].length;
            out << "  " << signs[grammar_->getProductions()[r].left] << " ->";
            for(int k = 0; k < len; k++) {
                if(k == p) out << " .";
                out << " " << signs[grammar_->getProductions()[r].right[k]];
            }
            if(p == len) out << " .";

            // lookahead 集合
            auto& las = lookaheads_[si].at({r, p});
            if(!las.empty()) {
                out << "  [";
                bool first = true;
                for(int a : las) {
                    if(!first) out << " ";
                    first = false;
                    if(a == 100) out << "#";
                    else if(a == MaxSetSize) out << "$";
                    else out << signs[a];
                }
                out << "]";
            }
            out << "  (Rule " << item.rule << ")" << std::endl;
        }
        for(auto& tr : lr0_->getTransitions()[si]) {
            out << "  --" << signs[tr.first] << "--> State " << tr.second << std::endl;
        }
        if(!lr0_->getTransitions()[si].empty()) out << std::endl;
    }
    out << "================================" << std::endl;

    if(isLALR1())
        out << "\nThis grammar is LALR(1)." << std::endl;
    else
        out << "\nThis grammar is NOT LALR(1)." << std::endl;
}

// ── 生成 DOT 字符串 ──
std::string LALR1DFA::toDotString() const {
    const auto& signs = grammar_->getSignSet();
    const auto& prods = grammar_->getProductions();

    std::ostringstream dot;
    dot << "digraph LALR1_DFA {" << std::endl;
    dot << "    rankdir=LR;" << std::endl;
    dot << "    node [shape=box, style=rounded, fontname=\"Consolas\"];" << std::endl;

    for(int si = 0; si < (int)lr0_->getStates().size(); si++) {
        dot << "    " << si << " [label=\"State " << si;
        for(auto& item : lr0_->getStates()[si]) {
            int r = item.rule, p = item.pos;
            int len = prods[r].length;
            dot << "\\n" << signs[prods[r].left] << " ->";
            for(int k = 0; k < len; k++) {
                if(k == p) dot << " .";
                dot << " " << signs[prods[r].right[k]];
            }
            if(p == len) dot << " .";

            auto& las = lookaheads_[si].at({r, p});
            if(!las.empty()) {
                dot << "  [";
                bool first = true;
                for(int a : las) {
                    if(!first) dot << " ";
                    first = false;
                    if(a == 100) dot << "#";
                    else if(a == MaxSetSize) dot << "$";
                    else dot << signs[a];
                }
                dot << "]";
            }
        }
        dot << "\"];" << std::endl;
    }

    for(int si = 0; si < (int)lr0_->getStates().size(); si++)
        for(auto& tr : lr0_->getTransitions()[si])
            dot << "    " << si << " -> " << tr.second
                << " [label=\"" << signs[tr.first] << "\"];" << std::endl;

    dot << "}" << std::endl;
    return dot.str();
}

// ── 保存 DOT 图 ──
void LALR1DFA::saveDot(const std::string& filename) const {
    std::ofstream dotFile(filename);
    dotFile << toDotString();
    dotFile.close();
    std::cout << "LALR(1) DFA saved to " << filename << std::endl;
}
