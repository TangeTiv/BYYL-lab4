#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#include <set>
#include <map>
#include <iterator>
#include <algorithm>
#define MaxProLength 20
#define MaxSetSize 200
#define MaxGramSize 100
#define Gap 100
#define MaxK  5
using namespace std;

struct production
{
    int left; // 
    int right[MaxProLength];
    int length;
};

vector<string> signset(MaxSetSize+1); // 符号集合
int endcnt = 2; // 终结符数目（含预定义的 # 和 $）
int noncnt = 0; // 非终结符号数目
int START = 0; // 开始符
vector <production> grammal; // 产生式集合


struct LRItem
{
    int rule; // 生成式
    int pos; // . 的位置
    int preview[MaxK];
    int k; 
};

struct state
{
    int no;
    vector<LRItem> rules;
};

struct edge
{
    int no; // 边的序号
    int signno;
};

// LR分析表，后续构建
vector<vector<int>> LR0_table;
int tbcnt = 0;
vector<string> SplitStr(string line, char dep = ' ')
{
    vector<string> tokens;
    string token;
    istringstream str(line);
    while(getline(str, token, dep))
    {
        if (!token.empty() && token != " ") 
        {
            tokens.push_back(token);
        }
    }
    return tokens;
}

int findpos(vector<string>tokens, string str)
{
    int size = tokens.size();
    for(int i = 0; i < size; i++)
    {
        if(str == tokens[i])
        {
            return i;
        }
    }
    return -1;
}
int saveinfo(string file)
{
    // 初始化预定义符号： #(句子结束) 和 $(文法结束)
    signset[Gap] = "#";
    signset[MaxSetSize] = "$";

    string line;
    vector<string> lines;
    ifstream input(file);
    while(getline(input,line))
    {
        lines.push_back(line);
    }

    // 第一遍：收集所有唯一的非终结符
    for(auto it : lines)
    {
        vector<string> tokens = SplitStr(it);
        if(tokens.empty()) continue;
        string nonend = tokens[0];
        if(findpos(signset, nonend) == -1)
        {
            signset[noncnt++] = nonend;
        }
    }
    if(noncnt > 0) START = 0;

    // 第二遍：构建产生式
    for(auto it : lines)
    {
        vector<string> tokens = SplitStr(it);
        if(tokens.size() < 3) continue;  // 跳过格式错误的行
        int left = findpos(signset, tokens[0]);
        int size = tokens.size();

        int i = 2;  // 跳过 "->"
        while(i < size)
        {
            production temp;
            temp.left = left;
            int pos = 0;  // right[] 下标计数器

            // 处理一个产生式（遇到 "|" 或行尾结束）
            while(i < size && tokens[i] != "|")
            {
                int n = findpos(signset, tokens[i]);
                if(n == -1)
                {
                    n = Gap + endcnt;
                    if(n < MaxSetSize)
                    {
                        signset[n] = tokens[i];
                    }
                    endcnt++;
                }
                temp.right[pos++] = n;
                i++;
            }
            temp.length = pos;
            grammal.push_back(temp);
            i++;  // 跳过 "|"
        }
    }
    return 0;
}

vector <set<int>> Firstset;
vector <set<int>> Followsets;
set <int> First(int n)
{
    if(n < Gap)
    {
        return Firstset[n];
    }
    set<int> tmp;
    tmp.insert(n);
    return tmp;
}
set <int> Follow(int n)
{
    if(n < Gap)
    {
        return Followsets[n];
    }
    set<int> tmp;
    tmp.insert(n);
    return tmp;
}
int genFollow()
{
    Followsets.resize(noncnt);
    Followsets[START].insert(MaxSetSize);   // $ 加入开始符的 FOLLOW

    int flag;
    do
    {
        flag = 0;
        int size = grammal.size();
        for(int i = 0; i < size; i++)
        {
            int A = grammal[i].left;            // 产生式左部
            int len = grammal[i].length;

            for(int j = 0; j < len; j++)
            {
                int Xj = grammal[i].right[j];
                if(Xj >= Gap) continue;          // 终结符没有 FOLLOW 集

                // 计算 First(β)，其中 β = X_{j+1} ... X_{len-1}
                set<int> firstOfBeta;
                bool allBetaNullable = true;

                for(int k = j + 1; k < len; k++)
                {
                    int sym = grammal[i].right[k];
                    set<int> fk = First(sym);

                    for(int x : fk)
                    {
                        if(x != 100)             // 排除 ε
                            firstOfBeta.insert(x);
                    }

                    if(fk.find(100) == fk.end()) // 不可空 → β 不含 ε
                    {
                        allBetaNullable = false;
                        break;
                    }
                }

                // 将 First(β) - {ε} 加入 Follow(Xj)
                for(int x : firstOfBeta)
                {
                    if(Followsets[Xj].find(x) == Followsets[Xj].end())
                    {
                        Followsets[Xj].insert(x);
                        flag = 1;
                    }
                }

                // 如果 ε ∈ First(β) 或 Xj 是最后一个符号
                if(allBetaNullable || j == len - 1)
                {
                    for(int x : Followsets[A])
                    {
                        if(Followsets[Xj].find(x) == Followsets[Xj].end())
                        {
                            Followsets[Xj].insert(x);
                            flag = 1;
                        }
                    }
                }
            }
        }
    } while(flag == 1);

    return 0;
}
int genFirst()
{
    // Firstset 按非终结符编号索引，而不是按产生式编号
    Firstset.resize(noncnt);

    int flag;
    do
    {
        flag = 0;
        int size = grammal.size();
        for(int i = 0; i < size; i++)
        {
            int left = grammal[i].left;         // 产生式左部（非终结符下标）
            int len = grammal[i].length;
            bool cont = true;                   // 当前所有符号都可空 → 继续

            for(int k = 0; cont && k < len; k++)
            {
                int n = grammal[i].right[k];
                set<int> A = First(n);          // 符号 n 的 FIRST 集

                // 将 A 中除 ε(100) 外的元素加入 Firstset[left]
                for(int x : A)
                {
                    if(x != 100)
                    {
                        if(Firstset[left].find(x) == Firstset[left].end())
                        {
                            Firstset[left].insert(x);
                            flag = 1;           // 有变化，需要继续迭代
                        }
                    }
                }

                // 如果 A 不含 100（不可空），停止往后看
                if(A.find(100) == A.end())
                {
                    cont = false;
                }
                // 否则（可空）→ cont 保持 true，继续看下一个符号
            }

            // 所有符号都可空 → 将 100(ε) 加入 FIRST
            if(cont && Firstset[left].find(100) == Firstset[left].end())
            {
                Firstset[left].insert(100);
                flag = 1;
            }
        }
    } while(flag == 1);

    return 0;
}

// ────────── LR(0) DFA 构建 ──────────

// LRItem 比较器，用于 set 去重（兼容 LR(0) k=0 和 LR(1) k>=1）
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

struct LALRItemCompare {
    bool operator()(const LRItem& a, const LRItem& b) const {
        if(a.rule != b.rule) return a.rule < b.rule;
        if(a.pos  != b.pos)  return a.pos  < b.pos;
        
        return false;
    }
};

// LR(0) DFA 结果（供 LALR(1) 先行符号传播算法使用）
vector<set<LRItem, LRItemCompare>> lr0States;
vector<vector<pair<int,int>>> lr0Transitions;

// 判断两个 LRItem 集合是否相等（含 preview 比较）
bool itemSetsEqual(const set<LRItem, LRItemCompare>& a,
                   const set<LRItem, LRItemCompare>& b) {
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

// Closure 闭包：反复展开 ·后的非终结符
set<LRItem, LRItemCompare> closure(set<LRItem, LRItemCompare> I) {
    set<LRItem, LRItemCompare> result = I;
    bool changed = true;
    while(changed) {
        changed = false;
        auto snapshot = result;          // 遍历快照，避免迭代器失效
        for(auto item : snapshot) {
            int r = item.rule, p = item.pos;
            if(p >= grammal[r].length) continue;   // ·在末尾，跳过

            int B = grammal[r].right[p];            // ·后的符号
            if(B >= Gap) continue;                  // 终结符不展开

            // B 是非终结符 → 加入所有 B → ·γ
            for(int pi = 0; pi < grammal.size(); pi++) {
                if(grammal[pi].left == B) {
                    LRItem newItem;
                    newItem.rule = pi;
                    newItem.pos  = 0;
                    newItem.k    = 0;               // LR(0) 不用 preview
                    if(result.find(newItem) == result.end()) {
                        result.insert(newItem);
                        changed = true;
                    }
                }
            }
        }
    }
    return result;
}

set<LRItem, LRItemCompare> closure_LR1(set<LRItem, LRItemCompare> I) {
    set<LRItem, LRItemCompare> result = I;
    bool changed = true;
    while(changed) {
        changed = false;
        auto snapshot = result;          // 遍历快照，避免迭代器失效
        for(auto item : snapshot) {
            int r = item.rule, p = item.pos;
            if(p >= grammal[r].length) continue;   // ·在末尾，跳过

            int B = grammal[r].right[p];            // ·后的符号
            if(B >= Gap) continue;                  // 终结符不展开
            set<int> firstOfBeta;
            bool allBetaNullable = true;

            for(int k = p + 1; k < grammal[r].length; k++)
            {
                int sym = grammal[r].right[k];
                set<int> fk = First(sym);

                for(int x : fk)
                {
                    if(x != 100)             // 排除 ε
                        firstOfBeta.insert(x);
                }

                if(fk.find(100) == fk.end()) // 不可空 → β 不含 ε
                {
                    allBetaNullable = false;
                    break;
                }
            }
            if(allBetaNullable == true) {
                // β 全部可空 → 加入原 item 的 preview
                for(int kp = 0; kp < item.k; kp++)
                    firstOfBeta.insert(item.preview[kp]);
            }
            
            // B 是非终结符 → 加入所有 B → ·γ
            for(int pi = 0; pi < grammal.size(); pi++) {
                if(grammal[pi].left == B) {
                    LRItem newItem;
                    newItem.rule = pi;
                    newItem.pos  = 0;
                    newItem.k    = 1;               // LR(0) 不用 preview
                    for(auto it: firstOfBeta)
                    {
                        newItem.preview[0] = it;                        
                        if(result.find(newItem) == result.end()) {
                            result.insert(newItem);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    return result;
}

// LR(0) GoTo：不处理 preview
set<LRItem, LRItemCompare> goTo_0(set<LRItem, LRItemCompare> I, int X) {
    set<LRItem, LRItemCompare> J;
    for(auto item : I) {
        int r = item.rule, p = item.pos;
        if(p < grammal[r].length && grammal[r].right[p] == X) {
            LRItem newItem;
            newItem.rule = r;
            newItem.pos  = p + 1;
            newItem.k    = 0;
            J.insert(newItem);
        }
    }
    return closure(J);
}

// LR(1) GoTo：保留 preview，调用 closure_LR1
set<LRItem, LRItemCompare> goTo_LR1(set<LRItem, LRItemCompare> I, int X) {
    set<LRItem, LRItemCompare> J;
    for(auto item : I) {
        int r = item.rule, p = item.pos;
        if(p < grammal[r].length && grammal[r].right[p] == X) {
            LRItem newItem;
            newItem.rule = r;
            newItem.pos  = p + 1;
            newItem.k    = item.k;                 // 保留原 k
            for(int i = 0; i < item.k; i++)
                newItem.preview[i] = item.preview[i];  // 复制 preview
            J.insert(newItem);
        }
    }
    return closure_LR1(J);
}

// 增广产生式：S' → 原开始符 $
void addAugmentedProduction() {
    production aug;
    aug.left = noncnt;
    signset[noncnt++] = "S'";
    aug.right[0] = START;
    aug.right[1] = MaxSetSize;    // $
    aug.length = 2;
    grammal.push_back(aug);
}

// 前向声明
bool JudgeLR0(vector<set<LRItem, LRItemCompare>> c);

// 构建整个 LR(0) DFA（项目集规范族）
void buildLR0DFA() {
    // 初始 Item：S' → ·S $ （k=0，无 preview）
    LRItem startItem;
    startItem.rule = grammal.size() - 1;
    startItem.pos  = 0;
    startItem.k    = 0;
    set<LRItem, LRItemCompare> startSet;
    startSet.insert(startItem);
    startSet = closure(startSet);

    // 状态集合
    vector<set<LRItem, LRItemCompare>> c = {startSet};
    vector<vector<pair<int,int>>> transitions = {{}};
    vector<int> queue = {0};
    int edgeCnt = 0;

    for(int idx = 0; idx < queue.size(); idx++) {
        int cur = queue[idx];
        set<int> symbols;
        for(auto item : c[cur]) {
            int r = item.rule, p = item.pos;
            if(p < grammal[r].length)
                symbols.insert(grammal[r].right[p]);
        }
        for(int X : symbols) {
            auto newSet = goTo_0(c[cur], X);   // LR(0) GoTo
            if(newSet.empty()) continue;
            int found = -1;
            for(int si = 0; si < c.size(); si++) {
                if(itemSetsEqual(c[si], newSet)) { found = si; break; }
            }
            if(found == -1) {
                found = c.size();
                c.push_back(newSet);
                queue.push_back(found);
                transitions.push_back({});
            }
            transitions[cur].push_back({X, found});
            edgeCnt++;
        }
    }

    // === Print DFA ===
    cout << "\n========== LR(0) DFA ==========" << endl;
    cout << c.size() << " states, " << edgeCnt << " edges" << endl << endl;
    for(int si = 0; si < c.size(); si++) {
        cout << "--- State " << si << " ---" << endl;
        for(auto item : c[si]) {
            cout << "  " << signset[grammal[item.rule].left] << " ->";
            for(int k = 0; k < grammal[item.rule].length; k++) {
                if(k == item.pos) cout << " .";
                cout << " " << signset[grammal[item.rule].right[k]];
            }
            if(item.pos == grammal[item.rule].length) cout << " .";
            cout << "  (Rule " << item.rule << ")" << endl;
        }
        for(auto tr : transitions[si]) {
            cout << "  --" << signset[tr.first] << "--> State " << tr.second << endl;
        }
        if(!transitions[si].empty()) cout << endl;
    }
    cout << "================================" << endl;

    if(JudgeLR0(c))
        cout << "This grammar is LR(0)." << endl;
    else
        cout << "This grammar is NOT LR(0)." << endl;

    // === Save DOT ===
    ofstream dotFile("LR0_DFA.dot");
    dotFile << "digraph LR0_DFA {" << endl;
    dotFile << "    rankdir=LR;" << endl;
    dotFile << "    node [shape=box, style=rounded, fontname=\"Consolas\"];" << endl;
    for(int si = 0; si < c.size(); si++) {
        dotFile << "    " << si << " [label=\"State " << si;
        for(auto item : c[si]) {
            dotFile << "\\n" << signset[grammal[item.rule].left] << " ->";
            for(int k = 0; k < grammal[item.rule].length; k++) {
                if(k == item.pos) dotFile << " .";
                dotFile << " " << signset[grammal[item.rule].right[k]];
            }
            if(item.pos == grammal[item.rule].length) dotFile << " .";
        }
        dotFile << "\"];" << endl;
    }
    for(int si = 0; si < c.size(); si++)
        for(auto tr : transitions[si])
            dotFile << "    " << si << " -> " << tr.second
                    << " [label=\"" << signset[tr.first] << "\"];" << endl;
    dotFile << "}" << endl;
    dotFile.close();
    cout << "LR(0) DFA saved to LR0_DFA.dot" << endl;

    // 保存 LR(0) DFA 结果供 LALR(1) 传播算法使用
    lr0States = c;
    lr0Transitions = transitions;
}

// 构建 LR(1) DFA（项目集规范族）
void buildLR1DFA() {
    // 初始 Item：S' → ·S $ ，preview = $
    LRItem startItem;
    startItem.rule = grammal.size() - 1;
    startItem.pos  = 0;
    startItem.k    = 1;
    startItem.preview[0] = MaxSetSize;
    set<LRItem, LRItemCompare> startSet;
    startSet.insert(startItem);
    startSet = closure_LR1(startSet);          // LR(1) 闭包

    vector<set<LRItem, LRItemCompare>> c = {startSet};
    vector<vector<pair<int,int>>> transitions = {{}};
    vector<int> queue = {0};
    int edgeCnt = 0;

    for(int idx = 0; idx < queue.size(); idx++) {
        int cur = queue[idx];
        set<int> symbols;
        for(auto item : c[cur]) {
            int r = item.rule, p = item.pos;
            if(p < grammal[r].length)
                symbols.insert(grammal[r].right[p]);
        }
        for(int X : symbols) {
            auto newSet = goTo_LR1(c[cur], X);  // LR(1) GoTo
            if(newSet.empty()) continue;
            int found = -1;
            for(int si = 0; si < c.size(); si++) {
                if(itemSetsEqual(c[si], newSet)) { found = si; break; }
            }
            if(found == -1) {
                found = c.size();
                c.push_back(newSet);
                queue.push_back(found);
                transitions.push_back({});
            }
            transitions[cur].push_back({X, found});
            edgeCnt++;
        }
    }

    // === Print DFA ===
    cout << "\n========== LR(1) DFA ==========" << endl;
    cout << c.size() << " states, " << edgeCnt << " edges" << endl << endl;
    for(int si = 0; si < c.size(); si++) {
        cout << "--- State " << si << " ---" << endl;
        for(auto item : c[si]) {
            cout << "  " << signset[grammal[item.rule].left] << " ->";
            for(int k = 0; k < grammal[item.rule].length; k++) {
                if(k == item.pos) cout << " .";
                cout << " " << signset[grammal[item.rule].right[k]];
            }
            if(item.pos == grammal[item.rule].length) cout << " .";
            cout << "  (Rule " << item.rule;
            // 显示 LR(1) preview
            if(item.k > 0) {
                cout << ", preview=[";
                for(int i = 0; i < item.k; i++) {
                    if(i > 0) cout << " ";
                    if(item.preview[i] == 100) cout << "#";
                    else if(item.preview[i] == MaxSetSize) cout << "$";
                    else cout << signset[item.preview[i]];
                }
                cout << "]";
            }
            cout << ")" << endl;
        }
        for(auto tr : transitions[si]) {
            cout << "  --" << signset[tr.first] << "--> State " << tr.second << endl;
        }
        if(!transitions[si].empty()) cout << endl;
    }
    cout << "================================" << endl;

    // === Save DOT ===
    ofstream dotFile("LR1_DFA.dot");
    dotFile << "digraph LR1_DFA {" << endl;
    dotFile << "    rankdir=LR;" << endl;
    dotFile << "    node [shape=box, style=rounded, fontname=\"Consolas\"];" << endl;
    for(int si = 0; si < c.size(); si++) {
        dotFile << "    " << si << " [label=\"State " << si;
        for(auto item : c[si]) {
            dotFile << "\\n" << signset[grammal[item.rule].left] << " ->";
            for(int k = 0; k < grammal[item.rule].length; k++) {
                if(k == item.pos) dotFile << " .";
                dotFile << " " << signset[grammal[item.rule].right[k]];
            }
            if(item.pos == grammal[item.rule].length) dotFile << " .";
            if(item.k > 0) {
                dotFile << "  [";
                for(int i = 0; i < item.k; i++) {
                    if(item.preview[i] == 100) dotFile << "#";
                    else if(item.preview[i] == MaxSetSize) dotFile << "$";
                    else dotFile << signset[item.preview[i]];
                }
                dotFile << "]";
            }
        }
        dotFile << "\"];" << endl;
    }
    for(int si = 0; si < c.size(); si++)
        for(auto tr : transitions[si])
            dotFile << "    " << si << " -> " << tr.second
                    << " [label=\"" << signset[tr.first] << "\"];" << endl;
    dotFile << "}" << endl;
    dotFile.close();
    cout << "LR(1) DFA saved to LR1_DFA.dot" << endl;
}

// ────────── LALR(1) DFA 构建：先行符号传播法 ──────────
// 算法：DeRemer-Pennello lookahead propagation
// 直接从 LR(0) DFA 计算 LALR(1) lookaheads，不经过完整 LR(1) 再合并
void buildLALR1DFA() {
    int n = lr0States.size();

    // ── ① 初始化 lookahead 集合 ──
    // la[state][(rule, pos)] = set<int> of lookahead symbols
    vector<map<pair<int,int>, set<int>>> la(n);
    for(int si = 0; si < n; si++) {
        for(auto& item : lr0States[si]) {
            la[si][{item.rule, item.pos}] = {};
        }
    }

    // ── ② 播种：起始项 S' → ·S$ → lookahead = {$} ──
    int startRule = grammal.size() - 1;
    la[0][{startRule, 0}].insert(MaxSetSize);

    // ── ③ 迭代传播直至收敛 ──
    bool changed;
    do {
        changed = false;

        for(int si = 0; si < n; si++) {
            for(auto& item : lr0States[si]) {
                int r = item.rule, p = item.pos;
                auto& curLA = la[si][{r, p}];
                if(curLA.empty()) continue;

                int len = grammal[r].length;
                if(p >= len) continue;  // 归约项，无可传播

                int X = grammal[r].right[p];  // ·后的符号

                // ── ③a GoTo 传播：沿 DFA 边传下去 ──
                for(auto& tr : lr0Transitions[si]) {
                    if(tr.first != X) continue;
                    int nextState = tr.second;
                    pair<int,int> nextKey = {r, p + 1};
                    if(la[nextState].count(nextKey)) {
                        for(int a : curLA) {
                            if(la[nextState][nextKey].insert(a).second)
                                changed = true;
                        }
                    }
                    break;  // 同一符号只对应一个目标状态
                }

                // ── ③b 闭包传播（·后是非终结符时） ──
                if(X >= Gap) continue;  // 终结符 → 没有闭包

                // 计算 First(β)，β = right[p+1 ... len-1]
                set<int> firstOfBeta;
                bool betaNullable = true;
                for(int k = p + 1; k < len; k++) {
                    int sym = grammal[r].right[k];
                    set<int> fk = First(sym);
                    for(int x : fk)
                        if(x != 100) firstOfBeta.insert(x);
                    if(fk.find(100) == fk.end()) {
                        betaNullable = false;
                        break;
                    }
                }

                // 遍历 X 的所有产生式 X → γ（即状态中对应的闭包项目）
                for(int pi = 0; pi < grammal.size(); pi++) {
                    if(grammal[pi].left != X) continue;
                    pair<int,int> ck = {pi, 0};
                    if(!la[si].count(ck)) continue;

                    // 自发生成：First(β) - {ε}
                    for(int a : firstOfBeta) {
                        if(la[si][ck].insert(a).second)
                            changed = true;
                    }
                    // 传播：β 全部可空时，将当前 lookahead 复制给闭包项目
                    if(betaNullable) {
                        for(int a : curLA) {
                            if(la[si][ck].insert(a).second)
                                changed = true;
                        }
                    }
                }
            }
        }
    } while(changed);

    // ── ④ 输出 LALR(1) DFA ──
    cout << "\n========== LALR(1) DFA =========" << endl;
    cout << n << " states (same core as LR(0))" << endl << endl;
    for(int si = 0; si < n; si++) {
        cout << "--- State " << si << " ---" << endl;
        for(auto& item : lr0States[si]) {
            cout << "  " << signset[grammal[item.rule].left] << " ->";
            for(int k = 0; k < grammal[item.rule].length; k++) {
                if(k == item.pos) cout << " .";
                cout << " " << signset[grammal[item.rule].right[k]];
            }
            if(item.pos == grammal[item.rule].length) cout << " .";
            // 显示 lookahead 集合
            auto& las = la[si][{item.rule, item.pos}];
            if(!las.empty()) {
                cout << "  [";
                bool first = true;
                for(int a : las) {
                    if(!first) cout << " ";
                    first = false;
                    if(a == 100) cout << "#";
                    else if(a == MaxSetSize) cout << "$";
                    else cout << signset[a];
                }
                cout << "]";
            }
            cout << "  (Rule " << item.rule << ")" << endl;
        }
        for(auto& tr : lr0Transitions[si]) {
            cout << "  --" << signset[tr.first] << "--> State " << tr.second << endl;
        }
        if(!lr0Transitions[si].empty()) cout << endl;
    }
    cout << "================================" << endl;

    // ── ⑤ LALR(1) 冲突检测 ──
    bool isLALR = true;
    for(int si = 0; si < n; si++) {
        map<int,bool> hasShift;
        map<int,int> reduceCnt;

        for(auto& item : lr0States[si]) {
            int r = item.rule, p = item.pos;
            if(p < grammal[r].length) {
                int X = grammal[r].right[p];
                if(X >= Gap) hasShift[X] = true;
            } else {
                auto& las = la[si][{r, p}];
                for(int a : las) {
                    // 接受项 S' → S $ . 不参与冲突
                    if(a == MaxSetSize && r == startRule) continue;
                    reduceCnt[a]++;
                }
            }
        }

        for(auto it = hasShift.begin(); it != hasShift.end(); ++it) {
            int sym = it->first;
            if(reduceCnt[sym] > 0) {
                cout << "  SR conflict in State " << si
                     << " on symbol " << signset[sym] << endl;
                isLALR = false;
            }
        }
        for(auto it = reduceCnt.begin(); it != reduceCnt.end(); ++it) {
            int a = it->first;
            int cnt = it->second;
            if(cnt > 1) {
                cout << "  RR conflict in State " << si
                     << " on symbol "
                     << (a == 100 ? "#" : a == MaxSetSize ? "$" : signset[a]) << endl;
                isLALR = false;
            }
        }
    }
    if(isLALR)
        cout << "\nThis grammar is LALR(1)." << endl;
    else
        cout << "\nThis grammar is NOT LALR(1)." << endl;

    // ── ⑥ 保存 DOT 图 ──
    ofstream dotFile("LALR1_DFA.dot");
    dotFile << "digraph LALR1_DFA {" << endl;
    dotFile << "    rankdir=LR;" << endl;
    dotFile << "    node [shape=box, style=rounded, fontname=\"Consolas\"];" << endl;
    for(int si = 0; si < n; si++) {
        dotFile << "    " << si << " [label=\"State " << si;
        for(auto& item : lr0States[si]) {
            dotFile << "\\n" << signset[grammal[item.rule].left] << " ->";
            for(int k = 0; k < grammal[item.rule].length; k++) {
                if(k == item.pos) dotFile << " .";
                dotFile << " " << signset[grammal[item.rule].right[k]];
            }
            if(item.pos == grammal[item.rule].length) dotFile << " .";
            auto& las = la[si][{item.rule, item.pos}];
            if(!las.empty()) {
                dotFile << "  [";
                bool first = true;
                for(int a : las) {
                    if(!first) dotFile << " ";
                    first = false;
                    if(a == 100) dotFile << "#";
                    else if(a == MaxSetSize) dotFile << "$";
                    else dotFile << signset[a];
                }
                dotFile << "]";
            }
        }
        dotFile << "\"];" << endl;
    }
    for(int si = 0; si < n; si++)
        for(auto& tr : lr0Transitions[si])
            dotFile << "    " << si << " -> " << tr.second
                    << " [label=\"" << signset[tr.first] << "\"];" << endl;
    dotFile << "}" << endl;
    dotFile.close();
    cout << "LALR(1) DFA saved to LALR1_DFA.dot" << endl;
}
bool JudgeLR0(vector<set<LRItem, LRItemCompare>> c)
{
    for(auto& state : c)
    {
        bool hasShift = false;
        bool hasReduce = false;
        int reduceCnt = 0;

        for(auto item : state)
        {
            int r = item.rule;
            int p = item.pos;

            if(p == grammal[r].length) {
                // 归约项：点在末尾
                hasReduce = true;
                reduceCnt++;
            }
            else {
                int X = grammal[r].right[p];
                if(X >= Gap) {     // ·后是终结符 → 移进
                    hasShift = true;
                }
                // ·后是非终结符 → GoTo 转移，不参与冲突判断
            }
        }

        // SR 冲突：既有移进又有归约
        if(hasShift && hasReduce) return false;
        // RR 冲突：有两条及以上归约
        if(reduceCnt > 1) return false;
    }
    return true;
}



int main()
{
    saveinfo("testdata.txt");

    // 添加增广产生式 S' → S $
    addAugmentedProduction();

    // 计算 FIRST / FOLLOW
    genFirst();
    genFollow();

    // Print productions
    cout << "----- Productions -----" << endl;
    for(int i = 0; i < grammal.size(); i++)
    {
        cout << i << ": " << signset[grammal[i].left] << "->";
        for(int k = 0; k < grammal[i].length; k++)
            cout << " " << signset[grammal[i].right[k]];
        cout << endl;
    }

    // Print FIRST sets
    cout << "\n========== FIRST Sets ==========" << endl;
    for(int i = 0; i < noncnt; i++)
    {
        cout << "FIRST(" << signset[i] << ") = { ";
        for(auto it = Firstset[i].begin(); it != Firstset[i].end(); it++)
        {
            if(*it == 100) cout << "#";
            else cout << signset[*it];
            if(next(it) != Firstset[i].end()) cout << ", ";
        }
        cout << " }" << endl;
    }
    cout << "================================" << endl;

    // Print FOLLOW sets
    cout << "\n========== FOLLOW Sets =========" << endl;
    for(int i = 0; i < noncnt; i++)
    {
        cout << "FOLLOW(" << signset[i] << ") = { ";
        for(auto it = Followsets[i].begin(); it != Followsets[i].end(); it++)
        {
            if(*it == 100) cout << "#";
            else if(*it == MaxSetSize) cout << "$";
            else cout << signset[*it];
            if(next(it) != Followsets[i].end()) cout << ", ";
        }
        cout << " }" << endl;
    }
    cout << "================================" << endl;

    // 构建 LR(0) DFA
    buildLR0DFA();

    // 构建 LR(1) DFA
    buildLR1DFA();

    // 构建 LALR(1) DFA（先行符号传播法）
    buildLALR1DFA();
}

