#ifndef DFA_H
#define DFA_H

#include "common.h"
#include "Grammar.h"
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <sstream>

// 抽象基类：LR DFA 构建器（模板方法模式）
class DFA {
public:
    DFA(const Grammar& g);
    virtual ~DFA() = default;

    // 模板方法：BFS 构建项目集规范族
    void build();

    // 输出（纯虚，子类自行实现）
    virtual void print(std::ostream& out) const = 0;
    virtual void saveDot(const std::string& filename) const = 0;
    virtual std::string toDotString() const = 0;

    // LALR1DFA 需要的访问器
    const std::vector<LRItemSet>& getStates() const { return states_; }
    const std::vector<TransitionList>& getTransitions() const { return transitions_; }
    int getEdgeCount() const { return edgeCount_; }
    const Grammar& getGrammar() const { return *grammar_; }

protected:
    // 纯虚：子类实现各自的闭包和转移
    virtual LRItemSet closure(LRItemSet I) = 0;
    virtual LRItemSet goTo(LRItemSet I, int X) = 0;
    virtual LRItem createStartItem(int augmentedRuleIndex) const = 0;

    // 共享工具：产生式核心文本 "L -> ... . ..."
    static std::string formatItemCore(const Grammar& g, const LRItem& item);

    const Grammar* grammar_;
    std::vector<LRItemSet> states_;
    std::vector<TransitionList> transitions_;
    int edgeCount_ = 0;
};

#endif
