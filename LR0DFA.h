#ifndef LR0DFA_H
#define LR0DFA_H

#include "DFA.h"

class LR0DFA : public DFA {
public:
    explicit LR0DFA(const Grammar& g);

    // LR(0) 冲突检测
    bool isLR0() const;
    // SLR(1) 冲突检测（利用 FOLLOW 集解决 LR(0) 冲突）
    bool isSLR1() const;
    // SLR(1) 冲突检测，返回详细冲突信息
    bool isSLR1(std::string& conflicts) const;

    // 输出
    void print(std::ostream& out) const override;
    void saveDot(const std::string& filename) const override;

protected:
    LRItemSet closure(LRItemSet I) override;
    LRItemSet goTo(LRItemSet I, int X) override;
    LRItem createStartItem(int augmentedRuleIndex) const override;
};

#endif
