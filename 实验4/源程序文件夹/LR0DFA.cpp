#include "LR0DFA.h"
#include <fstream>
#include <map>

LR0DFA::LR0DFA(const Grammar& g) : DFA(g) {}

// LR(0) 闭包：·后为非终结符时展开，k=0（无 preview）
LRItemSet LR0DFA::closure(LRItemSet I) {
    LRItemSet result = I;
    bool changed = true;
    while(changed) {
        changed = false;
        auto snapshot = result;
        for(auto item : snapshot) {
            int r = item.rule, p = item.pos;
            if(p >= (int)grammar_->getProductions()[r].length) continue;
            int B = grammar_->getProductions()[r].right[p];
            if(B >= Gap) continue;  // 终结符不展开

            // 展开所有 B → ·γ
            const auto& prods = grammar_->getProductions();
            for(int pi = 0; pi < (int)prods.size(); pi++) {
                if(prods[pi].left == B) {
                    LRItem newItem;
                    newItem.rule = pi;
                    newItem.pos  = 0;
                    newItem.k    = 0;
                    if(result.find(newItem) == result.end()) {
                        result.insert(newItem);
                        changed = true;
                    }
                }
            }
        }
    }
    return result;
}

// LR(0) GoTo：·前移过 X 后取闭包
LRItemSet LR0DFA::goTo(LRItemSet I, int X) {
    LRItemSet J;
    const auto& prods = grammar_->getProductions();
    for(auto item : I) {
        int r = item.rule, p = item.pos;
        if(p < (int)prods[r].length && prods[r].right[p] == X) {
            LRItem newItem;
            newItem.rule = r;
            newItem.pos  = p + 1;
            newItem.k    = 0;
            J.insert(newItem);
        }
    }
    return closure(J);
}

// LR(0) 起始项：k=0
LRItem LR0DFA::createStartItem(int augmentedRuleIndex) const {
    LRItem item;
    item.rule = augmentedRuleIndex;
    item.pos  = 0;
    item.k    = 0;
    return item;
}

// LR(0) 冲突检测
bool LR0DFA::isLR0() const {
    const auto& prods = grammar_->getProductions();
    for(auto& state : states_) {
        bool hasShift = false;
        bool hasReduce = false;
        int reduceCnt = 0;

        for(auto item : state) {
            int r = item.rule, p = item.pos;
            if(p == (int)prods[r].length) {
                hasReduce = true;
                reduceCnt++;
            } else {
                int X = prods[r].right[p];
                if(X >= Gap)
                    hasShift = true;
            }
        }

        if(hasShift && hasReduce) return false;
        if(reduceCnt > 1) return false;
    }
    return true;
}

// SLR(1) 冲突检测（基于 FOLLOW 集，收集所有冲突）
bool LR0DFA::isSLR1(std::string& conflicts) const {
    const auto& prods = grammar_->getProductions();
    bool hasConflict = false;

    for(auto& state : states_) {
        std::map<int, int> shiftSyms;
        std::vector<int> reduceRules;

        for(auto item : state) {
            int r = item.rule, p = item.pos;
            if(p == (int)prods[r].length) {
                reduceRules.push_back(r);
            } else {
                int X = prods[r].right[p];
                if(X >= Gap)
                    shiftSyms[X] = 1;
            }
        }

        int stateIdx = (int)(&state - &states_[0]);

        // 检查 SR 冲突
        for(auto& sr : shiftSyms) {
            int sym = sr.first;
            for(int r : reduceRules) {
                auto followSet = grammar_->getFollow(prods[r].left);
                if(followSet.find(sym) != followSet.end()) {
                    conflicts += "State " + std::to_string(stateIdx)
                        + ": SR冲突 — 移进 \"" + grammar_->getSignSet()[sym]
                        + "\"，但该符号在 FOLLOW(" + grammar_->getSignSet()[prods[r].left] + ") 中\n";
                    hasConflict = true;
                }
            }
        }

        // 检查 RR 冲突
        for(size_t i = 0; i < reduceRules.size(); i++) {
            for(size_t j = i + 1; j < reduceRules.size(); j++) {
                auto f1 = grammar_->getFollow(prods[reduceRules[i]].left);
                auto f2 = grammar_->getFollow(prods[reduceRules[j]].left);
                for(int sym : f1) {
                    if(f2.find(sym) != f2.end()) {
                        std::string symName = (sym == MaxSetSize) ? "$"
                            : (sym == 100 ? "#" : grammar_->getSignSet()[sym]);
                        conflicts += "State " + std::to_string(stateIdx)
                            + ": RR冲突 — " + grammar_->getSignSet()[prods[reduceRules[i]].left]
                            + " 和 " + grammar_->getSignSet()[prods[reduceRules[j]].left]
                            + " 的 FOLLOW 集有交集 \"" + symName + "\"\n";
                        hasConflict = true;
                    }
                }
            }
        }
    }

    if(!hasConflict)
        conflicts = "无冲突";
    return !hasConflict;
}

// SLR(1) 冲突检测（基于 FOLLOW 集）
bool LR0DFA::isSLR1() const {
    const auto& prods = grammar_->getProductions();

    for(auto& state : states_) {
        std::map<int, int> shiftSyms;     // 移进符号 → 计数
        std::vector<int> reduceRules;     // 可归约的产生式

        for(auto item : state) {
            int r = item.rule, p = item.pos;
            if(p == (int)prods[r].length) {
                reduceRules.push_back(r);
            } else {
                int X = prods[r].right[p];
                if(X >= Gap)
                    shiftSyms[X] = 1;
            }
        }

        // 检查 SR 冲突：移进符号是否在归约项的 FOLLOW 集中
        for(auto& sr : shiftSyms) {
            int sym = sr.first;
            for(int r : reduceRules) {
                auto followSet = grammar_->getFollow(prods[r].left);
                if(followSet.find(sym) != followSet.end()) {
                    std::cout << "  SLR(1) SR conflict in state: shift "
                              << grammar_->getSignSet()[sym]
                              << " in FOLLOW(" << grammar_->getSignSet()[prods[r].left] << ")" << std::endl;
                    return false;
                }
            }
        }

        // 检查 RR 冲突：归约项的 FOLLOW 集是否有交集
        for(size_t i = 0; i < reduceRules.size(); i++) {
            for(size_t j = i + 1; j < reduceRules.size(); j++) {
                auto f1 = grammar_->getFollow(prods[reduceRules[i]].left);
                auto f2 = grammar_->getFollow(prods[reduceRules[j]].left);
                for(int sym : f1) {
                    if(f2.find(sym) != f2.end()) {
                        std::cout << "  SLR(1) RR conflict in state: "
                                  << grammar_->getSignSet()[prods[reduceRules[i]].left]
                                  << " and " << grammar_->getSignSet()[prods[reduceRules[j]].left]
                                  << " both have " << (sym == MaxSetSize ? "$" : grammar_->getSignSet()[sym]) << std::endl;
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

// 输出到控制台
void LR0DFA::print(std::ostream& out) const {
    const auto& signs = grammar_->getSignSet();

    out << "\n========== LR(0) DFA ==========" << std::endl;
    out << states_.size() << " states, " << edgeCount_ << " edges" << std::endl << std::endl;

    for(int si = 0; si < (int)states_.size(); si++) {
        out << "--- State " << si << " ---" << std::endl;
        for(auto item : states_[si]) {
            out << "  " << formatItemCore(*grammar_, item);
            out << "  (Rule " << item.rule << ")" << std::endl;
        }
        for(auto tr : transitions_[si]) {
            out << "  --" << signs[tr.first] << "--> State " << tr.second << std::endl;
        }
        if(!transitions_[si].empty()) out << std::endl;
    }
    out << "================================" << std::endl;

    if(isLR0())
        out << "This grammar is LR(0)." << std::endl;
    else
        out << "This grammar is NOT LR(0)." << std::endl;

    if(isSLR1())
        out << "This grammar is SLR(1)." << std::endl;
    else
        out << "This grammar is NOT SLR(1)." << std::endl;
}

// 生成 DOT 字符串
std::string LR0DFA::toDotString() const {
    const auto& signs = grammar_->getSignSet();
    const auto& prods = grammar_->getProductions();
    std::ostringstream dot;
    dot << "digraph LR0_DFA {" << std::endl;
    dot << "    rankdir=LR;" << std::endl;
    dot << "    node [shape=box, style=rounded, fontname=\"Consolas\"];" << std::endl;

    for(int si = 0; si < (int)states_.size(); si++) {
        dot << "    " << si << " [label=\"State " << si;
        for(auto& item : states_[si]) {
            int r = item.rule, p = item.pos;
            int len = prods[r].length;
            dot << "\\n" << signs[prods[r].left] << " ->";
            for(int k = 0; k < len; k++) {
                if(k == p) dot << " .";
                dot << " " << signs[prods[r].right[k]];
            }
            if(p == len) dot << " .";
        }
        dot << "\"];" << std::endl;
    }

    for(int si = 0; si < (int)states_.size(); si++)
        for(auto tr : transitions_[si])
            dot << "    " << si << " -> " << tr.second
                << " [label=\"" << signs[tr.first] << "\"];" << std::endl;

    dot << "}" << std::endl;
    return dot.str();
}

// 保存 DOT 图
void LR0DFA::saveDot(const std::string& filename) const {
    std::ofstream dotFile(filename);
    dotFile << toDotString();
    dotFile.close();
    std::cout << "LR(0) DFA saved to " << filename << std::endl;
}
