#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QStatusBar>
#include <QFont>
#include <QTextStream>
#include <QTimer>

#include "Grammar.h"
#include "LR0DFA.h"
#include "LR1DFA.h"
#include "LALR1DFA.h"
#include "LALR1Parser.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoadGrammar();
    void onSaveGrammar();
    void onParseSentence();
    void onClearTrace();

private:
    // 核心算法对象
    Grammar* grammar_;
    LR0DFA* lr0_;
    LR1DFA* lr1_;
    LALR1DFA* lalr_;
    LALR1Parser* parser_;
    bool built_;

    // UI 组件
    QTabWidget* mainTabs_;

    // Tab 1: 文法编辑
    QPlainTextEdit* grammarEditor_;
    QLabel* grammarFileLabel_;
    QLabel* grammarResultLabel_;
    QString currentFilePath_;

    // Tab 2: FIRST/FOLLOW
    QTableWidget* firstFollowTable_;

    // Tab 3: LR(0) DFA
    QTableWidget* lr0Table_;

    // Tab 4: LR(1) DFA
    QTableWidget* lr1Table_;

    // Tab 5: LALR(1) DFA
    QWidget* lalrTabWidget_;   // 占位容器
    QTableWidget* lalrTable_;  // 运行时创建

    // Tab 6: 分析表
    QTableWidget* actionTable_;
    QTableWidget* gotoTable_;

    // Tab 7: 句子分析
    QLineEdit* sentenceEdit_;
    QPushButton* parseBtn_;
    QPushButton* clearBtn_;
    QTableWidget* traceTable_;
    QLabel* parseResultLabel_;

    // 构建 UI
    void setupUI();
    QWidget* createGrammarTab();
    QWidget* createFirstFollowTab();
    QWidget* createDFATab(const QString& title);
    QWidget* createParsingTableTab();
    QWidget* createParseTab();

    // 数据填充
    void rebuildAll();
    void populateFirstFollow();
    void populateDFATable(QTableWidget* table,
                          const std::vector<LRItemSet>& states,
                          const std::vector<TransitionList>& trans,
                          int showLA = 0);
    void populateActionTable();
    void populateGotoTable();
    void populateTraceTable(const LALR1Parser::ParseResult& result);

    // 工具
    std::string symName(int sym) const;
    std::string itemStr(const production& prod, int pos,
                        const std::set<int>* la = nullptr) const;
};

#endif // MAINWINDOW_H
