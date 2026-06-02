#include "mainwindow.h"
#include <sstream>
#include <fstream>
#include <QTimer>
#include <QDir>
#include <QFile>

// ═══════════════════════════════════════════════
//  MainWindow 实现
// ═══════════════════════════════════════════════

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , grammar_(nullptr), lr0_(nullptr), lr1_(nullptr)
    , lalr_(nullptr), parser_(nullptr), built_(false)
    , dotRenderers_{nullptr, nullptr, nullptr}
{
    setupUI();

    // 每个 DFA 标签页独立 DotRenderer，避免互相 kill 进程
    for (int i = 0; i < 3; i++) {
        dotRenderers_[i] = new DotRenderer(this);
        connect(dotRenderers_[i], &DotRenderer::svgReady,
                this, [this, i](const QByteArray &svg, int) {
                    onSvgReady(svg, i);
                });
        connect(dotRenderers_[i], &DotRenderer::renderError,
                this, [this, i](const QString &err, int) {
                    onRenderError(err, i);
                });
    }
}

MainWindow::~MainWindow() {
    delete parser_;
    delete lalr_;
    delete lr1_;
    delete lr0_;
    delete grammar_;
}

// ── 符号显示名 ──
std::string MainWindow::symName(int sym) const {
    if(!grammar_) return "?";
    const auto& signs = grammar_->getSignSet();
    if(sym == 100) return "#";
    if(sym == MaxSetSize) return "$";
    if(sym >= 0 && sym < (int)signs.size() && !signs[sym].empty())
        return signs[sym];
    return "?";
}

// ── 项目文本 ──
std::string MainWindow::itemStr(const production& prod, int pos,
                                const std::set<int>* la) const {
    std::string s = symName(prod.left) + " ->";
    for(int k = 0; k < prod.length; k++) {
        if(k == pos) s += " .";
        s += " " + symName(prod.right[k]);
    }
    if(pos == prod.length) s += " .";

    if(la && !la->empty()) {
        s += "  [";
        bool first = true;
        for(int a : *la) {
            if(!first) s += " ";
            first = false;
            if(a == 100) s += "#";
            else if(a == MaxSetSize) s += "$";
            else s += symName(a);
        }
        s += "]";
    }
    return s;
}

// ═══════════════════════════════════════════════
//  UI 构建
// ═══════════════════════════════════════════════

void MainWindow::setupUI() {
    mainTabs_ = new QTabWidget(this);
    setCentralWidget(mainTabs_);

    QFont monoFont("Consolas", 10);

    // ── Tab 1: 文法 ──
    mainTabs_->addTab(createGrammarTab(), "文法");

    // ── Tab 2: FIRST/FOLLOW ──
    mainTabs_->addTab(createFirstFollowTab(), "FIRST/FOLLOW");

    // ── Tab 3~5: DFA ──
    dfaTabs_[0] = createDFATab("LR(0) DFA");
    dfaTabs_[1] = createDFATab("LR(1) DFA");
    dfaTabs_[2] = createDFATab("LALR(1) DFA");
    mainTabs_->addTab(dfaTabs_[0].splitter, "LR(0) DFA");
    mainTabs_->addTab(dfaTabs_[1].splitter, "LR(1) DFA");
    mainTabs_->addTab(dfaTabs_[2].splitter, "LALR(1) DFA");

    // ── Tab 6: 分析表 ──
    mainTabs_->addTab(createParsingTableTab(), "LALR(1) 分析表");

    // ── Tab 7: 句子分析 ──
    mainTabs_->addTab(createParseTab(), "句子分析");

    // ── 初始加载 ──
    // 不自动加载，由用户点击"加载文法"按钮触发
    grammarEditor_->setPlainText("A -> ( A ) | a");
}

// ── 文法编辑 Tab ──
QWidget* MainWindow::createGrammarTab() {
    QWidget* w = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(w);

    grammarEditor_ = new QPlainTextEdit;
    grammarEditor_->setFont(QFont("Consolas", 11));
    grammarEditor_->setPlaceholderText(
        "输入文法，每行一个产生式，用 -> 分隔，| 表示分支\n"
        "例:\n"
        "E -> E + T | T\n"
        "T -> n"
    );

    QHBoxLayout* btnLay = new QHBoxLayout;
    QPushButton* loadBtn = new QPushButton("加载文法");
    QPushButton* saveBtn = new QPushButton("另存为...");
    grammarFileLabel_ = new QLabel("未加载");

    btnLay->addWidget(loadBtn);
    btnLay->addWidget(saveBtn);
    btnLay->addWidget(grammarFileLabel_);
    btnLay->addStretch();

    lay->addWidget(new QLabel("文法编辑："));
    lay->addWidget(grammarEditor_);
    lay->addLayout(btnLay);

    // LR(0) / SLR(1) 判定结果显示
    grammarResultLabel_ = new QLabel("点击「加载文法」查看 LR(0) 和 SLR(1) 判定结果");
    grammarResultLabel_->setWordWrap(true);
    grammarResultLabel_->setStyleSheet("background: #f5f5f5; padding: 8px; border-radius: 4px;");
    grammarResultLabel_->setFont(QFont("Consolas", 10));
    lay->addWidget(grammarResultLabel_);

    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadGrammar);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveGrammar);

    return w;
}

// ── FIRST/FOLLOW Tab ──
QWidget* MainWindow::createFirstFollowTab() {
    QWidget* w = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(w);

    firstFollowTable_ = new QTableWidget;
    firstFollowTable_->setColumnCount(3);
    firstFollowTable_->setHorizontalHeaderLabels({"非终结符", "FIRST", "FOLLOW"});
    firstFollowTable_->horizontalHeader()->setStretchLastSection(true);
    firstFollowTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    firstFollowTable_->setFont(QFont("Consolas", 10));
    firstFollowTable_->verticalHeader()->setVisible(false);
    firstFollowTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    lay->addWidget(firstFollowTable_);
    return w;
}

// ── DFA Tab（通用） ──
DFATabWidgets MainWindow::createDFATab(const QString& title) {
    DFATabWidgets tw;

    // ── 上半：SVG 图形（QScrollArea 包裹 QSvgWidget） ──
    tw.svgWidget = new QSvgWidget;
    tw.svgWidget->setMinimumSize(400, 250);
    tw.svgWidget->setStyleSheet("background: white; border: 1px solid #ccc;");

    tw.scrollArea = new QScrollArea;
    tw.scrollArea->setWidget(tw.svgWidget);
    tw.scrollArea->setWidgetResizable(true);
    tw.scrollArea->setAlignment(Qt::AlignCenter);

    // ── 状态标签 ──
    tw.statusLabel = new QLabel("图形就绪后显示在此处");
    tw.statusLabel->setAlignment(Qt::AlignCenter);
    tw.statusLabel->setStyleSheet("color: #888; font-size: 11px; padding: 4px;");

    // ── 上半容器 ──
    QWidget* topWidget = new QWidget;
    QVBoxLayout* topLay = new QVBoxLayout(topWidget);
    topLay->setContentsMargins(0, 0, 0, 0);
    topLay->addWidget(tw.scrollArea);
    topLay->addWidget(tw.statusLabel);

    // ── 下半：表格 ──
    tw.table = new QTableWidget;
    tw.table->setColumnCount(3);
    tw.table->setHorizontalHeaderLabels({"State", "Items", "Transitions"});
    tw.table->horizontalHeader()->setStretchLastSection(true);
    tw.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tw.table->setFont(QFont("Consolas", 10));
    tw.table->verticalHeader()->setVisible(false);
    tw.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    tw.table->setWordWrap(true);

    // ── 分屏器 ──
    tw.splitter = new QSplitter(Qt::Vertical);
    tw.splitter->addWidget(topWidget);
    tw.splitter->addWidget(tw.table);
    tw.splitter->setStretchFactor(0, 3);   // 图形占 3/5
    tw.splitter->setStretchFactor(1, 2);   // 表格占 2/5

    return tw;
}

// ── 分析表 Tab ──
QWidget* MainWindow::createParsingTableTab() {
    QWidget* w = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(w);

    QLabel* actionLabel = new QLabel("ACTION 表：");
    actionTable_ = new QTableWidget;
    actionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    actionTable_->setFont(QFont("Consolas", 10));
    actionTable_->verticalHeader()->setVisible(false);

    QLabel* gotoLabel = new QLabel("GOTO 表：");
    gotoTable_ = new QTableWidget;
    gotoTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    gotoTable_->setFont(QFont("Consolas", 10));
    gotoTable_->verticalHeader()->setVisible(false);

    lay->addWidget(actionLabel);
    lay->addWidget(actionTable_);
    lay->addWidget(gotoLabel);
    lay->addWidget(gotoTable_);
    return w;
}

// ── 句子分析 Tab ──
QWidget* MainWindow::createParseTab() {
    QWidget* w = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(w);

    QHBoxLayout* inputLay = new QHBoxLayout;
    inputLay->addWidget(new QLabel("输入句子:"));
    sentenceEdit_ = new QLineEdit;
    sentenceEdit_->setFont(QFont("Consolas", 11));
    sentenceEdit_->setPlaceholderText("例如: a  或  ( a )");
    parseBtn_ = new QPushButton("分析");
    clearBtn_ = new QPushButton("清除");
    parseResultLabel_ = new QLabel;
    inputLay->addWidget(sentenceEdit_);
    inputLay->addWidget(parseBtn_);
    inputLay->addWidget(clearBtn_);
    inputLay->addWidget(parseResultLabel_);

    traceTable_ = new QTableWidget;
    traceTable_->setColumnCount(5);
    traceTable_->setHorizontalHeaderLabels({"Step", "State Stack", "Symbol Stack",
                                            "Remaining Input", "Action"});
    traceTable_->horizontalHeader()->setStretchLastSection(true);
    traceTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    traceTable_->setFont(QFont("Consolas", 10));
    traceTable_->verticalHeader()->setVisible(false);
    traceTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    lay->addLayout(inputLay);
    lay->addWidget(traceTable_);

    connect(parseBtn_, &QPushButton::clicked, this, &MainWindow::onParseSentence);
    connect(clearBtn_, &QPushButton::clicked, this, &MainWindow::onClearTrace);
    connect(sentenceEdit_, &QLineEdit::returnPressed, this, &MainWindow::onParseSentence);

    return w;
}

// ═══════════════════════════════════════════════
//  文法加载与重建
// ═══════════════════════════════════════════════

void MainWindow::onLoadGrammar() {
    // 从编辑器文本重建
    QString text = grammarEditor_->toPlainText();
    if(text.trimmed().isEmpty()) {
        // 尝试从默认文件加载
        std::ifstream file(currentFilePath_.toStdString());
        if(file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            text = QString::fromStdString(ss.str());
            grammarEditor_->setPlainText(text);
        } else {
            // 填入默认文法
            text = "A -> ( A ) | a";
            grammarEditor_->setPlainText(text);
        }
    }

    // 重建所有
    if(grammar_) {
        delete parser_; parser_ = nullptr;
        delete lalr_;   lalr_ = nullptr;
        delete lr1_;    lr1_ = nullptr;
        delete lr0_;    lr0_ = nullptr;
        delete grammar_; grammar_ = nullptr;
    }
    std::cout << "C: old objects deleted" << std::endl;

    grammar_ = new Grammar;
    std::cout << "D: new Grammar created" << std::endl;
    if(grammar_->loadFromString(text.toStdString()) != 0) {
        std::cout << "E: load failed!" << std::endl;
        QMessageBox::warning(this, "错误", "无法解析文法！");
        return;
    }
    std::cout << "F: grammar loaded OK" << std::endl;
    grammar_->addAugmentedProduction();
    std::cout << "G: augmented" << std::endl;
    grammar_->computeFirst();
    std::cout << "H: FIRST done" << std::endl;
    grammar_->computeFollow();
    std::cout << "I: FOLLOW done" << std::endl;

    rebuildAll();
    built_ = true;
    grammarFileLabel_->setText(QString("已加载: %1 产生式, %2 非终结符, %3 终结符")
        .arg(grammar_->getProductions().size())
        .arg(grammar_->getNoncnt())
        .arg(grammar_->getEndcnt()));

    // SLR(1) 判定结果
    std::string slr1detail;
    bool slr1ok = lr0_->isSLR1(slr1detail);
    if(slr1ok) {
        grammarResultLabel_->setText(
            "<b style='color:green; font-size:14px;'>✔ 该文法是 SLR(1) 文法</b>");
    } else {
        QString text = "<b style='color:red; font-size:14px;'>✘ 该文法不是 SLR(1) 文法</b>";
        text += "<br><br><b>冲突详情：</b><br>";
        text += QString::fromStdString(slr1detail).replace("\n", "<br>");
        grammarResultLabel_->setText(text);
    }

    statusBar()->showMessage(QString("文法已加载，%1 个状态").arg(lr0_->getStates().size()));
}

void MainWindow::onSaveGrammar() {
    QString path = QFileDialog::getSaveFileName(this, "保存文法", "grammar.txt",
                                                "Text files (*.txt);;All (*)");
    if(path.isEmpty()) return;
    QFile f(path);
    if(f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        out << grammarEditor_->toPlainText();
        f.close();
    }
}

void MainWindow::rebuildAll() {
    lr0_ = new LR0DFA(*grammar_);
    lr0_->build();

    lr1_ = new LR1DFA(*grammar_);
    lr1_->build();

    lalr_ = new LALR1DFA(*grammar_, *lr0_);
    lalr_->build();

    parser_ = new LALR1Parser(*grammar_, *lalr_);
    parser_->buildTable();

    built_ = true;

    populateFirstFollow();

    // LR(0) DFA
    populateDFATable(dfaTabs_[0].table, lr0_->getStates(), lr0_->getTransitions(), 0);
    renderDFAGraph(0, lr0_->toDotString());

    // LR(1) DFA
    populateDFATable(dfaTabs_[1].table, lr1_->getStates(), lr1_->getTransitions(), 1);
    renderDFAGraph(1, lr1_->toDotString());

    // LALR(1) DFA
    populateDFATable(dfaTabs_[2].table, lalr_->getLr0States(), lalr_->getLr0Transitions(), 2);
    renderDFAGraph(2, lalr_->toDotString());

    populateActionTable();
    populateGotoTable();

    statusBar()->showMessage("加载完成", 3000);
}

// ── 渲染 DFA 图形 ──
void MainWindow::renderDFAGraph(int tabIndex, const std::string& dotString) {
    if (!dotRenderers_[tabIndex]->isGraphvizAvailable()) {
        dfaTabs_[tabIndex].statusLabel->setText(
            "Graphviz (dot) 未安装。请安装 Graphviz 并确保 dot 在 PATH 中，以启用图形可视化。");
        dfaTabs_[tabIndex].statusLabel->setStyleSheet(
            "color: #c90; font-size: 11px; padding: 4px;");
        return;
    }
    dfaTabs_[tabIndex].statusLabel->setText("正在渲染图形...");
    dfaTabs_[tabIndex].statusLabel->setStyleSheet(
        "color: #888; font-size: 11px; padding: 4px;");
    dotRenderers_[tabIndex]->render(dotString, tabIndex);
}

// ── SVG 渲染完成 ──
void MainWindow::onSvgReady(const QByteArray &svgData, int tag) {
    if (tag < 0 || tag > 2) return;
    dfaTabs_[tag].svgWidget->load(svgData);
    dfaTabs_[tag].statusLabel->setText("图形就绪");
    dfaTabs_[tag].statusLabel->setStyleSheet(
        "color: #4a4; font-size: 11px; padding: 4px;");
}

// ── 渲染出错 ──
void MainWindow::onRenderError(const QString &error, int tag) {
    if (tag < 0 || tag > 2) return;
    dfaTabs_[tag].statusLabel->setText("渲染失败: " + error);
    dfaTabs_[tag].statusLabel->setStyleSheet(
        "color: #c33; font-size: 11px; padding: 4px;");
}


// ═══════════════════════════════════════════════
//  数据填充
// ═══════════════════════════════════════════════

void MainWindow::populateFirstFollow() {
    if(!grammar_) return;
    int nNonterm = grammar_->getNoncnt();
    firstFollowTable_->setRowCount(nNonterm);
    firstFollowTable_->setColumnCount(3);

    for(int i = 0; i < nNonterm; i++) {
        // 符号名
        auto* nameItem = new QTableWidgetItem(QString::fromStdString(symName(i)));
        nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        firstFollowTable_->setItem(i, 0, nameItem);

        // FIRST
        std::string firstStr;
        auto firstSet = grammar_->getFirst(i);
        for(auto it = firstSet.begin(); it != firstSet.end(); ++it) {
            if(it != firstSet.begin()) firstStr += ", ";
            if(*it == 100) firstStr += "#";
            else firstStr += symName(*it);
        }
        auto* firstItem = new QTableWidgetItem(QString::fromStdString("{ " + firstStr + " }"));
        firstItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        firstFollowTable_->setItem(i, 1, firstItem);

        // FOLLOW
        std::string followStr;
        auto followSet = grammar_->getFollow(i);
        for(auto it = followSet.begin(); it != followSet.end(); ++it) {
            if(it != followSet.begin()) followStr += ", ";
            if(*it == 100) followStr += "#";
            else if(*it == MaxSetSize) followStr += "$";
            else followStr += symName(*it);
        }
        auto* followItem = new QTableWidgetItem(QString::fromStdString("{ " + followStr + " }"));
        followItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        firstFollowTable_->setItem(i, 2, followItem);
    }

    firstFollowTable_->resizeColumnsToContents();
    firstFollowTable_->setColumnWidth(0, firstFollowTable_->columnWidth(0) + 10);
    firstFollowTable_->setColumnWidth(1, firstFollowTable_->columnWidth(1) + 20);
}

void MainWindow::populateDFATable(QTableWidget* table,
                                  const std::vector<LRItemSet>& states,
                                  const std::vector<TransitionList>& trans,
                                  int showLA)
{
    if(!table || !grammar_) return;
    const auto& prods = grammar_->getProductions();
    int n = (int)states.size();
    table->setRowCount(n);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"State", "Items", "Transitions"});
    for(int si = 0; si < n; si++) {
        // State 编号
        auto* stateItem = new QTableWidgetItem(QString::number(si));
        stateItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        table->setItem(si, 0, stateItem);

        // Items
        std::string itemsStr;
        for(auto& item : states[si]) {
            int r = item.rule, p = item.pos;
            if(showLA == 2) {
                auto& las = lalr_->getLookahead(si, r, p);
                itemsStr += itemStr(prods[r], p, &las) + "\n";
            } else if(showLA == 1) {
                // LR(1): preview from item
                std::set<int> tmp;
                for(int j = 0; j < item.k; j++)
                    tmp.insert(item.preview[j]);
                itemsStr += itemStr(prods[r], p, &tmp) + "\n";
            } else {
                // LR(0): no lookahead
                itemsStr += itemStr(prods[r], p, nullptr) + "\n";
            }
        }
        auto* itemsItem = new QTableWidgetItem(QString::fromStdString(itemsStr));
        itemsItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        table->setItem(si, 1, itemsItem);

        // Transitions
        std::string transStr;
        for(auto& tr : trans[si]) {
            if(!transStr.empty()) transStr += ", ";
            transStr += symName(tr.first) + " → " + std::to_string(tr.second);
        }
        auto* transItem = new QTableWidgetItem(QString::fromStdString(transStr));
        transItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        table->setItem(si, 2, transItem);
    }

    table->resizeColumnsToContents();
    table->setColumnWidth(0, 60);
    table->setColumnWidth(1, table->columnWidth(1) + 40);
    table->setColumnWidth(2, table->columnWidth(2) + 20);
}

void MainWindow::populateActionTable() {
    if(!parser_ || !grammar_) return;

    const auto& terms = parser_->getTerminals();
    int nStates = (int)lalr_->getLr0States().size();

    actionTable_->setColumnCount((int)terms.size() + 1);
    QStringList headers;
    headers << "State";
    for(int t : terms)
        headers << QString::fromStdString(parser_->symName(t));
    actionTable_->setHorizontalHeaderLabels(headers);
    actionTable_->setRowCount(nStates);

    for(int si = 0; si < nStates; si++) {
        auto* siItem = new QTableWidgetItem(QString::number(si));
        siItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        actionTable_->setItem(si, 0, siItem);

        for(size_t j = 0; j < terms.size(); j++) {
            const auto& acts = parser_->getActionTable()[si];
            auto it = acts.find(terms[j]);
            QString cellText = (it != acts.end())
                ? QString::fromStdString(it->second) : "";
            auto* cell = new QTableWidgetItem(cellText);
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            // 颜色标记
            if(it != acts.end()) {
                if(it->second == "acc")
                    cell->setBackground(QColor(100, 200, 100));
                else if(it->second[0] == 's')
                    cell->setBackground(QColor(180, 200, 255));
                else if(it->second[0] == 'r')
                    cell->setBackground(QColor(255, 220, 180));
            }
            actionTable_->setItem(si, (int)j + 1, cell);
        }
    }
    actionTable_->resizeColumnsToContents();
}

void MainWindow::populateGotoTable() {
    if(!parser_ || !grammar_) return;

    const auto& nonterms = parser_->getNonterminals();
    int nStates = (int)lalr_->getLr0States().size();

    gotoTable_->setColumnCount((int)nonterms.size() + 1);
    QStringList headers;
    headers << "State";
    for(int nt : nonterms)
        headers << QString::fromStdString(parser_->symName(nt));
    gotoTable_->setHorizontalHeaderLabels(headers);

    // 只显示有内容的行
    std::vector<int> activeRows;
    for(int si = 0; si < nStates; si++) {
        for(int nt : nonterms) {
            const auto& g = parser_->getGotoTable()[si];
            if(g.find(nt) != g.end()) {
                activeRows.push_back(si);
                break;
            }
        }
    }

    gotoTable_->setRowCount((int)activeRows.size());
    for(size_t ri = 0; ri < activeRows.size(); ri++) {
        int si = activeRows[ri];
        auto* siItem = new QTableWidgetItem(QString::number(si));
        siItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        gotoTable_->setItem((int)ri, 0, siItem);

        for(size_t j = 0; j < nonterms.size(); j++) {
            const auto& g = parser_->getGotoTable()[si];
            auto it = g.find(nonterms[j]);
            QString cellText = (it != g.end())
                ? QString::number(it->second) : "";
            auto* cell = new QTableWidgetItem(cellText);
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            if(it != g.end())
                cell->setBackground(QColor(200, 230, 200));
            gotoTable_->setItem((int)ri, (int)j + 1, cell);
        }
    }
    gotoTable_->resizeColumnsToContents();
}

// ═══════════════════════════════════════════════
//  句子分析
// ═══════════════════════════════════════════════

void MainWindow::onParseSentence() {
    if(!built_ || !parser_) {
        parseResultLabel_->setText("<font color=red>请先加载文法</font>");
        return;
    }

    std::string input = sentenceEdit_->text().toStdString();
    if(input.empty()) {
        parseResultLabel_->setText("<font color=orange>请输入句子</font>");
        return;
    }

    // Tokenize
    auto tokens = parser_->tokenize(input);
    if(tokens.empty()) {
        parseResultLabel_->setText("<font color=red>输入为空</font>");
        return;
    }
    if(tokens.size() == 1 && tokens[0] == -1) {
        parseResultLabel_->setText("<font color=red>包含无法识别的符号</font>");
        return;
    }

    // 解析
    auto result = parser_->parse(tokens);

    // 显示结果
    if(result.accepted) {
        parseResultLabel_->setText("<b style='color:green; font-size:14px;'>✔ ACCEPTED</b>");
    } else {
        parseResultLabel_->setText(QString("<b style='color:red; font-size:14px;'>✘ REJECTED</b> — %1")
            .arg(QString::fromStdString(result.errorMsg)));
    }

    populateTraceTable(result);
}

void MainWindow::onClearTrace() {
    sentenceEdit_->clear();
    traceTable_->setRowCount(0);
    parseResultLabel_->clear();
}

void MainWindow::populateTraceTable(const LALR1Parser::ParseResult& result) {
    int n = (int)result.steps.size();
    traceTable_->setRowCount(n);
    traceTable_->setColumnCount(5);

    for(int i = 0; i < n; i++) {
        const auto& st = result.steps[i];

        // Step
        auto* stepItem = new QTableWidgetItem(QString::number(i));
        stepItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        traceTable_->setItem(i, 0, stepItem);

        // State Stack
        std::string statesStr;
        for(size_t k = 0; k < st.stateStack.size(); k++) {
            if(k > 0) statesStr += " ";
            statesStr += std::to_string(st.stateStack[k]);
        }
        auto* ssItem = new QTableWidgetItem(QString::fromStdString(statesStr));
        ssItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        traceTable_->setItem(i, 1, ssItem);

        // Symbol Stack
        std::string symStr;
        for(size_t k = 0; k < st.symStack.size(); k++) {
            if(st.symStack[k] < 0) continue;
            if(!symStr.empty()) symStr += " ";
            symStr += symName(st.symStack[k]);
        }
        auto* symItem = new QTableWidgetItem(QString::fromStdString(symStr));
        symItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        traceTable_->setItem(i, 2, symItem);

        // Remaining Input
        std::string inputStr;
        for(int k = st.nextInputIdx; k < (int)result.tokens.size(); k++) {
            if(!inputStr.empty()) inputStr += " ";
            inputStr += symName(result.tokens[k]);
        }
        auto* inpItem = new QTableWidgetItem(QString::fromStdString(inputStr));
        inpItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        traceTable_->setItem(i, 3, inpItem);

        // Action
        auto* actItem = new QTableWidgetItem(QString::fromStdString(st.action));
        actItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        // 颜色标记
        if(st.action == "Accept") {
            actItem->setBackground(QColor(100, 200, 100));
        } else if(st.action.find("ERROR") != std::string::npos) {
            actItem->setBackground(QColor(255, 100, 100));
        } else if(st.action.find("Shift") != std::string::npos) {
            actItem->setBackground(QColor(180, 200, 255));
        } else if(st.action.find("Reduce") != std::string::npos) {
            actItem->setBackground(QColor(255, 220, 180));
        }
        traceTable_->setItem(i, 4, actItem);
    }

    traceTable_->resizeColumnsToContents();
    traceTable_->setColumnWidth(0, 50);
    traceTable_->setColumnWidth(1, traceTable_->columnWidth(1) + 20);
    traceTable_->setColumnWidth(2, traceTable_->columnWidth(2) + 20);
    traceTable_->setColumnWidth(3, traceTable_->columnWidth(3) + 20);
}
