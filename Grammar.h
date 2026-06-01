#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "common.h"
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>

class Grammar {
public:
    Grammar();

    // 从文件读入文法
    int loadFromFile(const std::string& filename);
    // 从字符串读入文法（避免文件编码问题）
    int loadFromString(const std::string& content);
    // 添加增广产生式 S' → S $
    void addAugmentedProduction();
    // 计算 FIRST 集
    int computeFirst();
    // 计算 FOLLOW 集
    int computeFollow();

    // 获取符号的 FIRST / FOLLOW
    std::set<int> getFirst(int n) const;
    std::set<int> getFollow(int n) const;

    // 输出
    void printProductions(std::ostream& out) const;
    void printFirstSets(std::ostream& out) const;
    void printFollowSets(std::ostream& out) const;

    // ===== 访问器 =====
    const std::vector<production>& getProductions() const { return grammal_; }
    const std::vector<std::string>& getSignSet() const { return signset_; }
    int getNoncnt() const { return noncnt_; }
    int getEndcnt() const { return endcnt_; }
    int getSTART() const { return START_; }

    // 查找符号索引（公有包装，供 LALR1Parser 做句子词法分析）
    int lookupSymbol(const std::string& s) const;

    // 工具函数
    static std::vector<std::string> split(const std::string& line, char dep = ' ');

private:
    int findpos(const std::string& str) const;

    // 符号表：0~noncnt-1 为非终结符，Gap=#(ε)，MaxSetSize=$，其余为终结符
    std::vector<std::string> signset_;
    int endcnt_;   // 终结符计数（含 # 和 $，初始为 2）
    int noncnt_;   // 非终结符计数
    int START_;    // 开始符索引
    std::vector<production> grammal_;   // 产生式集合
    std::vector<std::set<int>> Firstset_;    // FIRST 集（按非终结符索引）
    std::vector<std::set<int>> Followsets_;  // FOLLOW 集（按非终结符索引）
};

#endif
