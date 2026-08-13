# MLIR Language Reference 中文精读版

> 依据官方文档整理：[MLIR Language Reference](https://mlir.llvm.org/docs/LangRef/)  
> 这不是逐字全文翻译，而是**核心概念精读**，方便英文一般的同学把结构钉死。  
> 官方原文是“干参考手册”；更活泼的解释见 Rationale / Glossary / Tutorials。

---

## 0. MLIR 是什么

MLIR（Multi-Level IR）是一种**编译器中间表示**：

- 类似 LLVM IR 那种 **SSA** 思想；
- 又能表示更高层的数据流图、以及更靠近硬件的代码；
- 目标：在同一套框架里，从高层图一路 lower 到高性能、甚至目标相关的代码。

MLIR 有三种形态，**语义相同**：

1. **文本形式**（`.mlir`，给人看、调试、写测试）——LangRef 主要讲这个  
2. **内存形式**（C++ 里的 `Operation`/`Value`，给 pass 改）  
3. **序列化形式**（存储/传输）

---

## 1. 高层结构（最重要）

MLIR 本质是一张图：

- **节点** = Operation（操作）  
- **边** = Value（值，SSA）

层级关系：

```text
Region（区域）
  └─ Block（基本块）
       └─ Operation（操作，按顺序排列）
            └─ 还可以再包含 Region（嵌套）
```

关键事实（官方原意）：

> **每个 Value 恰好来自一个定义：要么是某个 Operation 的结果（OpResult），要么是 Block Argument。**  
> 每个 Value 都有一个 Type。

Operations 可以表示各种各样的东西：函数定义、函数调用、内存分配、buffer view、算术、机器指令……  
集合是**可扩展的**（靠 Dialect 加新 op）。

变换靠 **Pass**；为了不让每个 pass 认识所有 op，用 **Traits / Interfaces** 描述抽象语义。

---

## 2. 标识符（读 IR 时会见到）

| 前缀 | 含义 | 例子 |
|------|------|------|
| `%` | Value（SSA） | `%0`, `%arg0` |
| `@` | Symbol（符号名，如函数名） | `@mul` |
| `^` | Block 标签 | `^bb0` |
| `#` | Attribute 别名等 | （高级用法） |
| `!` | Type 别名等 | （高级用法） |

注意：

- `%` 名字主要方便人读；**不一定持久化**，打印时可能变成 `%42`。  
- Value 的作用域一般在定义它的（嵌套）Region 内，出不去。  
- Symbol（`@foo`）有另一套符号表作用域规则。

---

## 3. Dialect（方言）

Dialect = 扩展 MLIR 的机制：定义新的 **Operation / Attribute / Type**。

- 每个方言有唯一 **namespace**（如 `affine`、`func`、`memref`、`arith`）  
- 一个 Module 里可以共存多个方言  
- Pass 可以在方言之间做 conversion  

你们的 `nn` / `schedule` / `acuity` / `memref_ext` 都是方言。

---

## 4. Operations（操作）——核心中的核心

### 4.1 一条 Operation 里有什么

官方描述，一条 op 可以有：

| 组成部分 | 含义 | 是不是 SSA |
|----------|------|------------|
| **Results** | 输出值（0 或多个） | ✅ Value |
| **Operands** | 输入值（0 或多个） | ✅ Value（使用已有 Value） |
| **Attributes** | 属性字典（编译期常量配置） | ❌ |
| **Properties** | 较新的结构化属性存储 | ❌（类似 attr） |
| **Successors** | 后继 block（给分支用） | ❌（指向 Block） |
| **Regions** | 嵌套区域（0 或多个） | ❌（里面再有 Block/Op） |

通用打印形态（generic form）大致是：

```mlir
%results = "dialect.op"(%operands) <{properties...}> ({
  // regions...
}) {discardable_attrs...} : (input_types) -> (result_types)
```

例子：

```mlir
%foo, %bar = "foo_div"() {some_attr = "value", other_attr = 42 : i64}
  : () -> (f32, i32)

%2 = "tf.scramble"(%result#0, %bar) <{fruit = "banana"}>
  : (f32, i32) -> f32
```

### 4.2 和中文“参数”怎么对应（防混淆）

LangRef **几乎不说笼统的 parameter**。请强制拆开：

| 你想说的 | MLIR 术语 |
|----------|-----------|
| 输入数据流 | **Operand**（SSA） |
| 输出数据流 | **Result**（SSA） |
| 自己附带的配置（offset、名字、trigger_type） | **Attribute / Property** |
| 函数形参 `%arg` | **Block Argument**（也是 Value，但不是该 func op 的 operand） |
| create/build 函数的 C++ 形参 | 只是 C++ API，别和 IR 术语混用 |

### 4.3 Operand vs Result（建 op 时为何不对称）

- **Operand**：必须是**已经存在**的 Value → `create` 时传 `Value`  
- **Result**：此时还不存在 → `create` 时传 **Result Type(s)**，由框架生成新的 result Value  

建好之后：`getOperand` / `getResult` **都返回 Value**。

### 4.4 自定义打印

方言注册了已知 op 后，可以用 **custom assembly form**（好看的写法），不必总用 `"dialect.op"(...)` 这种 generic 形式。

---

## 5. Blocks（基本块）

Block = **一串有序的 Operations**。

在 **SSACFG region** 里，Block ≈ 传统编译器的 basic block：

- 块内顺序执行  
- **最后一条必须是 Terminator（终结指令）**（少数带 `NoTerminator` trait 的 op 例外，如顶层 `module`）

### 5.1 Block Arguments（块参数）

Block 可以带参数，写法像函数参数：

```mlir
^bb0(%a: i64, %cond: i1):
  cf.cond_br %cond, ^bb1, ^bb2

^bb3(%c: i64):
  ...
```

要点：

- Block Argument **也是 Value（SSA）**  
- Entry block 的 arguments = 这个 Region 的 arguments  
- 其它 block 的 arguments 由 **Terminator（如分支）** 传入  
- 这样可避免传统 SSA 里复杂的 PHI 节点

### 5.2 Terminator（终结指令）

Block 末尾决定“下一步去哪”的 op，例如：

- `cf.br` 无条件跳转  
- `cf.cond_br` 条件跳转  
- `return` 返回  

没有后继的 terminator 可能表示返回给外层 op，或不可达（如 `ub.unreachable`），具体语义由方言决定。

---

## 6. Regions（区域）

Region = **有序的 Block 列表**。

- Region **没有名字、没有类型、没有 attributes**  
- 必须包含在某个 Operation 里  
- **第一个 block 叫 entry block（入口块）**  
- Entry block 的 arguments = region 的 arguments  
- Entry block **不能**成为其它 block 的 successor

Region 的语义**不由 IR 通用规则强加**，而由**包含它的那个 Operation**定义。

### 6.1 两种 Region 种类（概念）

1. **SSACFG region**：块之间有控制流（函数体最常见）  
2. **Graph region**：不要求块间控制流（更像图）

由 `RegionKindInterface` 等描述。

### 6.2 值的可见性（Scoping）

- 不能分支到别的 region 里的 block  
- Region 内定义的值默认**逃不出**该 region  
- 默认情况下，region 内可以引用外层已可见的值（像外层 op 的 operand 能引用的那些）；也可用 `IsolatedFromAbove` 等限制  

### 6.3 控制流直觉（SSACFG）

1. 控制流进入 region → **总是从 entry block 开始**  
2. 执行块内 op，直到 terminator  
3. Terminator 决定继续跳到哪个 successor，或返回/结束  
4. 从未当过后继的非入口块 = 不可达，可删  

这就是 CFG（Control Flow Graph，控制流图）：**Block 是节点，Terminator 画出的跳转是边**。

---

## 7. 函数类结构怎么套进上面的术语

以 `func.func` / 你们的 `schedule.dispatch` 为例：

```mlir
func.func @foo(%arg0: i32, %arg1: f32) -> i32 {
  ^bb0(%arg0: i32, %arg1: f32):
    ...
    return %x : i32
}
```

| 现象 | 术语 |
|------|------|
| `func.func` 这条 | Operation（常无 operand） |
| `@foo` | Symbol |
| `(i32,f32)->i32` | Function Type（类型/签名） |
| `%arg0/%arg1` | Entry Block Arguments（SSA 形参） |
| `{ ^bb0: ... }` | Region / 实现体 |
| `func.call @foo(%x,%y)` | 另一条 Call Operation；`%x/%y` 是其 **operands（实参）** |

所以：

- **声明/定义** ≈ function-like op + symbol + function_type + region  
- **调用** ≈ call-like op + callee 符号属性 + operands  
- **实现** ≈ region 里的 blocks  

`dispatchOp.getArgument(i)`（若有）通常只是 entry block argument 的语法糖。

---

## 8. Attributes（属性）与 Types（类型）—精读要点

### Attributes

- 编译期就知道的常量元数据  
- 挂在 Operation 上（字典）  
- **不是 SSA**，没有 use-def 数据流边  
- 例子：`{axis = 1 : i64, message = "mul"}`、`ElementsAttr` 里的常量数据  

Constant 类 op 常见模式：

```text
无 operand
有 Attribute 存大数据（如 dense / dense_resource）
有 result SSA 把常量接入数据流
```

### Types

- 描述 Value 的类型：`i32`、`f32`、`tensor<...>`、`memref<...>`、`FunctionType` 等  
- `RankedTensorType` / `MemRefType` 含 **shape（维度）**  
- 从 Value 取维度：`value.getType()` → `cast<ShapedType>` → `getShape()`

### FunctionType

- 一种 Type：`(inputs...) -> (results...)`  
- **主要用于函数类 op 的签名声明**  
- 普通 arith/trigger 不一定有 FunctionType；它们只有各自 operand/result 的类型  

---

## 9. 和你们代码强相关的对照

| 场景 | LangRef 概念 |
|------|----------------|
| `getOperand` / `getResult` | Operation 的 SSA 输入/输出 |
| `getAttr` / TriggerTypeAttr | Attributes |
| `cmdView` 挂到 trigger | 把已有 Value 当作 operand |
| `newTensorType` 建 Constant | 传 **result type**，框架生成 result Value |
| `dispatch()` 无 operand | function-like；形参在 block arguments |
| `function_type = ...` | 签名 Type/属性 |
| `dispatch_call` | call-like；实参是 operands；callee 是符号属性 |
| `static_view` | 普通 op：operands + attrs，产生 view result |
| `cf.br` | Terminator；构成 CFG 边 |
| piggyback 改 functype/argument | 同时改签名与 block arguments（及 call operands）以保持一致 |

---

## 10. 建议你怎么用这份精读

1. 先反复看 **§1 结构、§4 Operation、§5 Block、§6 Region**  
2. 强制禁用笼统词“参数”，改说 operand / attribute / block argument  
3. 拿一条真实 IR：`constant` / `trigger` / `dispatch` / `dispatch_call` 各标注一遍  
4. 英文对照时，只打开官网对应小节，不必整页硬啃  

原文入口：https://mlir.llvm.org/docs/LangRef/

---

## 11. 一句话总纲

```text
Region → Block → Operation
Operation 连接 Value（SSA）：operands 入，results 出
Value 只可能是：OpResult 或 BlockArgument
配置用 Attribute；签名用 Type（如 FunctionType）
控制流：Terminator 连接 Blocks，形成 CFG
```
