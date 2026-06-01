#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#include <set>
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


int main()
{
    saveinfo("testdata.txt");
    for(auto it : grammal)
    {
        cout<<it.left<<" -> ";
        for(int i = 0; i < it.length; i++)
        {
            cout<<" "<<it.right[i];
        }
        cout<<endl;
    }

    // 测试 genFirst
    genFirst();
    cout << "\n========== FIRST 集 ==========" << endl;
    for(int i = 0; i < noncnt; i++)
    {
        cout << "FIRST(" << signset[i] << ") = { ";
        for(auto it = Firstset[i].begin(); it != Firstset[i].end(); it++)
        {
            if(*it == 100)
                cout << "#";
            else
                cout << signset[*it];
            if(next(it) != Firstset[i].end())
                cout << ", ";
        }
        cout << " }" << endl;
    }
    cout << "==============================" << endl;

    // 测试 genFollow
    genFollow();
    cout << "\n========== FOLLOW 集 =========" << endl;
    for(int i = 0; i < noncnt; i++)
    {
        cout << "FOLLOW(" << signset[i] << ") = { ";
        for(auto it = Followsets[i].begin(); it != Followsets[i].end(); it++)
        {
            if(*it == 100)
                cout << "#";
            else if(*it == MaxSetSize)
                cout << "$";
            else
                cout << signset[*it];
            if(next(it) != Followsets[i].end())
                cout << ", ";
        }
        cout << " }" << endl;
    }
    cout << "==============================" << endl;
}

