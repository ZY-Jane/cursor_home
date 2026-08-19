# C++ 内存笔记（对照 C / 结合我们的 codegen）

面向刚从 C 转过来的写法。例子用 `std::string`、`std::vector`、`TriggerParams`、`VipLiteCodegenPass`。

---

## 1. 和 C 最大的差别

C：谁 `malloc`，谁 `free`。

C++：尽量让 **对象在析构函数里自己释放**（RAII）。你把 `string` / `vector` 当变量用，出了 `}` 自动收堆，不必 `free`。

```c
char *p = malloc(n);
free(p);                 // 必须自己写
```

```cpp
std::vector<uint8_t> buf(n);
// 出作用域自动释放
```

---

## 2. 类不是变量，对象才是

**类 ≈ C 的 `struct` 类型（图纸）；对象 ≈ C 的变量（真正占内存）。**

```cpp
class TriggerParams { ... };   // 类型
TriggerParams params;          // 对象 = 变量
```

`params` 在栈上（或作为别人的成员），出 `}` 就没了。C++ 多出来的是：没了时会调析构，把内部堆也收掉。

不要 `delete &params`。它不是 `new` 出来的。

---

## 3. `vector` / `string`：壳在栈上，数据在堆上

```text
栈上（对象本身很小，大约几个指针）
┌──────────────────┐
│ ptr, size, cap   │ ──► 堆上：真正的字节 / 字符
└──────────────────┘
   std::vector / std::string
```

- `std::vector<uint8_t> buf;`：先有空壳，还没有堆。
- `push_back` / `resize`：`vector` **自己**在堆上申请 buffer。
- 类里放 `vector` 当成员：类对象仍然很小，大块数据在堆上。
- **不要因为「逻辑上数据很大」就 `new` 整个类。** 改成 `vector` 成员即可。

会把对象撑大的是 **嵌在类肚子里的大数组**：

```cpp
uint8_t buf[1 << 20];   // 对象本身就 1MB，栈上可能爆
```

这种优先改成 `std::vector<uint8_t>`，而不是 `new` 整个类。

也不要：`new std::vector<uint8_t>()`。`vector` 已经自己管堆了。

---

## 4. 什么时候要手动释放

**不用你 `free` / `delete`**

| 写法 | 谁释放 |
|------|--------|
| 局部 `string` / `vector` / `TriggerParams` | 出 `}` 析构 |
| `return std::string` / `return std::vector` | 调用方接到的对象析构 |
| `std::move` 进成员（`loadStateBuf_`） | 成员所在对象析构 |
| `unique_ptr<T>` | 智能指针析构时 `delete` |

CL：`std::string tc_code` 出函数自己析构。  
NPU：`packLoadState` `return vector`，move 进 `loadStateBuf_`，collector 析构时释放。

**不要 `delete`（你不是拥有者）**

| 拿到的 | 原因 |
|--------|------|
| `.c_str()` / `.data()` / `ArrayRef` | 窗口，内存还在 `string`/`vector` 里 |
| `const HardwareInfo *` | Pass 上的对象，只是借用 |
| `Operation *` | IR 节点，MLIR context 管 |

这些不是你 `new` 的。删了会把别人的对象拆掉。

**才需要按约定释放**

- C API：`malloc` / `fopen` / `XxxCreate` → 对上 `free` / `fclose` / `XxxDestroy`
- 自己 `new T` 又没交给 `unique_ptr` → 必须 `delete`（尽量别这样写）
- 内部模块给你 `{data, size}` 且文档说「拷完后内部 free」→ 调它的 release

判断口诀：

1. 类型是 `string` / `vector` / 普通类对象 → 不用手动释放  
2. 是裸 `T*` → 问：谁 `new`/`malloc` 的？不是你就别删  
3. `return` 一个 `vector`/`string` → 移交所有权，内部不用再 `free`  
4. 只把内容 `os <<` 拷走 → 两边各管各的

---

## 5. `new` 和智能指针

默认：**不要 `new`。**

需要堆，通常是：

- 对象必须活过当前函数，又不能靠返回值带出去
- 多态：基类指针指向子类
- 要长期挂在别人（如 `PassManager`）手里

能 `return vector` / `return string` 的，用返回值。

用了堆，就用智能指针，不要裸 `new`/`delete`：

| 情况 | 用什么 |
|------|--------|
| 只有一个主人 | `std::unique_ptr<T>`（默认选这个） |
| 多个主人，最后一个走才释放 | `std::shared_ptr<T>` |
| 只观察、不负责释放 | `T&` 或普通 `T*`（借用） |

```cpp
auto p = std::make_unique<T>();   // 出作用域自动 delete
```

口诀：

1. 能当局部 / 成员变量 → 不用 `new`  
2. 能返回值 → 不用 `new`  
3. 必须堆上且你拥有 → `make_unique`  
4. 借用别人的 → 普通指针，不要 `delete`，也不要 `unique_ptr` 包别人的

MLIR 的 `Operation*`、`Value` 是框架建的，`dyn_cast` 只是句柄，不要 `new`/`delete`/`unique_ptr`。

---

## 6. 栈上建变量，再用 `move` 带出函数

可以，这是正经写法。`move` 把 **壳里的资源**（堆上 buffer）交给对方，不是把栈变量整块搬走。

```cpp
std::vector<uint8_t> packLoadState(...) {
  std::vector<uint8_t> buf;   // 栈上的壳
  // 往 buf 里填
  return buf;                 // 一般不必写成 return std::move(buf)
}

collector.add(TriggerNbgEntry(..., std::move(loadStateBuf)));  // 交给参数时写 move
```

move 之后：

- 对方 `vector` 拿走堆上那块（指针移交，字节通常不拷贝）
- 局部 `buf` 变成空壳，函数结束析构
- **不要再读 `buf` 里的内容**

不能 `return &buf` / `return buf.data()`，那是把栈/即将失效的地址带出去。

和 C 对照：C 带出堆内存只能 `malloc` 再返回指针，调用方 `free`。C++ 用栈上 `vector` + `return`/`move`，壳生灭在栈上，堆跟着对象走。

---

## 7. `move` 不是把栈对象搬到堆上

`move` 移交的是 **对象里面的资源**，对象自己住在哪（栈还是堆）不变。

```text
栈上 tmp（函数结束必没）
┌──────────────────┐
│ Pass / vector 壳 │──► 堆上：成员 vector 的 buffer
└──────────────────┘
        std::move
           │
           │  不是把上面这个方块挪走
           ▼
堆上（make_unique 新申请的一块）
┌──────────────────┐
│ 另一个对象       │  unique_ptr 指向这里，以后 delete 这里
│ 成员被 move 过来 │──► 同一块 buffer（只移交 ptr）
└──────────────────┘
```

所以：`std::make_unique<VipLiteCodegenPass>(std::move(tmp))` 仍然会 **先在堆上再造一个 Pass**，再用 `tmp` 做移动构造。栈上的 `tmp` 还在栈上，最后析构成空壳。

---

## 8. Pass 为什么用 `unique_ptr`（不是因为类大）

Pass 对象通常很小，IR 在 `ModuleOp` / `MLIRContext` 里。

用 `unique_ptr` 是因为：

1. 要活过 `createXxxPass()`，交给 `PassManager` 长期拿着  
2. 多态：管道里是基类 `Pass`  
3. 所有权：PM 独占，跑完析构  

```cpp
return std::make_unique<VipLiteCodegenPass>();
pm.addPass(createVipLiteCodegenPass());
```

**不能**把栈上 Pass 的地址交给 `unique_ptr`：

```cpp
VipLiteCodegenPass pass;
return std::unique_ptr<Pass>(&pass);  // 错：函数结束栈对象没了，还会 delete 栈
```

`unique_ptr` 默认 `delete`，必须指向堆。

两种 `make_unique` 的差别只是 **构造方式**：

```cpp
std::make_unique<VipLiteCodegenPass>();                 // 堆上直接构造
std::make_unique<VipLiteCodegenPass>(std::move(tmp));   // 堆上移动构造
```

结果一样：堆上一个 Pass + 一个 `unique_ptr`。没有理由先造 `tmp` 再 move，选项直接传给 `make_unique(...)`。

对照：

| | Pass | `vector` / `TriggerParams` |
|--|------|---------------------------|
| 带出函数 | `unique_ptr`（堆 + 多态） | `return buf`（move 值） |
| 对你 | 调 `createXxxPass()` | 当变量用 |

对象要当基类指针长期挂在别人那里 → `make_unique`。  
只是把数据带出函数 → 返回值 / `move`。

---

## 9. `unique_ptr p = std::move(tmp)` 对不对？

看 `tmp` 是什么。

```cpp
// tmp 是栈上的 Pass → 错，编译不过
VipLiteCodegenPass tmp;
std::unique_ptr<VipLiteCodegenPass> p = std::move(tmp);

// 正确：先堆上造，再移动构造
auto p = std::make_unique<VipLiteCodegenPass>(std::move(tmp));

// tmp 已经是 unique_ptr → 对，移交指针所有权
auto tmp = std::make_unique<VipLiteCodegenPass>();
std::unique_ptr<VipLiteCodegenPass> p = std::move(tmp);
```

`unique_ptr` 不能拷贝，只能 `move` 另一个 `unique_ptr`。不能直接从 Pass 对象赋过来。

---

## 10. 和我们 codegen 的对应

**OpenCL**

```cpp
std::string inner();                 // 内部建 string，return
std::string tc_code = inner();       // 接到（通常 move）
os << tc_code;                       // 字符拷进 kernelBuffer
// 出函数：tc_code 析构，释放自己那块
// kernelBuffer 还在，由外层 string 析构释放
```

`os <<` 是拷贝内容，不是把 `tc_code` 所有权交给 `os`。不要 `return tc_code.c_str()`。

**NPU loadState**

内部若是 C++，和对 CL 一样：

```cpp
FailureOr<std::vector<uint8_t>> packed = packLoadState(params);
collector.add(TriggerNbgEntry(..., std::move(*packed)));
```

`return buf` 把内部 vector **move** 出来。entry/collector 析构时自动释放。不必 `{data, size}` 再 copy，也不必 `LoadStateStore`。

只有内部是 C 的 `uint8_t*` + `size` 时，才在边界上：你分配 `vector`、`memcpy`、内部再 `free` 它那块。

---

## 11. 一张总表

| 场景 | 做法 |
|------|------|
| 局部数据、函数返回数据 | 栈上 `string`/`vector`/小类，`return` / `move` |
| 类里有「很大的数据」 | 成员用 `vector`/`string`，类当小变量 |
| 对象本身必须很大还放栈 | 先改成员，少用 `new` 整个类 |
| 长期挂着、还要当基类指针（Pass） | `make_unique`，交给 PM |
| 借用（`HardwareInfo*`、`Operation*`） | 普通指针，不 `delete` |
| C API 裸指针 | 按文档 `free`/`Destroy` |
| `unique_ptr` 之间交接 | `std::move` 那个 `unique_ptr` |

**默认当变量；必须堆且你拥有再用 `unique_ptr`；`move` 只移交里面的堆资源，不改变对象住在栈还是堆。**
