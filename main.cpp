#include "Grammar.h"
#include "LR0DFA.h"
#include "LR1DFA.h"
#include "LALR1DFA.h"

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

    return 0;
}
