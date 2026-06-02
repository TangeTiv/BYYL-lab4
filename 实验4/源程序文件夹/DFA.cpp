#include "DFA.h"

DFA::DFA(const Grammar& g) : grammar_(&g), edgeCount_(0) {}

std::string DFA::formatItemCore(const Grammar& g, const LRItem& item) {
    const auto& prods = g.getProductions();
    const auto& signs = g.getSignSet();
    int r = item.rule, p = item.pos;
    int len = prods[r].length;

    std::ostringstream oss;
    oss << signs[prods[r].left] << " ->";
    for(int k = 0; k < len; k++) {
        if(k == p) oss << " .";
        oss << " " << signs[prods[r].right[k]];
    }
    if(p == len) oss << " .";
    return oss.str();
}

// BFS 模板方法：构建项目集规范族
void DFA::build() {
    int augRule = (int)grammar_->getProductions().size() - 1;

    // 初始项目
    LRItem startItem = createStartItem(augRule);
    LRItemSet startSet;
    startSet.insert(startItem);
    startSet = closure(startSet);

    // 初始化
    states_.push_back(startSet);
    transitions_.push_back({});
    std::vector<int> queue = {0};

    for(int idx = 0; idx < (int)queue.size(); idx++) {
        int cur = queue[idx];
        std::set<int> symbols;

        // 收集 · 后的所有符号
        for(auto& item : states_[cur]) {
            int r = item.rule, p = item.pos;
            if(p < (int)grammar_->getProductions()[r].length)
                symbols.insert(grammar_->getProductions()[r].right[p]);
        }

        // 对每个符号计算 GoTo
        for(int X : symbols) {
            auto newSet = goTo(states_[cur], X);
            if(newSet.empty()) continue;

            int found = -1;
            for(int si = 0; si < (int)states_.size(); si++) {
                if(itemSetsEqual(states_[si], newSet)) {
                    found = si;
                    break;
                }
            }

            if(found == -1) {
                found = (int)states_.size();
                states_.push_back(newSet);
                queue.push_back(found);
                transitions_.push_back({});
            }
            transitions_[cur].push_back({X, found});
            edgeCount_++;
        }
    }
}
