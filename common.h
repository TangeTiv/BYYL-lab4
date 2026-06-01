#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <set>
#include <string>
#include <utility>   // pair

#define MaxProLength 20
#define MaxSetSize 200
#define MaxGramSize 100
#define Gap 100
#define MaxK 5

// 产生式
struct production {
    int left;
    int right[MaxProLength];
    int length;
};

// LR 项目
struct LRItem {
    int rule;          // 产生式索引
    int pos;           // 点·的位置
    int preview[MaxK]; // lookahead 数组
    int k;             // lookahead 数量
};

// LRItem 完整比较器（含 preview 比较）
struct LRItemCompare {
    bool operator()(const LRItem& a, const LRItem& b) const {
        if(a.rule != b.rule) return a.rule < b.rule;
        if(a.pos  != b.pos)  return a.pos  < b.pos;
        if(a.k    != b.k)    return a.k    < b.k;
        for(int i = 0; i < a.k && i < b.k; i++) {
            if(a.preview[i] != b.preview[i])
                return a.preview[i] < b.preview[i];
        }
        return false;
    }
};

// LALR 核心比较器（忽略 preview）
struct LALRItemCompare {
    bool operator()(const LRItem& a, const LRItem& b) const {
        if(a.rule != b.rule) return a.rule < b.rule;
        if(a.pos  != b.pos)  return a.pos  < b.pos;
        return false;
    }
};

// 状态判等（含 preview 比较）
inline bool itemSetsEqual(const std::set<LRItem, LRItemCompare>& a,
                          const std::set<LRItem, LRItemCompare>& b) {
    if(a.size() != b.size()) return false;
    auto ia = a.begin(), ib = b.begin();
    while(ia != a.end()) {
        if(ia->rule != ib->rule || ia->pos != ib->pos) return false;
        if(ia->k != ib->k) return false;
        for(int i = 0; i < ia->k; i++) {
            if(ia->preview[i] != ib->preview[i]) return false;
        }
        ++ia; ++ib;
    }
    return true;
}

// 类型别名
using LRItemSet = std::set<LRItem, LRItemCompare>;
using Transition = std::pair<int,int>;
using TransitionList = std::vector<Transition>;

#endif
