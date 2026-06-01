#ifndef LR1DFA_H
#define LR1DFA_H

#include "DFA.h"

class LR1DFA : public DFA {
public:
    explicit LR1DFA(const Grammar& g);

    // 输出
    void print(std::ostream& out) const override;
    void saveDot(const std::string& filename) const override;

protected:
    LRItemSet closure(LRItemSet I) override;
    LRItemSet goTo(LRItemSet I, int X) override;
    LRItem createStartItem(int augmentedRuleIndex) const override;
};

#endif
