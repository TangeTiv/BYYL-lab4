#include "LALR1Parser.h"
#include <iomanip>
#include <algorithm>

LALR1Parser::LALR1Parser(const Grammar& g, const LALR1DFA& lalr)
    : grammar_(&g), lalr_(&lalr) {}

// ── 收集所有终结符 / 非终结符（用于表头） ──
void LALR1Parser::collectSymbols_() {
    const auto& signs = grammar_->getSignSet();

    // 终结符（Gap..MaxSetSize，跳过 # (Gap)，包含 $ (MaxSetSize)）
    terminals_.clear();
    for(int i = Gap; i <= MaxSetSize; i++) {
        if(i == Gap) continue;                  // # 不是输入符号
        if(!signs[i].empty())
            terminals_.push_back(i);
    }
    // 确保 $ 在最后
    auto it = std::find(terminals_.begin(), terminals_.end(), MaxSetSize);
    if(it != terminals_.end()) {
        terminals_.erase(it);
        terminals_.push_back(MaxSetSize);
    }

    // 非终结符（0..noncnt-1，跳过 S' = noncnt-1）
    nonterminals_.clear();
    int sPrime = grammar_->getNoncnt() - 1;      // S' 是最后一个非终结符
    for(int i = 0; i < grammar_->getNoncnt(); i++) {
        if(i != sPrime)
            nonterminals_.push_back(i);
    }
}

// ── 构建 ACTION / GOTO 表 ──
void LALR1Parser::buildTable() {
    collectSymbols_();

    int n = (int)lalr_->getLr0States().size();
    const auto& prods = grammar_->getProductions();
    const auto& trans = lalr_->getLr0Transitions();
    int startRule = (int)prods.size() - 1;

    action_.resize(n);
    goto_.resize(n);

    for(int si = 0; si < n; si++) {
        // ① 移进动作：终结符转移
        for(auto& tr : trans[si]) {
            int X = tr.first;
            if(X >= Gap && X <= MaxSetSize) {
                action_[si][X] = "s" + std::to_string(tr.second);
            }
        }

        // ② 归约动作：可归约项的 lookahead
        for(auto& item : lalr_->getLr0States()[si]) {
            int r = item.rule, p = item.pos;
            if(p < (int)prods[r].length) continue;  // 非归约项

            const std::set<int>& las = lalr_->getLookahead(si, r, p);
            for(int a : las) {
                if(a == MaxSetSize && r == startRule) {
                    action_[si][a] = "acc";
                } else {
                    action_[si][a] = "r" + std::to_string(r);
                }
            }
        }

        // ③ GOTO：非终结符转移
        for(auto& tr : trans[si]) {
            int X = tr.first;
            if(X < Gap) {
                goto_[si][X] = tr.second;
            }
        }
    }
}

// ── 符号显示名 ──
std::string LALR1Parser::symName_(int sym) const {
    const auto& signs = grammar_->getSignSet();
    if(sym == 100) return "#";
    if(sym == MaxSetSize) return "$";
    if(sym >= 0 && sym < (int)signs.size() && !signs[sym].empty())
        return signs[sym];
    return "?";
}

// ── 表格单元格文本 ──
std::string LALR1Parser::cellStr_(int state, int sym, bool isGoto) const {
    if(isGoto) {
        auto it = goto_[state].find(sym);
        if(it != goto_[state].end())
            return std::to_string(it->second);
    } else {
        auto it = action_[state].find(sym);
        if(it != action_[state].end())
            return it->second;
    }
    return "";
}

// ── 打印分析表 ──
void LALR1Parser::printTable(std::ostream& out, bool showEmpty) const {
    if(action_.empty()) {
        out << "(Table not built yet)" << std::endl;
        return;
    }

    int n = (int)lalr_->getLr0States().size();

    // ── ACTION 表 ──
    out << "\n============= LALR(1) ACTION Table ===============" << std::endl;

    // 计算列宽
    std::vector<int> colW(terminals_.size(), 3);
    for(size_t j = 0; j < terminals_.size(); j++)
        colW[j] = std::max(colW[j], (int)symName_(terminals_[j]).size() + 1);
    for(int si = 0; si < n; si++) {
        for(size_t j = 0; j < terminals_.size(); j++) {
            std::string c = cellStr_(si, terminals_[j], false);
            if(!c.empty() || showEmpty)
                colW[j] = std::max(colW[j], (int)c.size() + 1);
        }
    }

    // 表头
    auto printSep = [&]() {
        out << "+";
        out << std::string(7, '-');
        for(size_t j = 0; j < terminals_.size(); j++) {
            out << "+" << std::string(colW[j], '-');
        }
        out << "+" << std::endl;
    };

    printSep();
    out << "| State";
    for(size_t j = 0; j < terminals_.size(); j++)
        out << "|" << std::setw(colW[j]) << std::right << symName_(terminals_[j]);
    out << "|" << std::endl;
    printSep();

    for(int si = 0; si < n; si++) {
        out << "| " << std::setw(5) << std::right << si;
        bool rowHasContent = false;
        for(size_t j = 0; j < terminals_.size(); j++) {
            std::string c = cellStr_(si, terminals_[j], false);
            if(!c.empty()) rowHasContent = true;
            if(!c.empty() || showEmpty)
                out << "|" << std::setw(colW[j]) << std::right << c;
            else
                out << "|" << std::string(colW[j], ' ');
        }
        out << "|" << std::endl;
        if(rowHasContent) printSep();
    }
    printSep();

    // ── GOTO 表 ──
    out << "\n============= LALR(1) GOTO Table =================" << std::endl;

    if(nonterminals_.empty()) {
        out << "(no non-terminals)" << std::endl;
        return;
    }

    std::vector<int> gotoColW(nonterminals_.size(), 3);
    for(size_t j = 0; j < nonterminals_.size(); j++)
        gotoColW[j] = std::max(gotoColW[j], (int)symName_(nonterminals_[j]).size() + 1);
    for(int si = 0; si < n; si++) {
        for(size_t j = 0; j < nonterminals_.size(); j++) {
            std::string c = cellStr_(si, nonterminals_[j], true);
            if(!c.empty() || showEmpty)
                gotoColW[j] = std::max(gotoColW[j], (int)c.size() + 1);
        }
    }

    auto printGotoSep = [&]() {
        out << "+";
        out << std::string(7, '-');
        for(size_t j = 0; j < nonterminals_.size(); j++)
            out << "+" << std::string(gotoColW[j], '-');
        out << "+" << std::endl;
    };

    printGotoSep();
    out << "| State";
    for(size_t j = 0; j < nonterminals_.size(); j++)
        out << "|" << std::setw(gotoColW[j]) << std::right << symName_(nonterminals_[j]);
    out << "|" << std::endl;
    printGotoSep();

    for(int si = 0; si < n; si++) {
        bool rowHasContent = false;
        for(size_t j = 0; j < nonterminals_.size(); j++)
            if(!cellStr_(si, nonterminals_[j], true).empty()) rowHasContent = true;
        if(!rowHasContent) continue;

        out << "| " << std::setw(5) << std::right << si;
        for(size_t j = 0; j < nonterminals_.size(); j++) {
            std::string c = cellStr_(si, nonterminals_[j], true);
            if(!c.empty() || showEmpty)
                out << "|" << std::setw(gotoColW[j]) << std::right << c;
            else
                out << "|" << std::string(gotoColW[j], ' ');
        }
        out << "|" << std::endl;
        printGotoSep();
    }
    printGotoSep();
}

// ── 句子 Token 化 ──
std::vector<int> LALR1Parser::tokenize(const std::string& sentence) const {
    const auto& signs = grammar_->getSignSet();
    std::vector<int> result;
    std::string s = sentence;

    // 收集所有终结符名（用于最长匹配，按长度降序排列）
    struct TermInfo { std::string name; int index; };
    std::vector<TermInfo> termList;
    for(int i = Gap; i <= MaxSetSize; i++) {
        if(i == Gap) continue;  // 跳过 #
        if(!signs[i].empty())
            termList.push_back({signs[i], i});
    }
    // 按长度降序排列（最长匹配优先）
    std::sort(termList.begin(), termList.end(),
        [](const TermInfo& a, const TermInfo& b) {
            if(a.name.size() != b.name.size())
                return a.name.size() > b.name.size();
            return a.name < b.name;
        });

    // 按空格分割（先尝试空白分隔）
    std::istringstream iss(s);
    std::string token;
    bool usedSpaceSplit = false;
    std::vector<std::string> parts;

    while(iss >> token) parts.push_back(token);
    if(parts.size() > 1) usedSpaceSplit = true;

    if(usedSpaceSplit) {
        // 空白分割模式
        for(auto& part : parts) {
            int idx = grammar_->lookupSymbol(part);
            if(idx == -1) {
                // 尝试在终结符中模糊匹配
                bool found = false;
                for(auto& t : termList) {
                    if(t.name == part) {
                        result.push_back(t.index);
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    result.clear();
                    result.push_back(-1);  // 标记错误
                    return result;
                }
            } else {
                if(idx >= Gap) result.push_back(idx);
                else { result.clear(); result.push_back(-1); return result; }
            }
        }
        return result;
    }

    // 空白分割失败 → 最长前缀匹配
    // 先去除首尾空格
    s.erase(0, s.find_first_not_of(" \t"));
    s.erase(s.find_last_not_of(" \t") + 1);

    while(!s.empty()) {
        bool matched = false;
        for(auto& t : termList) {
            if(s.compare(0, t.name.size(), t.name) == 0) {
                result.push_back(t.index);
                s = s.substr(t.name.size());
                matched = true;
                break;
            }
        }
        if(!matched) {
            // 跳过空白
            if(s[0] == ' ' || s[0] == '\t') {
                s = s.substr(1);
                continue;
            }
            result.clear();
            result.push_back(-1);
            return result;
        }
    }

    return result;
}

// ── 语法分析 ──
LALR1Parser::ParseResult LALR1Parser::parse(const std::vector<int>& userTokens) const {
    ParseResult result;
    const auto& prods = grammar_->getProductions();

    // 输入序列 = 用户 token 序列 + $
    result.tokens = userTokens;
    result.tokens.push_back(MaxSetSize);
    const auto& input = result.tokens;

    // 状态栈和符号栈
    std::vector<int> states;   // 状态栈
    std::vector<int> symbols;  // 符号栈

    states.push_back(0);
    symbols.push_back(-1);     // 初始占位

    int inputPos = 0;
    int stepCount = 0;
    const int MAX_STEPS = 1000;

    auto saveStep = [&](const std::string& act) {
        if(stepCount >= MAX_STEPS) return;
        Step st;
        st.stateStack = states;
        st.symStack = symbols;
        st.nextInputIdx = inputPos;
        st.action = act;
        result.steps.push_back(st);
        stepCount++;
    };

    saveStep("");

    while(stepCount < MAX_STEPS) {
        int curState = states.back();
        int lookahead = (inputPos < (int)input.size()) ? input[inputPos] : MaxSetSize;

        // 查 ACTION 表
        auto actIt = action_[curState].find(lookahead);
        if(actIt == action_[curState].end()) {
            std::ostringstream err;
            err << "语法错误：状态 " << curState
                << " 在符号 " << symName_(lookahead) << " 下无动作";
            result.accepted = false;
            result.errorPos = inputPos;
            result.errorMsg = err.str();
            saveStep("ERROR");
            return result;
        }

        const std::string& act = actIt->second;

        if(act == "acc") {
            saveStep("Accept");
            result.accepted = true;
            return result;
        }

        if(act[0] == 's') {
            // Shift
            int target = std::stoi(act.substr(1));
            int sym = lookahead;
            symbols.push_back(sym);
            states.push_back(target);
            inputPos++;

            std::string desc = "Shift " + symName_(sym) + " → " + std::to_string(target);
            saveStep(desc);
        }
        else if(act[0] == 'r') {
            // Reduce
            int ruleIdx = std::stoi(act.substr(1));
            const production& rule = prods[ruleIdx];
            int popLen = rule.length;

            // 弹出 |α| 个符号和 |α| 个状态
            for(int i = 0; i < popLen; i++) {
                if(!symbols.empty()) symbols.pop_back();
                if(!states.empty()) states.pop_back();
            }

            int prevState = states.back();
            int lhs = rule.left;

            // 查 GOTO 表
            auto goIt = goto_[prevState].find(lhs);
            if(goIt == goto_[prevState].end()) {
                std::ostringstream err;
                err << "语法错误：状态 " << prevState
                    << " 在非终结符 " << symName_(lhs) << " 下无转移";
                result.accepted = false;
                result.errorPos = inputPos;
                result.errorMsg = err.str();
                saveStep("ERROR");
                return result;
            }

            symbols.push_back(lhs);
            states.push_back(goIt->second);

            std::ostringstream desc;
            desc << "Reduce by Rule " << ruleIdx << ": "
                 << symName_(lhs) << " →";
            for(int k = 0; k < rule.length; k++)
                desc << " " << symName_(rule.right[k]);
            saveStep(desc.str());
        }
        else {
            std::ostringstream err;
            err << "未知动作: " << act;
            result.accepted = false;
            result.errorPos = inputPos;
            result.errorMsg = err.str();
            saveStep("ERROR");
            return result;
        }
    }

    // 超时保护
    result.accepted = false;
    result.errorMsg = "解析超过最大步数限制";
    saveStep("ERROR (timeout)");
    return result;
}

// ── 打印追踪过程 ──
void LALR1Parser::printTrace(const ParseResult& result, std::ostream& out) const {
    if(result.steps.empty()) return;

    // 列宽计算
    const int MIN_SW = 16;
    int maxStatesLen = MIN_SW;
    int maxSymLen = MIN_SW;

    for(auto& st : result.steps) {
        // 状态栈文本
        std::string s;
        for(size_t i = 0; i < st.stateStack.size(); i++) {
            if(i > 0) s += " ";
            s += std::to_string(st.stateStack[i]);
        }
        maxStatesLen = std::max(maxStatesLen, (int)s.size() + 4);

        // 符号栈文本
        std::string s2;
        for(size_t i = 0; i < st.symStack.size(); i++) {
            if(i > 0) s2 += " ";
            if(st.symStack[i] >= 0)
                s2 += symName_(st.symStack[i]);
        }
        maxSymLen = std::max(maxSymLen, (int)s2.size() + 4);
    }

    // 定界符
    std::string sep = "+" + std::string(6, '-')
                    + "+" + std::string(maxStatesLen, '-')
                    + "+" + std::string(maxSymLen, '-')
                    + "+" + std::string(20, '-')
                    + "+" + std::string(50, '-') + "+";

    auto printRow = [&](const std::string& step, const std::string& statesStr,
                        const std::string& symStr, const std::string& inputStr,
                        const std::string& actionStr) {
        out << "| " << std::setw(4) << std::right << step;
        out << "| " << std::setw(maxStatesLen-1) << std::left << statesStr;
        out << "| " << std::setw(maxSymLen-1) << std::left << symStr;
        out << "| " << std::setw(18) << std::left << inputStr;
        out << "| " << std::setw(48) << std::left << actionStr;
        out << "|" << std::endl;
    };

    out << "\n==================== LALR(1) Parse Trace ====================" << std::endl;
    out << "Input tokens:";
    for(size_t i = 0; i < result.tokens.size(); i++)
        out << " " << symName_(result.tokens[i]);
    out << std::endl << std::endl;

    out << sep << std::endl;
    printRow("Step", "State Stack", "Symbol Stack", "Remaining Input", "Action");
    out << sep << std::endl;

    for(size_t si = 0; si < result.steps.size(); si++) {
        auto& st = result.steps[si];

        // 状态栈文本
        std::string statesStr;
        for(size_t i = 0; i < st.stateStack.size(); i++) {
            if(i > 0) statesStr += " ";
            statesStr += std::to_string(st.stateStack[i]);
        }

        // 符号栈文本（跳过初始占位符 -1）
        std::string symStr;
        for(size_t i = 0; i < st.symStack.size(); i++) {
            if(st.symStack[i] < 0) continue;
            if(!symStr.empty()) symStr += " ";
            symStr += symName_(st.symStack[i]);
        }

        // 剩余输入文本
        std::string inputStr;
        for(int i = st.nextInputIdx; i < (int)result.tokens.size(); i++) {
            if(!inputStr.empty()) inputStr += " ";
            inputStr += symName_(result.tokens[i]);
        }
        if(inputStr.empty()) inputStr = "";

        printRow(std::to_string((int)si),
                 statesStr, symStr, inputStr, st.action);
        out << sep << std::endl;
    }

    // 结果
    out << std::endl;
    if(result.accepted) {
        out << "✔ ACCEPTED!" << std::endl;
    } else {
        out << "✘ REJECTED" << std::endl;
        out << "  " << result.errorMsg << std::endl;
    }
    out << "=============================================================" << std::endl;
}
