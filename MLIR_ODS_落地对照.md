# MLIR ODS 落地对照（对着你们 IR / 代码看）

> 不讲空概念。每一条都落到你们已经见过的东西上。  
> 配套长文：`MLIR_DefiningDialects_ODS_中文精读.md`

---

## 1. 一张图先钉死

```text
.td 里写 NN_LUTOp
        ↓ mlir-tblgen
生成 LUTOp 类：getInput() / build / verify / print
        ↓ pass 里
rewriter.create<NN::LUTOp>(...)
        ↓ 打印成
nn.lut(%7, %10, %8) {activation_type=...} : (memref, memref, memref) ->
```

| 你在哪 | 看到什么 |
|--------|----------|
| `.td` | 定义“这条 op 长什么样” |
| C++ pass | `create` / `replaceOpWithNewOp` |
| `.mlir` 文本 | `nn.lut(...)` |

---

## 2. 对着你们这条 lut 读 ODS

### IR（你们实际打印）

```mlir
nn.lut(%7, %10, %8) {
  activation_type = 0 : i32,
  input_qtypes = [f16, i8, f16],
  result_qtypes = [f16]
} : (memref<1x3x6x6xf16, ...>,
     memref<4352xi8, ...>,
     memref<1x3x6x6xf16, ...>) ->
schedule.local.return
```

### `.td`（你们定义）

```tablegen
let arguments = (ins
  TensorOrMemref:$input,              // → %7
  TensorOrMemref:$lut_data,           // → %10
  Variadic<TensorOrMemref>:$output,   // → %8（目的 buffer）
  NN_ActivationType:$activation_type,
  DefaultValuedAttr<I64Attr,"1">:$event_id
);
let results = (outs
  Variadic<AnyRankedTensor>:$result_tensor  // 现在是 0 个
);
```

### 一一对应

| IR 里 | ODS 里 | C++ 里大致 |
|-------|--------|------------|
| `%7` | `$input` | `lut.getInput()` |
| `%10` | `$lut_data` | `lut.getLutData()` |
| `%8` | `$output`（在 **ins**） | `lut.getOutput()` |
| `{activation_type=...}` | `$activation_type` | `lut.getActivationType()` |
| `->` 后面空 | `$result_tensor` 长度为 0 | `getNumResults()==0` |
| 下一行 `local.return` | **不是** lut 的一部分 | 下一条 op |

口诀：

```text
结果写进 %8 这个 memref  →  所以没有 %x = nn.lut
$output 在 arguments(ins) →  是输入槽位里的“目的地”
$result_tensor 在 results →  才是 SSA 返回值（你们当前为空）
```

---

## 3. 为啥打印是裸 `->`，不是 `-> ()`

你们格式最后是：

```tablegen
`->` type($result_tensor)
```

| `$result_tensor` | 打印机行为 | 你看见 |
|------------------|------------|--------|
| 有 tensor | 打出类型 | `-> tensor<...>` |
| **空** | `type(...)` 打出**空白** | 只剩 `->` |

所以：**不是语义指向 return**，是格式串遇上了空 Variadic。

---

## 4. Operand / Attr / Result —— 只用你们的词

拿 `nn.lut`：

```text
(%7, %10, %8)     ← Operand：必须是已经存在的 SSA
{activation_type} ← Attribute：编译期常量，不是 SSA
-> （空）         ← Result：0 个新 SSA
```

再拿 `acuity.constant`（对比）：

```text
()                ← 无 Operand
{value = dense_resource...}  ← Attribute 扛大数据
-> tensor/memref  ← 有 Result SSA，给后面用
```

建 op 时为什么不对称：

```cpp
// operand：传 Value
// result ：传 Type，框架生成 Value
rewriter.create<NN::LUTOp>(loc,
  /*resultTypes=*/TypeRange{},   // 0 result
  input, lutData, output, ...);
```

---

## 5. `local_func`：为啥 view 要用函数参数，不用外面 constant 的 result

```text
模块外/调用侧：
  %buf = ... constant / to_memref ...     ← result SSA
  schedule.local_call @foo(%buf, ...)

local_func @foo 里面：
  ^bb0(%arg_buf, ...):                   ← block argument（另一条 SSA）
     %view = memref_ext.static_view %arg_buf ...
     nn.trigger(..., %view, ...)
```

| 位置 | 能用的 SSA |
|------|------------|
| call 旁边 | constant 的 **result** |
| func **体内** | 只能用 **block argument**（`legalConstArg`） |

所以代码才是：先找/加 argument → `legalConstArg` → 再 `create<StaticViewOp>(..., legalConstArg, ...)`。

---

## 6. `function_type` 到底打什么（函数，不是 lut）

```text
function_type = (memref<...>, memref<...>) -> ()
                 └─ 每个形参的 Type ─┘     └─ 返回类型
```

| 会变 | 不变（不在 function_type 里） |
|------|------------------------------|
| 形参 Type / 返回 Type | `%arg` 名字 |
| | 函数体 |
| | `arg_attrs` |
| | 业务 attr（activation_type 等） |

改 block arg 的 shape/rank → **同步改 `function_type` 对应 input**；call 实参那条 Value 的 type 也要对齐。

---

## 7. `{}` 和 `<>`（按你们仓库真实打印）

| 你们看见 | 实际多半是 |
|----------|------------|
| `nn.lut ... {activation_type=...}` | `assemblyFormat` 用了 `attr-dict`，固有 attr 也进 `{}` |
| `local_func ... <>` | 这个 op 的格式用了 properties / `prop-dict` |

**别用括号判断“是不是固有属性”。**  
判断法：看 `.td` 的 `ins` 有没有声明；有 = 固有。

---

## 8. Pass 里三条最常用动作

### A. 新建

```cpp
auto op = rewriter.create<NN::LUTOp>(loc, resultTypes, operands..., attrs...);
```

### B. 替换（只重接 uses）

```cpp
rewriter.replaceOpWithNewOp<Acuity::ConstantOp>(oldConst, newType, newValueAttr);
// ✅ 旧 result 的用户改指向新 result
// ❌ 不会给 dispatch_call 多加 operand，不会改 function_type
```

### C. 给 local_func 塞一个 buffer 实参（你们 piggyback）

```text
1) 所有 local_call 追加 operand = bufferizedConstArg
2) local_func entry block addArgument(同 type)
3) 更新 function_type
4) arg_attrs 补一个空字典 {}
5) 函数体内用新的 block arg 建 view / trigger
```

---

## 9. 读别人 `.td` 的 30 秒清单

1. mnemonic？ → IR 名 `方言.mnemonic`  
2. `ins` 里哪些是 Type（SSA），哪些是 `*Attr`（配置）？  
3. `outs` 几个 result？有没有 `Variadic` 可能为 0？  
4. 有没有 DPS：`output` 在 ins + `getDpsInitsMutable`？  
5. `assemblyFormat` 最后怎么打 type？空 result 会不会裸 `->`？  
6. traits 里有没有 `SameOperandsAndResultType` / Terminator / Interface？

---

## 10. 和 LangRef 的分工（一句话）

```text
LangRef：IR 运行时/文本里是什么（Region/Block/Value）
ODS    ：.td 如何生成 C++，让你少手写 build/print/verify
落地   ：你们的 lut / local_func / constant / trigger 怎么对上号
```
