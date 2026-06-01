# 实验4 LALR(1) 语法分析生成器 — 项目进度

## 当前分支 `parse3`

---

## ✅ 已完成功能

### 1. 文法读取 (`saveinfo`)
- [x] 从 `testdata.txt` 读取产生式
- [x] 支持 `|` 分隔的多个产生式
- [x] 自动区分非终结符 / 终结符
- [x] 预定义 `signset[100] = "#"`（ε/句子结束符）
- [x] 预定义 `signset[200] = "$"`（文法结束符）
- [x] 添加增广产生式 `S' → S $`

### 2. FIRST 集计算 (`genFirst`)
- [x] 按非终结符索引
- [x] 迭代至收敛（`do-while(flag)`）
- [x] 正确处理 ε(100) 的传播

### 3. FOLLOW 集计算 (`genFollow`)
- [x] 初始化 `FOLLOW(S) = {$}`
- [x] 迭代至收敛
- [x] 处理可空符号链式传递

### 4. LR(0) DFA 构建
- [x] `closure` — 闭包（LR(0) 版，无 preview）
- [x] `goTo_0` — 状态转移（LR(0) 版）
- [x] `buildLR0DFA` — BFS 构建项目集规范族
- [x] `JudgeLR0` — LR(0) 冲突检测（SR / RR）
- [x] 输出 DOT 图文件 `LR0_DFA.dot`

### 5. LR(1) DFA 构建
- [x] `closure_LR1` — 带 preview 传播的闭包
- [x] `goTo_LR1` — 保留并传播 preview 的状态转移
- [x] `buildLR1DFA` — BFS 构建 LR(1) 项目集规范族
- [x] 输出 DOT 图文件 `LR1_DFA.dot`

### 6. 基础工具
- [x] `SplitStr` — 字符串分割
- [x] `findpos` — 符号查找
- [x] `LRItemCompare` — LRItem 比较器（含 preview 比较）
- [x] `itemSetsEqual` — 状态判等（含 preview 比较）

---

## 📊 测试结果（当前文法）

```
exp -> exp addop term | term
addop -> + | -
term -> term mulop factor | factor
mulop -> * | /
factor -> ( exp ) | n | #
```

| 指标 | LR(0) | LR(1) |
|------|-------|-------|
| 状态数 | 18 | 33 |
| 转移边数 | 35 | 60 |
| SR/RR 冲突 | **有**（非 LR(0)） | 待检测 |

---

## ❌ 待实现功能

### 7. LR(0) 分析表构建
- [ ] 根据 DFA 生成 `action` 表（shift / reduce / accept）
- [ ] 根据 DFA 生成 `goto` 表
- [ ] 存入 `LR0_table`（`vector<vector<int>>`）

### 8. LR(1) 分析表构建
- [ ] 根据 LR(1) DFA 生成 LR(1) 分析表
- [ ] 注：LR(1) 表的 action 按 lookahead 区分

### 9. LALR(1) 合并
- [ ] 合并 LR(1) 中相同核心（core）的状态
- [ ] 生成 LALR(1) 分析表

### 10. 语法分析驱动器
- [ ] 输入符号串
- [ ] 利用分析表进行 Shift / Reduce / Accept
- [ ] 输出分析过程（状态栈、符号栈、动作）

### 11. 其他
- [ ] 错误处理（非法符号、语法错误）
- [ ] 测试多个文法
- [ ] 输出语义分析结果（可选）

---

## 🏗️ 代码结构

```
parse1.cpp
├── 数据结构
│   ├── production      — 产生式
│   ├── LRItem           — LR 项目
│   ├── state            — DFA 状态
│   └── edge             — 转移边
├── 工具函数
│   ├── SplitStr()       — 字符串分割
│   └── findpos()        — 符号查找
├── saveinfo()           — 文法读取
├── genFirst()           — FIRST 集
├── genFollow()          — FOLLOW 集
├── LR(0) 构建
│   ├── closure()
│   ├── goTo_0()
│   ├── buildLR0DFA()
│   └── JudgeLR0()
├── LR(1) 构建
│   ├── closure_LR1()
│   ├── goTo_LR1()
│   └── buildLR1DFA()
└── main()
```

---

## 📁 输出文件

| 文件 | 说明 |
|------|------|
| `testdata.txt` | 文法输入文件 |
| `LR0_DFA.dot` | LR(0) DFA 图（Graphviz）|
| `LR1_DFA.dot` | LR(1) DFA 图（Graphviz）|
