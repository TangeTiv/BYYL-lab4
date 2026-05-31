#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#define MaxProLength 20
#define MaxSetSize 200
#define MaxGramSize 100
#define Gap 100
using namespace std;

struct production
{
    int left; // 
    int right[MaxProLength];
    int length;
};

vector<string> signset(MaxSetSize+1); // 符号集合
int endcnt = 0; // 终结符数目
int noncnt = 0; // 非终结符号数目
int START = -1; // 开始符
vector <production> grammal; // 产生式集合
struct Item
{
    int rule; // 生成式
    int pos; // . 的位置
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
}

