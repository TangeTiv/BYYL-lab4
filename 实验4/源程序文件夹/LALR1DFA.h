#ifndef LALR1DFA_H
#define LALR1DFA_H

#include "common.h"
#include "Grammar.h"
#include "LR0DFA.h"
#include <map>
#include <set>

class LALR1DFA {
public:
    LALR1DFA(const Grammar& g, const LR0DFA& lr0);

    // DeRemer-Pennello 先行符号传播
    void build();

    // 输出
    void print(std::ostream& out) const;
    void saveDot(const std::string& filename) const;
    std::string toDotString() const;

    // LALR(1) 冲突检测
    bool isLALR1() const;

    // 访问 LR(0) 状态与转移（供分析表构建使用）
    const std::vector<LRItemSet>& getLr0States() const { return lr0_->getStates(); }
    const std::vector<TransitionList>& getLr0Transitions() const { return lr0_->getTransitions(); }
    const Grammar& getGrammar() const { return *grammar_; }

    // 访问 lookahead（供后续表构建使用）
    const std::set<int>& getLookahead(int state, int rule, int pos) const;

private:
    using ItemKey = std::pair<int,int>;

    const Grammar* grammar_;
    const LR0DFA* lr0_;

    // la[state][(rule,pos)] = lookahead 集合
    std::vector<std::map<ItemKey, std::set<int>>> lookaheads_;
    bool built_ = false;
};

#endif
