#ifndef LALR1PARSER_H
#define LALR1PARSER_H

#include "common.h"
#include "Grammar.h"
#include "LALR1DFA.h"
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <sstream>

// LALR(1) 分析表与语法分析驱动
class LALR1Parser {
public:
    LALR1Parser(const Grammar& g, const LALR1DFA& lalr);

    // 构建 ACTION / GOTO 表
    void buildTable();

    // 打印分析表
    void printTable(std::ostream& out, bool showEmpty = true) const;

    // 将句子字符串 token 化为终结符索引序列（不含 $）
    std::vector<int> tokenize(const std::string& sentence) const;

    // ----- 解析结果 -----
    struct Step {
        std::vector<int> stateStack;    // 状态栈备份（从左到右=栈底到栈顶）
        std::vector<int> symStack;      // 符号栈备份
        int nextInputIdx;               // 当前输入位置（用于显示剩余输入）
        std::string action;             // 动作描述
    };

    struct ParseResult {
        bool accepted = false;
        std::vector<Step> steps;
        int errorPos = -1;              // 出错时的输入位置
        std::string errorMsg;
        std::vector<int> tokens;        // 完整的输入 token 序列（含 $）
    };

    // 执行解析，返回详细结果
    ParseResult parse(const std::vector<int>& userTokens) const;

    // 打印追踪过程
    void printTrace(const ParseResult& result, std::ostream& out) const;

    // ===== 访问器（供 GUI 使用） =====
    const std::vector<std::map<int, std::string>>& getActionTable() const { return action_; }
    const std::vector<std::map<int, int>>& getGotoTable() const { return goto_; }
    const std::vector<int>& getTerminals() const { return terminals_; }
    const std::vector<int>& getNonterminals() const { return nonterminals_; }
    std::string symName(int sym) const { return symName_(sym); }
    const Grammar& getGrammar() const { return *grammar_; }

private:
    const Grammar* grammar_;
    const LALR1DFA* lalr_;

    // action_[state][terminal]   = "sN", "rN", "acc"
    std::vector<std::map<int, std::string>> action_;
    // goto_[state][nonterminal]  = target_state
    std::vector<std::map<int, int>> goto_;

    // 表格显示用
    std::vector<int> terminals_;     // 所有终结符（含 $，不含 #）
    std::vector<int> nonterminals_;  // 所有非终结符（不含 S'）

    void collectSymbols_();
    std::string cellStr_(int state, int sym, bool isGoto) const;

    // 符号 → 显示名
    std::string symName_(int sym) const;
};

#endif
