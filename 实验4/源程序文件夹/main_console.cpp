#include "Grammar.h"
#include "LR0DFA.h"
#include "LR1DFA.h"
#include "LALR1DFA.h"
#include "LALR1Parser.h"
#include <iostream>
#include <string>

int main() {
    // 1. 读取文法
    Grammar grammar;
    grammar.loadFromFile("testdata.txt");

    // 2. 添加增广产生式 S' → S $
    grammar.addAugmentedProduction();

    // 3. 计算 FIRST / FOLLOW
    grammar.computeFirst();
    grammar.computeFollow();

    // 4. 输出产生式、FIRST、FOLLOW 集
    std::cout << "----- Productions -----" << std::endl;
    grammar.printProductions(std::cout);

    std::cout << "\n========== FIRST Sets ==========" << std::endl;
    grammar.printFirstSets(std::cout);
    std::cout << "================================" << std::endl;

    std::cout << "\n========== FOLLOW Sets =========" << std::endl;
    grammar.printFollowSets(std::cout);
    std::cout << "================================" << std::endl;

    // 5. 构建 LR(0) DFA
    LR0DFA lr0(grammar);
    lr0.build();
    lr0.print(std::cout);
    lr0.saveDot("LR0_DFA.dot");

    // 6. 构建 LR(1) DFA
    LR1DFA lr1(grammar);
    lr1.build();
    lr1.print(std::cout);
    lr1.saveDot("LR1_DFA.dot");

    // 7. 构建 LALR(1) DFA（先行符号传播法）
    LALR1DFA lalr(grammar, lr0);
    lalr.build();
    lalr.print(std::cout);
    lalr.saveDot("LALR1_DFA.dot");

    // ── 阶段4：选做功能 ──

    // 选做功能1 & 3：LALR(1) 分析表 + 句子分析
    LALR1Parser parser(grammar, lalr);
    parser.buildTable();
    parser.printTable(std::cout);

    // 选做功能2 & 3：交互式句子分析
    std::cout << "\n========== LALR(1) Sentence Parser ==========" << std::endl;
    std::cout << "Enter sentences to parse. Type 'quit' to exit." << std::endl;
    std::cout << "Tokens are whitespace-separated or single characters." << std::endl;
    std::cout << "Example: a, ( a ), a + a" << std::endl;
    std::cout << "=============================================" << std::endl;

    std::string line;
    while(true) {
        std::cout << "\n> ";
        if(!std::getline(std::cin, line)) break;

        // 去掉首尾空格
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if(line.empty()) continue;
        if(line == "quit" || line == "exit") break;
        if(line == "table") {
            parser.printTable(std::cout);
            continue;
        }

        // Tokenize
        std::vector<int> tokens = parser.tokenize(line);
        if(tokens.empty()) {
            std::cout << "(empty input)" << std::endl;
            continue;
        }
        if(tokens.size() == 1 && tokens[0] == -1) {
            std::cout << "Unknown token in input. Use whitespace-separated tokens" << std::endl;
            std::cout << "or single characters matching the grammar's terminals." << std::endl;
            continue;
        }

        // Parse
        auto result = parser.parse(tokens);
        parser.printTrace(result, std::cout);
    }

    std::cout << "\nBye!" << std::endl;
    return 0;
}
