#include "Grammar.h"

Grammar::Grammar()
    : signset_(MaxSetSize + 1), endcnt_(2), noncnt_(0), START_(0)
{
    // 初始化预定义符号：#(句子结束) 和 $(文法结束)
    signset_[Gap] = "#";
    signset_[MaxSetSize] = "$";
}

std::vector<std::string> Grammar::split(const std::string& line, char dep) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream str(line);
    while(getline(str, token, dep)) {
        if(!token.empty() && token != " ")
            tokens.push_back(token);
    }
    return tokens;
}

int Grammar::findpos(const std::string& str) const {
    for(int i = 0; i < noncnt_ + endcnt_; i++) {
        if(i < noncnt_ || (i >= Gap && i <= MaxSetSize)) {
            if(signset_[i] == str)
                return i;
        }
    }
    // 在终结符区搜索
    for(int i = Gap; i < MaxSetSize; i++) {
        if(signset_[i] == str)
            return i;
    }
    return -1;
}

int Grammar::loadFromFile(const std::string& file) {
    std::ifstream input(file);
    if(!input.is_open()) {
        std::cerr << "Cannot open file: " << file << std::endl;
        return -1;
    }
    std::string content((std::istreambuf_iterator<char>(input)),
                         std::istreambuf_iterator<char>());
    input.close();
    return loadFromString(content);
}

int Grammar::loadFromString(const std::string& content) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream stream(content);
    while(std::getline(stream, line))
        lines.push_back(line);

    // 第一遍：收集所有唯一的非终结符
    for(auto& it : lines) {
        auto tokens = split(it);
        if(tokens.empty()) continue;
        std::string nonend = tokens[0];
        if(findpos(nonend) == -1)
            signset_[noncnt_++] = nonend;
    }
    if(noncnt_ > 0) START_ = 0;

    // 第二遍：构建产生式
    for(auto& it : lines) {
        auto tokens = split(it);
        if(tokens.size() < 3) continue;
        int left = findpos(tokens[0]);
        int size = tokens.size();
        int i = 2;  // 跳过 "->"

        while(i < size) {
            production temp;
            temp.left = left;
            int pos = 0;

            while(i < size && tokens[i] != "|") {
                int n = findpos(tokens[i]);
                if(n == -1) {
                    n = Gap + endcnt_;
                    if(n < MaxSetSize)
                        signset_[n] = tokens[i];
                    endcnt_++;
                }
                temp.right[pos++] = n;
                i++;
            }
            temp.length = pos;
            grammal_.push_back(temp);
            i++;  // 跳过 "|"
        }
    }
    return 0;
}

// ============ FIRST 集计算 ============

std::set<int> Grammar::getFirst(int n) const {
    if(n < Gap)
        return Firstset_[n];
    std::set<int> tmp;
    tmp.insert(n);
    return tmp;
}

int Grammar::computeFirst() {
    Firstset_.resize(noncnt_);

    int flag;
    do {
        flag = 0;
        for(int i = 0; i < (int)grammal_.size(); i++) {
            int left = grammal_[i].left;
            int len = grammal_[i].length;
            bool cont = true;

            for(int k = 0; cont && k < len; k++) {
                int n = grammal_[i].right[k];
                std::set<int> A = getFirst(n);

                for(int x : A) {
                    if(x != 100) {
                        if(Firstset_[left].find(x) == Firstset_[left].end()) {
                            Firstset_[left].insert(x);
                            flag = 1;
                        }
                    }
                }

                if(A.find(100) == A.end())
                    cont = false;
            }

            if(cont && Firstset_[left].find(100) == Firstset_[left].end()) {
                Firstset_[left].insert(100);
                flag = 1;
            }
        }
    } while(flag == 1);

    return 0;
}

// ============ FOLLOW 集计算 ============

std::set<int> Grammar::getFollow(int n) const {
    if(n < Gap)
        return Followsets_[n];
    std::set<int> tmp;
    tmp.insert(n);
    return tmp;
}

int Grammar::computeFollow() {
    Followsets_.resize(noncnt_);
    Followsets_[START_].insert(MaxSetSize);   // $ 加入开始符的 FOLLOW

    int flag;
    do {
        flag = 0;
        for(int i = 0; i < (int)grammal_.size(); i++) {
            int A = grammal_[i].left;
            int len = grammal_[i].length;

            for(int j = 0; j < len; j++) {
                int Xj = grammal_[i].right[j];
                if(Xj >= Gap) continue;

                std::set<int> firstOfBeta;
                bool allBetaNullable = true;

                for(int k = j + 1; k < len; k++) {
                    int sym = grammal_[i].right[k];
                    std::set<int> fk = getFirst(sym);

                    for(int x : fk)
                        if(x != 100)
                            firstOfBeta.insert(x);

                    if(fk.find(100) == fk.end()) {
                        allBetaNullable = false;
                        break;
                    }
                }

                for(int x : firstOfBeta) {
                    if(Followsets_[Xj].find(x) == Followsets_[Xj].end()) {
                        Followsets_[Xj].insert(x);
                        flag = 1;
                    }
                }

                if(allBetaNullable || j == len - 1) {
                    for(int x : Followsets_[A]) {
                        if(Followsets_[Xj].find(x) == Followsets_[Xj].end()) {
                            Followsets_[Xj].insert(x);
                            flag = 1;
                        }
                    }
                }
            }
        }
    } while(flag == 1);

    return 0;
}

// ============ 符号查找（公有） ============

int Grammar::lookupSymbol(const std::string& s) const {
    return findpos(s);
}

// ============ 增广产生式 ============

void Grammar::addAugmentedProduction() {
    production aug;
    aug.left = noncnt_;
    signset_[noncnt_++] = "S'";
    aug.right[0] = START_;
    aug.right[1] = MaxSetSize;    // $
    aug.length = 2;
    grammal_.push_back(aug);
}

// ============ 输出 ============

void Grammar::printProductions(std::ostream& out) const {
    for(int i = 0; i < (int)grammal_.size(); i++) {
        out << i << ": " << signset_[grammal_[i].left] << "->";
        for(int k = 0; k < grammal_[i].length; k++)
            out << " " << signset_[grammal_[i].right[k]];
        out << std::endl;
    }
}

void Grammar::printFirstSets(std::ostream& out) const {
    for(int i = 0; i < noncnt_; i++) {
        out << "FIRST(" << signset_[i] << ") = { ";
        for(auto it = Firstset_[i].begin(); it != Firstset_[i].end(); ++it) {
            if(*it == 100) out << "#";
            else out << signset_[*it];
            if(std::next(it) != Firstset_[i].end()) out << ", ";
        }
        out << " }" << std::endl;
    }
}

void Grammar::printFollowSets(std::ostream& out) const {
    for(int i = 0; i < noncnt_; i++) {
        out << "FOLLOW(" << signset_[i] << ") = { ";
        for(auto it = Followsets_[i].begin(); it != Followsets_[i].end(); ++it) {
            if(*it == 100) out << "#";
            else if(*it == MaxSetSize) out << "$";
            else out << signset_[*it];
            if(std::next(it) != Followsets_[i].end()) out << ", ";
        }
        out << " }" << std::endl;
    }
}
