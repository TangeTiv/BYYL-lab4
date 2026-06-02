#include "LR1DFA.h"
#include <fstream>

LR1DFA::LR1DFA(const Grammar& g) : DFA(g) {}

// LR(1) 闭包：带 lookahead 传播
LRItemSet LR1DFA::closure(LRItemSet I) {
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

            // 计算 First(βa)
            std::set<int> firstOfBeta;
            bool allBetaNullable = true;

            for(int k = p + 1; k < (int)grammar_->getProductions()[r].length; k++) {
                int sym = grammar_->getProductions()[r].right[k];
                std::set<int> fk = grammar_->getFirst(sym);

                for(int x : fk)
                    if(x != 100)
                        firstOfBeta.insert(x);

                if(fk.find(100) == fk.end()) {
                    allBetaNullable = false;
                    break;
                }
            }

            // β 全部可空 → 加入原 item 的 preview
            if(allBetaNullable) {
                for(int kp = 0; kp < item.k; kp++)
                    firstOfBeta.insert(item.preview[kp]);
            }

            // 展开所有 B → ·γ，每个 lookahead 作为一个新项目
            const auto& prods = grammar_->getProductions();
            for(int pi = 0; pi < (int)prods.size(); pi++) {
                if(prods[pi].left != B) continue;

                for(int la : firstOfBeta) {
                    LRItem newItem;
                    newItem.rule = pi;
                    newItem.pos  = 0;
                    newItem.k    = 1;
                    newItem.preview[0] = la;

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

// LR(1) GoTo：保留 preview，调 closure_LR1
LRItemSet LR1DFA::goTo(LRItemSet I, int X) {
    LRItemSet J;
    const auto& prods = grammar_->getProductions();
    for(auto item : I) {
        int r = item.rule, p = item.pos;
        if(p < (int)prods[r].length && prods[r].right[p] == X) {
            LRItem newItem;
            newItem.rule = r;
            newItem.pos  = p + 1;
            newItem.k    = item.k;
            for(int i = 0; i < item.k; i++)
                newItem.preview[i] = item.preview[i];
            J.insert(newItem);
        }
    }
    return closure(J);
}

// LR(1) 起始项：k=1, preview[0]=$
LRItem LR1DFA::createStartItem(int augmentedRuleIndex) const {
    LRItem item;
    item.rule = augmentedRuleIndex;
    item.pos  = 0;
    item.k    = 1;
    item.preview[0] = MaxSetSize;
    return item;
}

// 输出到控制台
void LR1DFA::print(std::ostream& out) const {
    const auto& signs = grammar_->getSignSet();

    out << "\n========== LR(1) DFA ==========" << std::endl;
    out << states_.size() << " states, " << edgeCount_ << " edges" << std::endl << std::endl;

    for(int si = 0; si < (int)states_.size(); si++) {
        out << "--- State " << si << " ---" << std::endl;
        for(auto item : states_[si]) {
            out << "  " << formatItemCore(*grammar_, item);
            out << "  (Rule " << item.rule;
            if(item.k > 0) {
                out << ", preview=[";
                for(int i = 0; i < item.k; i++) {
                    if(i > 0) out << " ";
                    if(item.preview[i] == 100) out << "#";
                    else if(item.preview[i] == MaxSetSize) out << "$";
                    else out << signs[item.preview[i]];
                }
                out << "]";
            }
            out << ")" << std::endl;
        }
        for(auto tr : transitions_[si]) {
            out << "  --" << signs[tr.first] << "--> State " << tr.second << std::endl;
        }
        if(!transitions_[si].empty()) out << std::endl;
    }
    out << "================================" << std::endl;
}

// 生成 DOT 字符串
std::string LR1DFA::toDotString() const {
    const auto& signs = grammar_->getSignSet();
    const auto& prods = grammar_->getProductions();
    std::ostringstream dot;
    dot << "digraph LR1_DFA {" << std::endl;
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

            if(item.k > 0) {
                dot << "  [";
                for(int i = 0; i < item.k; i++) {
                    if(item.preview[i] == 100) dot << "#";
                    else if(item.preview[i] == MaxSetSize) dot << "$";
                    else dot << signs[item.preview[i]];
                }
                dot << "]";
            }
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
void LR1DFA::saveDot(const std::string& filename) const {
    std::ofstream dotFile(filename);
    dotFile << toDotString();
    dotFile.close();
    std::cout << "LR(1) DFA saved to " << filename << std::endl;
}
