#include "LR0DFA.h"
#include <fstream>

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

// 输出到控制台
void LR0DFA::print(std::ostream& out) const {
    const auto& signs = grammar_->getSignSet();
    const auto& prods = grammar_->getProductions();

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
}

// 保存 DOT 图
void LR0DFA::saveDot(const std::string& filename) const {
    const auto& signs = grammar_->getSignSet();
    std::ofstream dotFile(filename);
    dotFile << "digraph LR0_DFA {" << std::endl;
    dotFile << "    rankdir=LR;" << std::endl;
    dotFile << "    node [shape=box, style=rounded, fontname=\"Consolas\"];" << std::endl;

    for(int si = 0; si < (int)states_.size(); si++) {
        dotFile << "    " << si << " [label=\"State " << si;
        for(auto item : states_[si]) {
            dotFile << "\\n" << formatItemCore(*grammar_, item);
        }
        dotFile << "\"];" << std::endl;
    }

    for(int si = 0; si < (int)states_.size(); si++)
        for(auto tr : transitions_[si])
            dotFile << "    " << si << " -> " << tr.second
                    << " [label=\"" << signs[tr.first] << "\"];" << std::endl;

    dotFile << "}" << std::endl;
    dotFile.close();
    std::cout << "LR(0) DFA saved to " << filename << std::endl;
}
