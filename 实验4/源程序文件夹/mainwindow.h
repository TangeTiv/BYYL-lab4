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
#include <QSvgWidget>
#include <QScrollArea>

#include "Grammar.h"
#include "LR0DFA.h"
#include "LR1DFA.h"
#include "LALR1DFA.h"
#include "LALR1Parser.h"
#include "DotRenderer.h"

// DFA 标签页控件组
struct DFATabWidgets {
    QSplitter *splitter;
    QTableWidget *table;
    QSvgWidget *svgWidget;
    QScrollArea *scrollArea;
    QLabel *statusLabel;
};

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
    void onSvgReady(const QByteArray &svgData, int tag);
    void onRenderError(const QString &error, int tag);

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

    // Tab 3~5: DFA 标签页（含表格 + SVG 图形）
    DFATabWidgets dfaTabs_[3];   // 0=LR0, 1=LR1, 2=LALR1
    DotRenderer* dotRenderers_[3];  // 每个 DFA 标签页独立渲染器

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
    DFATabWidgets createDFATab(const QString& title);
    QWidget* createParsingTableTab();
    QWidget* createParseTab();

    // 数据填充
    void rebuildAll();
    void renderDFAGraph(int tabIndex, const std::string& dotString);
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
