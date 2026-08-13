# MLIR Defining Dialects / Operations (ODS) 中文精读版

> 依据官方文档整理：  
> - [Defining Dialects](https://mlir.llvm.org/docs/DefiningDialects/)  
> - [Operation Definition Specification (ODS)](https://mlir.llvm.org/docs/DefiningDialects/Operations/)  
> 这不是逐字全文翻译，而是**核心概念精读**。  
> 建议先读完 [LangRef 中文精读](./MLIR_LangRef_中文精读.md)，再看本文（LangRef = IR 长什么样；本文 = 怎么用 `.td` 定义方言/Op）。

---

## 0. 这两篇文档在讲什么

| 文档 | 解决什么问题 |
|------|----------------|
| **Defining Dialects** | 怎么声明一个 **Dialect**（命名空间、初始化、依赖、常量物化等） |
| **Operations (ODS)** | 怎么用 TableGen **声明一条 Op**（operands/attrs/results、打印、verify、build） |

目标一句话：

```text
用 .td 写清事实 → mlir-tblgen 生成 C++ 样板
→ getXxx() / build / verify / parse/print 少手写
```

没有 ODS 时的痛点（官方动机）：

- 到处 `getOperand(3)`，易错、难读  
- verify 分散或缺失  
- 构造函数冗长、文本 IR 难看  

ODS 的收益：**单一真相来源** + **少样板** + **可再生成文档/其它工具**。

---

## 1. TableGen 最小语法（读 `.td` 够用）

| 概念 | 直觉 | 例子 |
|------|------|------|
| `class` | 可模板、可继承的“模板类” | `class Op<...>` |
| `def` | 具体记录（不能再模板化） | `def NN_LUTOp : ...` |
| `dag` | `(算子 参数...)` 结构 | `(ins I32:$x, F32Attr:$a)` |
| `$name` | 给参数起名 → 生成 getter | `$input` → `getInput()` |
| `include` | 引入其它 `.td` | `include "mlir/IR/OpBase.td"` |

命名习惯（和你们代码一致）：

```text
方言前缀_Op名   →  C++ 类名去掉方言前缀
TF_AddOp        →  AddOp
NN_LUTOp        →  LUTOp（若 def 是 NN_LUTOp）
```

规则：第一个 `_` 当分界；`Foo_Dialect` → C++ `FooDialect`（去掉 `_`）。

Accessor 命名：

```text
snake_case 字段 → getOtherValue() / setOtherValue()
$other_value    → getOtherValue()
```

---

## 2. Defining a Dialect（定义方言）

### 2.1 最小例子

```tablegen
include "mlir/IR/DialectBase.td"

def MyDialect : Dialect {
  let summary = "一行简介";
  let description = [{ 更长的 Markdown 文档 }];
  let name = "my_dialect";           // IR 里的命名空间：my_dialect.foo
  let cppNamespace = "::my_dialect"; // C++ 命名空间
}
```

建议：**Dialect 的 `.td` 和 Ops/Attrs/Types 的 `.td` 分开**，避免重复定义、层次混乱。

### 2.2 必须实现：`initialize()`

每个方言在 C++ 里都要写初始化钩子：注册 op/attr/type、挂 interface 等。

```cpp
void MyDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "MyOps.cpp.inc"
  >();
  // addAttributes / addTypes / ...
}
```

### 2.3 常用字段速查

| 字段 | 作用 |
|------|------|
| `name` | 方言字符串名（`nn`、`schedule`） |
| `cppNamespace` | C++ 命名空间；嵌套用 `"A::B"`；`""` 表示不进命名空间 |
| `dependentDialects` | 依赖其它方言（canonicalizer 会建那些方言的 op 时要加载） |
| `extraClassDeclaration` | 额外 C++ 声明，原样拷进生成类（长尾需求） |
| `hasConstantMaterializer` | fold 后能否从 Attribute **物化常量 op** |
| `hasCanonicalizer` | 方言级 canonicalization patterns |
| `useDefaultAttributePrinterParser` / `useDefaultTypePrinterParser` | 有 mnemonic 时自动生成 attr/type 打印解析（默认常开） |
| `isExtensible` | 运行时还可扩展新 op/type（进阶，可先跳过） |

依赖示例：

```tablegen
let dependentDialects = [
  "arith::ArithDialect",
  "func::FuncDialect"
];
```

### 2.4 Discardable Attribute 与方言前缀

LangRef：名字带方言前缀的 discardable attr（如 `gpu.contained_module`）由**该方言**定义语义。

方言可开验证钩子：

| 字段 | 验证什么 |
|------|----------|
| `hasOperationAttrVerify` | op 字典上的本方言前缀 attr |
| `hasRegionArgAttrVerify` | region 入口参数对应的 attr（如 `arg_attrs`） |
| `hasRegionResultAttrVerify` | region 结果对应的 attr（如 `res_attrs`） |

这和你们改 `argAttrsAttr` 时“每个形参一个字典”是同一套世界。

### 2.5 常量物化（和 fold 相关）

`materializeConstant(builder, attribute, type, loc)`：  
canonicalizer fold 出 Attribute 后，方言负责生成“像常量一样”的 op。  
有自己 `constant` op 的方言常常需要这个。

---

## 3. ODS：定义一条 Operation

### 3.1 核心拼装块（OpBase.td）

| 构造 | 用途 |
|------|------|
| `Op` / `Dialect_Op<"mnemonic", [traits...]>` | 定义一条 op |
| `ins` / `outs` | 参数（operand/attr/prop） / 结果 |
| `TypeConstraint`（含各种 `Type`） | 约束 operand/result 类型 |
| `AttrConstraint`（含各种 `Attr`） | 约束 attribute |
| `Property` / `PropConstraint` | 非 Attribute 存储的固有属性 |
| `Trait` | 语义/语法特性、多实体约束、Interface |

### 3.2 完整骨架（对照你们的 lut）

```tablegen
def NN_LUTOp : NNBase_Op<"lut", [/* traits / interfaces */]> {
  let summary = "NN LUT (Look-Up Table) operation";
  let description = [{ ... }];

  let arguments = (ins
    TensorOrMemref:$input,           // SSA operand
    TensorOrMemref:$lut_data,        // SSA operand
    Variadic<TensorOrMemref>:$output,// SSA operand（DPS 目的缓冲，可空）
    NN_ActivationType:$activation_type, // 固有 attr/prop
    DefaultValuedAttr<I64Attr, "1">:$event_id
  );

  let results = (outs
    Variadic<AnyRankedTensor>:$result_tensor  // 可 0 个 result
  );

  let assemblyFormat = [{
    `(` $input `,` $lut_data (`,` $output^)? `)`
    attr-dict `:` `(` type($input) `,` type($lut_data)
      (`,` type($output)^)? `)` `->` type($result_tensor)
  }];

  let extraClassDeclaration = [{
    MutableOperandRange getDpsInitsMutable() { return getOutputMutable(); }
  }];
}
```

读这段时钉死：

```text
$output 在 ins 里     → 是 operand（目的 buffer），不是 result
$result_tensor 在 outs → 才是 SSA result（Variadic 故可为 0）
打印末尾 `->` type($result_tensor)
  → result 为空时 type(...) 打出空串 → 出现裸 `->`、没有 ()
```

### 3.3 Operation name

```text
完整名 = 方言名 + "." + mnemonic
例：nn.lut、schedule.local_func
```

用于：文本解析/打印、pattern 匹配、`OperationName` 查询。

### 3.4 `arguments = (ins ...)`：三种东西混在一起

官方明确：`arguments` 里可以有三类：

| 种类 | 运行时？ | 存哪 | 例子 |
|------|----------|------|------|
| **Operand** | 是（SSA Value） | use-def | `$input` |
| **Attribute** | 否（编译期常量） | 多数在 attr 字典 / inherent | `$event_id`、`ElementsAttr:$value` |
| **Property** | 否 | **inline 存在 op 里**（不一定进 Context uniquing） | 较新的固有状态 |

记忆：

```text
Operand  = 边（SSA）
Attr/Prop = 配置（不是 SSA）
```

顺序：operand 与 attr **可以交错**；但 **operand 之间的相对顺序有意义**（对应 operand 下标）。

每个 `$name` 都会生成：

- operand → `Value getName()` / `getNameMutable()` 等  
- attr → `XxxAttr getNameAttr()` 以及更友好的 `getName()`（视类型）

#### Natural vs Derived attributes

| | 含义 |
|--|------|
| **Natural** | 定义 op 行为需要的（padding、activation_type） |
| **Derived** | 可从 op 推出、多为方便接口/对接框架（如某输出 shape）；应仍能物化成 Attribute |

#### Variadic / Optional

| 包装 | 含义 |
|------|------|
| `Variadic<Type>` | 0..N 个同约束 operand/result |
| `Optional<Type>` | 0 或 1 个 |
| `OptionalAttr<Attr>` | 可选属性 |
| `DefaultValuedAttr<Attr, "cxx">` | 默认值；等于默认时打印常省略 |
| `ConfinedAttr<Attr, [约束...]>` | 额外约束（如 `ArrayMinCount<4>`） |

多个可变长度 operand 时，通常要加：

- `SameVariadicOperandSize`，或  
- `AttrSizedOperandSegments`  

否则运行时无法把动态 operand 列表拆回各个静态槽位。

### 3.5 `results = (outs ...)`

与 operand 对称；`Variadic<>` 同样适用。  
**0 个 result** 完全合法（DPS：结果写进 `$output` operand）。

### 3.6 Regions / Successors

```tablegen
let regions = (region
  SizedRegion<1>:$body
  // VariadicRegion<> 只能放在最后
);

let successors = (successor
  AnySuccessor:$dest
  // 给 terminator；VariadicSuccessor<> 只能最后
);
```

函数类 op（`local_func` / `dispatch`）的“函数体”就是 **region**；  
`cf.br` 一类才需要 **successors**。

### 3.7 Traits / Interfaces（`Op` 的模板参数列表）

```tablegen
def FooOp : My_Op<"foo", [
  NoMemoryEffect,
  SameOperandsAndResultType,
  DeclareOpInterfaceMethods<MyInterface>
]> { ... }
```

Traits 影响：语法、语义、verify、是否可 fold、是否 terminator 等。  
多 operand/result 之间的约束也挂在这里（如 `AllTypesMatch`）。

常见直觉：

| Trait / Interface | 直觉 |
|-------------------|------|
| `NoMemoryEffect` | 无内存副作用（纯计算） |
| `Terminator` | 必须在 block 末尾 |
| `IsolatedFromAbove` | region 不能随便用外层 SSA |
| `InferTypeOpInterface` | 可从 operands 推断 result types → build/打印可省略 result types |
| DPS 相关 interface | `$output` 当 destination；你们 `getDpsInitsMutable` |

---

## 4. Builder（`create` / `build` 从哪来）

ODS 按 `ins`/`outs` **自动生成多个 `build` 重载**，大致包括：

1. **聚合版**：`TypeRange` + `ValueRange` + properties/attrs（方便 pattern）  
2. **逐参数版**：每个 result type / operand / attr 一个形参（手写 pass 友好）  
3. **可推断 result type 时**：可省略 result types 的重载  

关键不对称（和 LangRef 一致）：

```text
create 时：
  operands → 传已有 Value
  results  → 传 Type（框架生成新 Value）
  attrs    → 传 Attribute / 或解包后的裸值
```

自定义 builder：

```tablegen
let builders = [
  OpBuilder<(ins "float":$val), [{
    $_state.addAttribute("attr", $_builder.getF32FloatAttr(val));
  }]>
];
```

特殊变量：`$_builder`、`$_state`；默认参数用 `CArg<"float", "0.5f">`。

---

## 5. 打印与解析：`assemblyFormat`

### 5.1 声明式格式长什么样

```tablegen
let assemblyFormat = [{
  $callee `(` $args `)` attr-dict `:` functional-type($args, results)
}];
```

三类组件：

| 组件 | 例子 |
|------|------|
| **字面量** | `` `(` `` `` `->` `` `` `:` `` |
| **变量** | `$input`、`$activation_type` |
| **指令（directive）** | `attr-dict`、`type($x)`、`functional-type(...)`、`prop-dict` |

### 5.2 最常用 directive

| Directive | 含义 |
|-----------|------|
| `attr-dict` | 属性字典（**几乎必须出现**）；discardable 总在这里 |
| `prop-dict` | 把 properties/未出现的固有 attr 打成字典（常在 `<>`） |
| `type(x)` | 打印/解析某 operand 或 result 的类型 |
| `functional-type(inputs, outputs)` | 打成 `(in...) -> (out...)`，**空结果一般是 `-> ()`** |
| `operands` / `results` / `regions` / `successors` | 一次性覆盖全部 |
| `custom<Name>(...)` | 一段手写 C++ parse/print |
| `( ... $anchor^ ...)?` | 可选组；`^` 标记锚点 |

### 5.3 硬性要求（官方 Requirements 精简）

1. 所有 **operands** 都要在格式里出现  
2. 所有 **regions / successors** 同理  
3. 所有 operand/result 的 **type** 都要能解析到（或可推断而省略）  
4. 必须有 **`attr-dict`**  
5. 若有未在格式中出现的 non-attribute properties → 需要 **`prop-dict`**  
6. 不能重复重叠信息  

类型可省略的情况：`BuildableType`（如 `I32`）、equality traits（`SameOperandsAndResultType` 等）、`InferTypeOpInterface`。

### 5.4 为什么你们 `nn.lut` 是裸 `->` 而不是 `-> ()`

格式末尾是：

```text
`->` type($result_tensor)
```

`$result_tensor` 为 **空 Variadic** 时，`type(...)` 产出**空串**，于是只剩 `->`。  
若写成 `functional-type(...)`，空结果通常会规范打成 `-> ()`。

这是 **打印机格式问题**，不是“没有类型”或“指向下一行 return”。

### 5.5 `<>` vs `{}`（和你们现象对齐）

| 打印 | 常见来源 |
|------|----------|
| `{...}` = `attr-dict` | 多数方言自定义格式只写了 `attr-dict`；固有 attr 也进这里 |
| `<{...}>` = `prop-dict` / properties | 较新或函数类 op（你们看到 `local_func` 有 `<>`） |

**不能**只靠括号判断是不是固有属性；以 ODS `ins` 是否声明为准。

---

## 6. Verify（校验顺序）

自动：类型约束、attr 约束、若干 traits。  
额外手写：

```tablegen
let hasVerifier = 1;        // LogicalResult verify();
let hasRegionVerifier = 1;  // 需要看 region 内 op 时用
```

执行顺序（简化）：

```text
1) Structural traits
2) ODS 生成的 verifyInvariants（类型/属性等）
3) 其它 verifyTrait（不访问 region）
4) 自定义 verify()
── 若有 region，等 region 内 op 都 verify 完 ──
5) verifyRegionTrait
6) 自定义 verifyRegions()
```

自定义 verifier 打诊断时优先用 **Error**（generic 打印），避免依赖尚未 verify 的 custom printer。

---

## 7. 其它常用布尔开关

| 字段 | 你要实现的 |
|------|------------|
| `hasCanonicalizer` | `getCanonicalizationPatterns()` |
| `hasCanonicalizeMethod` | 简单的 `canonicalize(...)` |
| `hasFolder` | `fold(...)` |
| `hasVerifier` / `hasRegionVerifier` | 见上 |
| `extraClassDeclaration` | 塞进类声明的 C++（如 DPS 辅助函数） |
| `extraClassDefinition` | 塞进 `.cpp` 的共用定义；`$cppClass` 可替换类名 |

---

## 8. 生成物与工程接入（知道即可）

`mlir-tblgen` 大致生成：

| 选项 | 产物 |
|------|------|
| `-gen-op-decls` | `*Ops.h.inc` 声明 |
| `-gen-op-defs` | `*Ops.cpp.inc` 定义 |
| `-gen-dialect-docs` 等 | 文档 |

C++ 侧：

```cpp
#define GET_OP_CLASSES
#include "NNOps.cpp.inc"
```

每个 op 还会生成 **Adaptor**（`LUTOpAdaptor`）：给 `ValueRange` 起与 op 相同的命名访问器，方便 pattern 里不用魔法下标。

---

## 9. 和你们代码的对照表

| 你们的东西 | ODS / Dialect 概念 |
|------------|---------------------|
| `nn` / `schedule` / `acuity` | `Dialect.name` |
| `NN_LUTOp` / `"lut"` | `def` + mnemonic → `nn.lut` |
| `$input` / `$lut_data` | operands（SSA） |
| `$output`（ins + Variadic） | DPS 目的 operand，不是 result |
| `$activation_type` / `$event_id` | 固有 attributes（arguments 里的 Attr） |
| `$result_tensor` Variadic outs | SSA results；buffer 形态下常为 0 |
| `assemblyFormat` + `attr-dict` | 自定义打印；attr 多在 `{}` |
| `local_func` 的 `<>` | 多半 `prop-dict` / 函数属性打印 |
| `getDpsInitsMutable` | `extraClassDeclaration` + DPS |
| `function_type` | 函数类 op 的 **FunctionType** 属性/属性式签名（不是 lut 那种 `(types)->` 打印） |
| `arg_attrs` | 函数类 discardable / 区域参数属性；改签名时常要同步补空字典 |

建 op 时：

```cpp
rewriter.create<NN::LUTOp>(loc, resultTypes, input, lutData, output, ...);
// 或 replaceOpWithNewOp：重接旧 result 的 uses，不自动改 call 签名
```

---

## 10. 建议阅读顺序（针对你们项目）

1. **§3.2–3.5**：`ins`/`outs`、Variadic、DPS（对照 `nn.lut`）  
2. **§5**：`assemblyFormat`（解释裸 `->`、`{}` vs `<>`）  
3. **§4**：生成的 `build` 与 `create` 参数不对称  
4. **§2**：Dialect 注册 / `dependentDialects` / `arg` 属性验证  
5. 需要时再查官方：AttributesAndTypes、Traits、Interfaces、DRR patterns  

原文：

- https://mlir.llvm.org/docs/DefiningDialects/  
- https://mlir.llvm.org/docs/DefiningDialects/Operations/  

---

## 11. 一句话总纲

```text
Dialect = 命名空间 + initialize 注册
Op ODS  = mnemonic + traits
          + arguments(ins: SSA | Attr | Prop)
          + results(outs)
          + regions/successors
          + assemblyFormat / verify / builders

ins 里的 Attr/Prop = 固有配置（不一定打印在 <>）
outs = SSA results（可以为空；空时别指望 type() 自动打出 ()）
打印长什么样由 assemblyFormat 决定，不是 LangRef 唯一形态
```
