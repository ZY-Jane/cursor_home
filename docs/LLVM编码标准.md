# LLVM 编码标准

本文是 [LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html) 的中文译本，供阅读使用。

- 原文：<https://llvm.org/docs/CodingStandards.html>
- 原文许可：Apache License v2.0 with LLVM Exceptions，见 <https://llvm.org/LICENSE.txt>
- 说明：这不是 LLVM 官方译本。规则以英文原文为准。代码示例保持原文，便于对照。LLVM 要求源码里的注释使用英文。

## 目录

- [引言](#引言)
- [语言、库与标准](#语言库与标准)
- [机械性的源码问题](#机械性的源码问题)
- [语言与编译器问题](#语言与编译器问题)
- [风格问题](#风格问题)
- [细枝末节](#细枝末节)
- [另见](#另见)

## 引言

本文描述 LLVM 项目使用的编码标准。任何编码标准都不应被当成所有场合都必须遵守的绝对要求，但对采用库式设计的大型代码库（例如 LLVM）来说，编码标准尤其重要。

本文会给一些机械的格式、空白或其他「微观细节」提供指导，但这些并不是不可改变的标准。始终遵守这条黄金规则：

> 如果你是在扩展、增强或修复已经写好的代码，就沿用那里已经在用的风格，让源码保持统一、容易顺着读下去。

有些代码库（例如 `libc++`）有特殊理由偏离这些标准。以 `libc++` 为例，命名和其他约定是由 C++ 标准规定的。

代码库里有些约定并没有被一律遵守（例如命名约定）。这是因为它们出现得比较晚，大量代码写于这些约定确立之前。长期目标是让整个代码库都遵循这些约定，但我们明确不希望有补丁去对现有代码做大规模重新排版。另一方面，如果你正要因为别的原因改一个类，顺手重命名它的方法是合理的。请把这类改动单独提交，方便代码审查。

这些准则的最终目的，是提高我们共同源码库的可读性和可维护性。

## 语言、库与标准

LLVM 以及采用这些编码标准的其他 LLVM 项目，大部分源码是 C++。有些地方会用 C，原因包括环境限制、历史限制，或是引入了第三方源码。总体而言，我们优先选择符合标准、现代、可移植的 C++ 作为实现语言。

自动化、构建系统和工具脚本优先使用 Python。LLVM 仓库里已经广泛使用 Python。

### C++ 标准版本

除非另有文档说明，LLVM 子项目使用标准 C++17，并避免不必要的、特定厂商的扩展。

尽管如此，我们只使用作为宿主编译器受支持的那些主要工具链里已经提供的特性（见 Getting Started with the LLVM System 页面的 Software 一节）。

各工具链对自己接受哪些特性都有较好的参考：

- Clang：<https://clang.llvm.org/cxx_status.html>
  - libc++：<https://libcxx.llvm.org/Status/Cxx17.html>
- GCC：<https://gcc.gnu.org/projects/cxx-status.html#cxx17>
  - libstdc++：<https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2017>
- MSVC：<https://learn.microsoft.com/cpp/overview/visual-cpp-language-conformance>

另外，cppreference.com 上有各编译器对 C++ 特性支持情况的对照表：<https://en.cppreference.com/w/cpp/compiler_support/17>。

为了跟上标准的演进，可以用更新的 C++ 版本来构建 LLVM。但我们的支持重点是最低受支持的 C++ 版本。很新的标准可能尚未得到支持，或者只能在受支持工具链的最新版本上使用，而且未必覆盖所有子项目。

### C++ 标准库

只要某项任务已有现成设施，我们就鼓励使用 C++ 标准库或 LLVM 支持库，而不是自己实现数据结构。LLVM 及相关项目尽可能依赖并强调标准库设施和 LLVM 支持库。

LLVM 支持库（例如 ADT）实现标准库里没有的专用数据结构或功能。这类库通常放在 `llvm` 命名空间中，并且在存在对应标准接口时遵循该接口。

当 C++ 标准库和 LLVM 支持库提供相近功能，又没有具体理由偏爱 C++ 实现时，一般优先使用 LLVM 的库。例如，几乎总是应该用 `llvm::DenseMap`，而不是 `std::map` 或 `std::unordered_map`；通常应该用 `llvm::SmallVector`，而不是 `std::vector`。

我们明确避开某些标准设施，例如输入输出流，改用 LLVM 的流库（`raw_ostream`）。更详细的说明见 LLVM Programmer's Manual。

关于 LLVM 数据结构及其取舍，请查阅程序员手册中的相应章节。

### Python 版本与源码格式

当前要求的最低 Python 版本记录在 Getting Started with the LLVM System 一节。LLVM 仓库中的 Python 代码只能使用该版本已经提供的语言特性。

LLVM 仓库中的 Python 代码应遵守 PEP 8 中的格式准则。

为了保持一致并减少无谓变动，代码应使用 black 自动排版。black 符合 PEP 8。使用它的默认规则。例如，不要指定 `--line-length`，即使默认值并不是 80。默认规则会在 black 的大版本之间变化。为避免格式规则带来不必要的变动，LLVM 目前使用 black 23.x。

提交一个与排版无关的补丁时，只排版该补丁修改过的 Python 代码。为此使用 darker：它只对改过的 Python 代码应用 black 的默认规则。这样补丁应能通过 LLVM 预提交 CI 中的 Python 格式检查，该检查同样使用 darker。如果补丁本身就是为了重新排版 Python 文件，则使用 black。black 目前只支持排版整个文件。

下面是几个简例，细节见 black 和 darker 的文档：

```bash
$ pip install black=='23.*' darker # 安装 black 23.x 和 darker
$ darker test.py                   # 排版未提交的改动
$ darker -r HEAD^ test.py          # 同时排版上一次提交的改动
$ black test.py                    # 排版整个文件
```

也可以把目录交给 darker，它会找出其中改过的文件，而不必逐个写文件名。但如果目录很大，比如整个 LLVM 仓库的克隆，darker 会慢得难以忍受。这时可以用 git 列出改过的文件，例如：

```bash
$ darker -r HEAD^ $(git diff --name-only --diff-filter=d HEAD^)
```

## 机械性的源码问题

### 源码格式

#### 注释

注释对可读性和可维护性很重要。写注释时，把它写成英文散文，使用正确的大小写和标点。目标是说明代码想做什么、为什么这样做，而不是在微观层面上说明它如何做。下面几项尤其应该写清楚。

##### 文件头

每个源文件都应该有一段说明该文件基本用途的文件头。标准文件头如下：

```cpp
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instruction class, which is the
/// base class for all of the VM instructions.
///
//===----------------------------------------------------------------------===//
```

文件开头第一段是一句简短说明，写明该文件以何种许可发布。这让源码可以按什么条款分发变得完全清楚，这段文字不应以任何方式修改。

主体是一条 Doxygen 注释（用 `///` 而不是普通的 `//`），描述文件的用途。第一句（或以 `\brief` 开头的一段）会被当作摘要。其余信息应用空行隔开。如果算法基于某篇论文或在别处有描述，请给出引用。

##### 头文件保护宏

头文件的保护宏应当是使用者 `#include` 该头文件时所用路径的全大写形式，路径分隔符和扩展名标记换成 `_`。例如，头文件 `llvm/include/llvm/Analysis/Utils/Local.h` 会被写成 `#include "llvm/Analysis/Utils/Local.h"`，因此它的保护宏是 `LLVM_ANALYSIS_UTILS_LOCAL_H`。

##### 类的概述

类是面向对象设计的基本部分。因此，类定义应当有一段注释，说明这个类用来做什么、如何工作。每个非平凡的类都应有一块 `doxygen` 注释。

##### 方法信息

方法和全局函数也应当有文档。这里只需要简短说明它做什么，并描述边界情况。读者应当不必阅读实现本身，就能明白如何使用这些接口。

这里适合写的是意外情况会发生什么。例如，该方法会返回空指针吗？

#### 注释格式

一般优先使用 C++ 风格注释（普通注释用 `//`，`doxygen` 文档注释用 `///`）。不过有几种情况适合使用 C 风格（`/* */`）注释：

1. 编写要与 C89 兼容的 C 代码时。
2. 编写可能被 C 源文件 `#include` 的头文件时。
3. 编写只接受 C 风格注释的工具所使用的源文件时。
4. 说明一次调用里作为实参的常量有什么意义时。这对 `bool` 参数，或传入 `0`、`nullptr` 最有帮助。注释应包含参数名，而且参数名本身应该有意义。例如，下面这次调用看不出参数是什么意思：

```cpp
Object.emitName(nullptr);
```

一行内的 C 风格注释能把意图说清楚：

```cpp
Object.emitName(/*Prefix=*/nullptr);
```

不鼓励把大块代码注释掉。如果确实必须这样做（为了文档，或作为调试打印的建议），使用 `#if 0` 和 `#endif`。它们能正确嵌套，总体行为也比 C 风格注释更好。

#### 文档注释中的 Doxygen 用法

用 `\file` 命令把标准文件头变成文件级注释。

为所有公开接口（公开的类、成员函数和非成员函数）写描述性段落。避免重复那些从 API 名称或签名就能推断出的信息。第一句（或以 `\brief` 开头的一段）会被当作摘要。尽量用一句话作为摘要，因为 `\brief` 会增加视觉上的杂乱。把详细讨论放到单独的段落里。

一条最简文档注释：

```cpp
/// Sets the xyzzy property to \p Baz.
void setXyzzy(bool Baz);
```

只有当代码示例、函数参数和返回值能提供额外信息（例如意图、用法或不明显的行为）时才写它们。尽可能用有描述性的函数名和参数名，从而不必再写文档注释。

在段落中提及参数名时，使用 `\p name` 命令。不要使用 `\arg name`，因为它会新开一段，专门写该参数的文档。

非行内的代码示例用 `\code ... \endcode` 包起来。

要文档化一个函数参数，用 `\param name` 命令新开一段。如果参数是输出参数或输入输出参数，分别使用 `\param [out] name` 或 `\param [in,out] name`。

要描述函数返回值，用 `\returns` 命令新开一段。

下面是一条按推荐方式用上各种 Doxygen 功能的文档注释：

```cpp
/// Does foo and bar.
///
/// Does not do foo the usual way if \p Baz is true.
///
/// Typical usage:
/// \code
///   fooBar(false, "quux", Res);
/// \endcode
///
/// \param Quux kind of foo to do.
/// \param [out] Result filled with bar sequence on foo success.
///
/// \returns true on success.
bool fooBar(bool Baz, StringRef Quux, std::vector<int> &Result);
```

不要在头文件和实现文件里重复同一条文档注释。公开 API 的文档注释放在头文件里。私有 API 的文档注释可以放在实现文件里。无论哪种情况，实现文件都可以另加注释（不必使用 Doxygen 标记）来解释所需的实现细节。

不要在注释开头重复函数名或类名。对人来说，正在文档化的是哪个函数或类一目了然；自动文档工具也足够聪明，能把注释绑定到正确的声明上。

避免这样写：

```cpp
// Example.h:

// example - Does something important.
void example();

// Example.cpp:

// example - Does something important.
void example() { ... }
```

推荐这样写：

```cpp
// Example.h:

/// Does something important.
void example();

// Example.cpp:

/// Builds a B-tree in order to do foo.  See paper by...
void example() { ... }
```

#### 错误与警告信息

清晰的诊断信息能帮助用户发现并修复输入中的问题。使用简练但正确的英文散文，给出用户理解出错原因所需的上下文。另外，为了与其他工具常见的错误信息风格一致，第一句以小写字母开头；如果最后一句本来会以句号结尾，则不要加句号。以其他标点结尾的句子仍应保留该标点，例如 “did you forget ‘;’?”。

例如，这是一条好的错误信息：

```text
error: file.o: section header 3 is corrupt. Size is 10 when it should be 20
```

这是一条差的信息，因为它没有提供有用信息，风格也不对：

```text
error: file.o: Corrupt section header.
```

和其他编码标准一样，个别项目（例如 Clang Static Analyzer）可能已有不符合本条的既有风格。如果整个项目一致地使用另一种格式，就沿用那种风格。否则，本标准适用于所有 LLVM 工具，包括 clang、clang-tidy 等。

如果工具或项目还没有现成的函数来发出警告或错误，使用 `Support/WithColor.h` 中提供的错误和警告处理函数，以便按合适的样式打印，而不是直接打印到 stderr。

使用 `report_fatal_error` 时，信息文字遵循与普通错误信息相同的标准。断言信息和 `llvm_unreachable` 调用不一定要遵循同一风格，因为它们会被自动排版，这些准则可能并不适合。

#### `#include` 风格

紧接头文件注释之后（如果是头文件，还要接在头文件保护宏之后），列出该文件所需的最少 `#include`。我们希望这些 `#include` 按以下顺序排列：

1. 主模块头文件
2. 本地 / 私有头文件
3. LLVM 项目 / 子项目头文件（`clang/...`、`lldb/...`、`llvm/...` 等）
4. 系统 `#include`

每一类内部按完整路径的字典序排序。

主模块头文件适用于实现某个 `.h` 所定义接口的 `.cpp` 文件。无论这个头文件在文件系统的什么位置，这条 `#include` 都应当始终放在第一位。在实现接口的 `.cpp` 里首先包含该头文件，可以保证头文件没有隐藏依赖：那些依赖没有在头文件里显式 `#include`，但本应包含。这同时也是 `.cpp` 文件中的一种文档，标明它所实现的接口定义在哪里。

LLVM 项目和子项目的头文件应按从最具体到最不具体的顺序分组，理由同上。例如，LLDB 同时依赖 clang 和 LLVM，clang 又依赖 LLVM。因此 LLDB 源文件应先包含 `lldb` 头文件，再包含 `clang` 头文件，最后包含 `llvm` 头文件，以降低这样的可能性：某个 LLDB 头文件因为主源文件或更早的头文件已经包含过另一个头文件，而意外补上了自己漏掉的 include。clang 同样应先包含自己的头文件，再包含 llvm 的头文件。这条规则适用于所有 LLVM 子项目。

#### 源码宽度

把代码写得不超过 80 列。

代码宽度必须有某个上限，这样开发者才能在普通显示器上并排打开多个文件。宽度上限的具体数字多少有些随意，但既然要选，就不如选一个通行的数字。改成 90 列（举例）并不会比 80 列增加什么实质性好处，还会不利于把代码打印出来。另外，许多其他项目已经把 80 列定为标准，有些人的编辑器也已经按 80 列而不是 90 列配置好了。

文档文件不是源码文件。它们不必塞进 80 列，而必须排成一行一句。这样，段落中间的一处修改不会牵动后面各行，审查者在文档更新时更容易看出改了什么。

另一个例外是 TableGen 文件（`.td`）。没有工具能解析、校验并按我们的编码风格编辑它们，clang-format 也不行，因此它们不受这一宽度限制。尽量把它们写进 80 列仍然是好习惯，但不是要求。和其他改动一样，如果修改 TD 文件，不要重新排版整个文件，只改你正在改的那些行。

如果你的重新排版导致后续行出现不必要的变化，请直接沿用现有格式。TD 文件中不必要的改动会让文件历史无谓地变动，并迫使许多不相关的文件重新编译。它也会给正在改同一批 TD 文件的分支带来不必要的差异，使以后的变基和合并更困难。

#### 空白

任何情况下，源文件里都优先使用空格而不是制表符。每个人喜欢的缩进宽度和缩进风格可以不同，这没问题。有问题的是不同编辑器或查看器把制表符展开到不同的制表位。这会让代码看起来完全无法阅读，不值得为此折腾。

一如既往，遵守上面的黄金规则：修改和扩展已有代码时，沿用现有风格。

不要添加行尾空白。有些常见编辑器保存文件时会自动删掉行尾空白，从而在 diff 和提交里出现无关改动。

##### 把 lambda 排得像一块代码

排版跨越多行的 lambda 时，把它排得像一块代码。如果一条语句里只有一个多行 lambda，并且在该语句中它后面没有别的表达式，就把缩进降到代码块标准的两个空格，就好像它是由语句前半部分打开的一个 if 块：

```cpp
std::sort(foo.begin(), foo.end(), [&](Foo a, Foo b) -> bool {
  if (a.blah < b.blah)
    return true;
  if (a.baz < b.baz)
    return true;
  return a.bam < b.bam;
});
```

为了充分利用这种排版，如果你设计的 API 接受一个延续或单个可调用参数（函数对象或 `std::function`），只要可能，就把它放在最后一个参数。

如果一条语句里有多个多行 lambda，或者 lambda 后面还有其他参数，就把代码块相对 `[]` 的缩进再缩进两个空格：

```cpp
dyn_switch(V->stripPointerCasts(),
           [] (PHINode *PN) {
             // process phis...
           },
           [] (SelectInst *SI) {
             // process selects...
           },
           [] (LoadInst *LI) {
             // process loads...
           },
           [] (AllocaInst *AI) {
             // process allocas...
           });
```

##### 花括号初始化列表

从 C++11 起，用花括号列表做初始化的场合多了很多。例如，可以用它们在表达式里构造聚合临时对象。它们现在很自然地会彼此嵌套，也会嵌在函数调用里，以便用局部变量拼出聚合（例如选项结构体）。

过去常见的、给聚合变量做花括号初始化的排版，和深层嵌套、一般的表达式上下文、函数参数以及 lambda 混在一起时并不干净。我们建议新代码用一条简单规则来排版花括号初始化列表：把花括号当成函数调用里的圆括号。排版规则与人们已经熟悉的嵌套函数调用完全相同。例如：

```cpp
foo({a, b, c}, {1, 2, 3});

llvm::Constant *Mask[] = {
    llvm::ConstantInt::get(llvm::Type::getInt32Ty(getLLVMContext()), 0),
    llvm::ConstantInt::get(llvm::Type::getInt32Ty(getLLVMContext()), 1),
    llvm::ConstantInt::get(llvm::Type::getInt32Ty(getLLVMContext()), 2)};
```

这种排版也特别容易用 Clang Format 之类的工具得到可预期、一致且自动的格式。见 <https://clang.llvm.org/docs/ClangFormat.html>。

## 语言与编译器问题

### 把编译器警告当成错误

编译器警告往往有用，能帮助改进代码。没用的警告通常可以用一小处代码改动压掉。例如，在 `if` 条件里赋值常常是笔误：

```cpp
if (V = getValue()) {
  ...
}
```

好几家编译器都会对上面的代码发出警告。加上括号可以压掉它：

```cpp
if ((V = getValue())) {
  ...
}
```

### 编写可移植的代码

几乎在所有情况下，都可以写出完全可移植的代码。当你必须依赖不可移植的代码时，把它放在一个定义清楚、文档清楚的接口后面。

### 不要使用 RTTI 或异常

为了减小代码和可执行文件的体积，LLVM 不使用异常或 RTTI（运行时类型信息，例如 `dynamic_cast<>`）。

尽管如此，LLVM 大量使用一套手写的 RTTI，用的是 `isa<>`、`cast<>` 和 `dyn_cast<>` 这类模板。这套 RTTI 是可选加入的，可以加到任何一个类上。

### 优先使用 C++ 风格的类型转换

做类型转换时，使用 `static_cast`、`reinterpret_cast` 和 `const_cast`，而不是 C 风格转换。有两个例外：

- 转换为 `void` 以抑制未使用变量的警告时（这是 `[[maybe_unused]]` 的一种替代）。这种情况优先使用 C 风格转换。注意：如果变量未使用只是因为它只出现在 `assert` 里，应使用 `[[maybe_unused]]`，而不是 C 风格的 void 转换。
- 在整数类型之间转换时（包括非强类型的枚举），允许用函数式转换作为 `static_cast` 的替代。

### 不要使用静态构造函数

不应往代码库里加入静态构造函数和析构函数（例如，类型带有构造函数或析构函数的全局变量），并且应尽可能把已有的删掉。全局常量（包括在函数内定义的静态变量）应尽可能写成 `constexpr`，以免在启动时无意中产生开销。

```cpp
// Avoid: this will cause the string to be constructed in a global constructor
// and be destructed in a global destructor. The same applies for global/static
// maps, sets, etc.
static std::string VeryBad = "abc123";

struct BadWrapInt {
  int x;
  BadWrapInt(int x) : x(x) {}
};
// Avoid: the BadWrapInt constructor is not constexpr and this will cause the
// initialization of Bad in a global constructor.
static const BadWrapInt Bad(1);

struct GoodWrapInt {
  int x;
  constexpr GoodWrapInt(int x) : x(x) {}
};
// Ok.
static constexpr GoodWrapInt Good(1);
```

不同源文件中的全局对象按不确定的顺序初始化，使代码更难推理。

静态构造函数会拖慢把 LLVM 当作库来使用的程序的启动时间。我们非常希望：把一个额外的 LLVM 目标或其他库链接进应用程序的成本为零。静态构造函数会破坏这个目标。

### 避免在全局或静态常量里放指针

应避免包含非空指针的静态常量，对大型或频繁使用的数据结构尤其如此。

当 LLVM 被构建为位置无关代码时，所有包含指针的静态常量都要在每次启动时重定位，从而增加启动时间和内存占用。

```cpp
// Avoid: StringRef stores pointers to string; this array needs to be touched
// on every startup (typically 32 bytes).
static constexpr StringRef Bad1[] = {"abc", "def"};

// Avoid: likewise (typically 16 bytes).
static const char *const Bad2[] = {"abc", "def"};

// Avoid: this is a mutable array!
static const char *ReallyBad[] = {"abc", "def"};

// Ok.
static constexpr char OkStorage[] = "\0abc\0def";
StringRef getOk(StringTable::Offset Off) { return StringTable(OkStorage)[Off]; }
// Avoid: StringTable itself is just a wrapper around StringRef.
extern constexpr StringTable Bad3(OkStorage);

// Avoid.
struct KeyValue {
  const char *Key;
  unsigned Value;
};
static constexpr KeyValue KVs[] = {{"abc", 1}, {"def", 2}};

// Ok.
constexpr EnumStringDef<unsigned> KVDefs[] = {{{"abc"}, 1}, {{"def"}, 2}};
static constexpr auto KVs = BUILD_ENUM_STRINGS(KVDefs);
```

### `class` 与 `struct` 关键字的使用

在 C++ 里，`class` 和 `struct` 几乎可以互换。唯一的差别出现在用它们声明类时：`class` 默认让所有成员为私有，`struct` 默认让所有成员为公有。

- 同一个 `class` 或 `struct` 的所有声明和定义必须使用同一个关键字。例如：

```cpp
// Avoid if `Example` is defined as a struct.
class Example;

// OK.
struct Example;

struct Example { ... };
```

- 当所有成员都声明为公有时，应使用 `struct`。

```cpp
// Avoid using `struct` here, use `class` instead.
struct Foo {
private:
  int Data;
public:
  Foo() : Data(0) { }
  int getData() const { return Data; }
  void setData(int D) { Data = D; }
};

// OK to use `struct`: all members are public.
struct Bar {
  int Data;
  Bar() : Data(0) { }
};
```

### 不要用花括号初始化列表去调用构造函数

从 C++11 起有一种「通用初始化语法」，允许用花括号初始化列表调用构造函数。如果构造函数含有不平凡的逻辑，或者你在意自己调用的是某一个特定构造函数，就不要这样用。那些调用看起来应该像使用圆括号的函数调用，而不是像聚合初始化。同样，如果需要显式写出类型并调用其构造函数来创建临时对象，不要用花括号初始化列表。花括号初始化列表（临时对象不写类型）用于聚合初始化，或概念上等价的事情。例如：

```cpp
class Foo {
public:
  // Construct a Foo by reading data from the disk in the whizbang format, ...
  Foo(std::string filename);

  // Construct a Foo by looking up the Nth element of some global data ...
  Foo(int N);

  // ...
};

// The Foo constructor call is reading a file, don't use braces to call it.
llvm::fill(foo, Foo("name"));

// The pair is being constructed like an aggregate, use braces.
bar_map.insert({my_key, my_value});
```

用花括号初始化列表初始化变量时，在开括号前加上等号：

```cpp
int data[] = {0, 1, 2, 3};
```

### 用 `auto` 类型推导让代码更好读

有人主张在 C++11 里采取「几乎总是用 `auto`」的策略；LLVM 的立场更温和。当且仅当 `auto` 让代码更好读或更好维护时才使用它。不要「几乎总是」使用 `auto`，但在 `cast<...>(...)` 这类初始化，以及其他类型已从上下文显而易见的地方，确实应该使用 `auto`。另一类很适合的情况是类型本来就会被抽象掉，常见于容器的 typedef 之后，例如 `std::vector<T>::iterator`。

类似地，C++14 增加了泛型 lambda 表达式，参数类型可以是 `auto`。在你本来会使用模板的地方使用它们。

### 当心 `auto` 带来的不必要拷贝

`auto` 用起来方便，容易让人忘记它的默认行为是拷贝。尤其在基于范围的 `for` 循环里，不小心的拷贝代价很高。

除非你需要制作一份拷贝，否则对值使用 `auto &`，对指针使用 `auto *`。

```cpp
// Typically there's no reason to copy.
for (const auto &Val : Container) observe(Val);
for (auto &Val : Container) Val.change();

// Remove the reference if you really want a new copy.
for (auto Val : Container) { Val.change(); saveSomewhere(Val); }

// Copy pointers, but make it clear that they're pointers.
for (const auto *Ptr : Container) observe(*Ptr);
for (auto *Ptr : Container) Ptr->change();
```

### 当心指针顺序带来的非确定性

一般而言，指针之间没有相对顺序。因此，当无序容器（如 set 和 map）用指针做键时，迭代顺序是未定义的。迭代这类容器可能导致不确定的代码生成。生成的代码也许仍能正确工作，但非确定性会让缺陷更难复现，编译器也更难调试。

如果期望有序的结果，记得在迭代前对无序容器排序。或者，如果要迭代指针键，使用 `vector`、`MapVector`、`SetVector` 这类有序容器。

### 当心相等元素排序顺序的非确定性

`std::sort` 使用不稳定的排序算法，不保证相等元素的原有顺序被保留。因此，对含有相等元素的容器使用 `std::sort` 可能导致非确定性行为。为了揭露这类非确定性，LLVM 引入了包装函数 `llvm::sort`。在 `EXPENSIVE_CHECKS` 构建中，它会在排序前随机打乱容器。默认使用 `llvm::sort`，而不是 `std::sort`。

## 风格问题

### 高层问题

#### 自包含的头文件

头文件应当自包含（能够单独编译），并以 `.h` 结尾。打算被包含、但不是头文件的文件应以 `.inc` 结尾，并且少用。

所有头文件都应当自包含。使用者和重构工具不应为了包含这个头文件而遵守特殊条件。具体来说，头文件应有保护宏，并包含它自己需要的所有其他头文件。

少数情况下，一个设计来被包含的文件并不是自包含的。它们通常打算被包含在不寻常的位置，例如另一个文件的中间。它们可能不用保护宏，也可能不包含自己的前置依赖。给这类文件使用 `.inc` 扩展名。尽量少用，能用自包含头文件时就用自包含头文件。

一般而言，一个头文件应由一个或多个 `.cpp` 文件实现。每个这样的 `.cpp` 文件都应首先包含定义其接口的那个头文件。这保证头文件的所有依赖都已正确加进头文件本身，而不是隐含的。在一个翻译单元里，系统头文件应放在用户头文件之后。

#### 库的分层

一个头文件目录（例如 `include/llvm/Foo`）定义一个库（`Foo`）。一个库（包括它的头文件和实现）只应使用其依赖列表中那些库里的东西。

这条约束有一部分可以由经典 Unix 链接器强制执行（Mac 和 Windows 的链接器以及 lld 并不强制这一点）。Unix 链接器从左到右搜索命令行上指定的库，并且不会回头再看一个库。这样一来，库之间就不能存在循环依赖。

这并不能完全强制所有库间依赖，更重要的是，它不能强制由内联函数造成的头文件循环依赖。判断「分层是否正确」的一个好办法是：假如所有内联函数都改成在类外定义，Unix 链接器能否成功链接这个程序。（并且对所有合法的依赖顺序都要成立。因为链接解析是线性的，有些隐含依赖可能混过去：A 依赖 B 和 C，于是合法顺序是 “C B A” 或 “B C A”，两种情况下显式依赖都出现在使用之前。但在第一种顺序里，如果 B 隐含依赖 C，它仍可能链接成功；第二种顺序则相反。）

#### 尽可能少 `#include`

`#include` 伤害编译时间。除非必须，否则不要包含，在头文件里尤其如此。

不过且慢：有时你需要一个类的定义才能使用它，或才能继承它。这些情况下，去 `#include` 那个头文件即可。但也要知道，很多时候你并不需要一个类的完整定义。如果你只用到某个类的指针或引用，就不需要那个头文件。如果你只是从一个已有原型的函数或方法返回一个类的实例，也不需要它。事实上，在大多数情况下，你根本不需要一个类的定义。不 `#include` 能加快编译。

然而，这条建议也很容易做过了头。你正在使用的所有头文件都必须包含进来，可以直接包含，也可以经由另一个头文件间接包含。为了确保不会意外忘记在模块头文件里包含某个头文件，请在实现文件里首先包含你的模块头文件（如前所述）。这样以后就不会发现隐藏依赖。

#### 让「内部」头文件保持私有

许多模块的实现比较复杂，因此会使用不止一个实现（`.cpp`）文件。人们常常想把内部通信接口（辅助类、额外函数等）放进公开的模块头文件。不要这样做。

如果确实需要这样做，把私有头文件放在与源文件相同的目录里，并在本地包含它。这保证你的私有接口保持私有，不受外部打扰。

把额外的实现方法放在公开类里面是可以的。只要把它们做成私有（或保护）即可。

#### 用命名空间限定符定义先前已声明的符号

在源文件里为各种符号（变量、函数、不透明类）提供类外定义时，不要在源文件里打开命名空间块。改用命名空间限定符，以帮助确保你的定义与已有声明匹配。这样做：

```cpp
// Foo.h
namespace llvm {
extern int FooVal;
int foo(const char *s);

namespace detail {
class FooImpl;
} // namespace detail
} // namespace llvm

// Foo.cpp
#include "Foo.h"
using namespace llvm;

int llvm::FooVal;

int llvm::foo(const char *s) {
  // ...
}

class detail::FooImpl {
  // ...
}
```

这样做有助于避免定义与头文件中的声明不匹配。例如，下面的 C++ 代码定义了 `llvm::foo` 的一个新重载，而不是为头文件里已声明的那个函数提供定义：

```cpp
// Foo.cpp
#include "Foo.h"
namespace llvm {
int foo(char *s) { // Mismatch between "const char *" and "char *"
}
} // namespace llvm
```

这个错误要到构建将近结束、链接器找不到原函数任何使用点的定义时才会被发现。如果改为用命名空间限定符来定义，错误会在编译这个定义时立刻被抓住。

类方法的实现必须已经写上类名，而且不能在类外引入新的重载，因此本建议不适用于它们。

#### 用提前返回和 `continue` 简化代码

读代码时，要留意读者为了理解一块代码，必须记住多少状态和多少先前的判断。在不让代码更难理解的前提下，尽量减少缩进。一个很好的办法是在长循环里使用提前返回和 `continue`。看这段没有提前返回的代码：

```cpp
Value *doSomething(Instruction *I) {
  if (!I->isTerminator() &&
      I->hasOneUse() && doOtherThing(I)) {
    ... some long code ....
  }

  return 0;
}
```

如果 `if` 的函数体很大，这段代码有几个问题。看函数开头时，并不能立刻看出它只对非终结指令做有意思的事，并且还要满足其他谓词。其次，这些谓词为什么重要相对难写进注释，因为 `if` 语句让注释不好排布。第三，深入代码体内部时，会多一层缩进。最后，在函数开头看不出谓词不成立时结果是什么；你必须读到函数末尾，才知道它返回空指针。

更推荐排成这样：

```cpp
Value *doSomething(Instruction *I) {
  // Terminators never need 'something' done to them because ...
  if (I->isTerminator())
    return 0;

  // We conservatively avoid transforming instructions with multiple uses
  // because goats like cheese.
  if (!I->hasOneUse())
    return 0;

  // This is really just here for example.
  if (!doOtherThing(I))
    return 0;

  ... some long code ....
}
```

这样就解决了上述问题。`for` 循环里经常出现类似问题。一个随手举的例子是：

```cpp
for (Instruction &I : BB) {
  if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
    Value *LHS = BO->getOperand(0);
    Value *RHS = BO->getOperand(1);
    if (LHS != RHS) {
      ...
    }
  }
}
```

循环非常非常小时，这种结构没问题。但一旦超过大约 10 到 15 行，人们就很难一眼读懂。这种代码的问题是它很快就会嵌得很深。这意味着读者必须在脑子里保持大量上下文，才能记得循环里眼下正在发生什么，因为他们不知道这些 `if` 条件会不会有 `else`。强烈推荐把循环写成这样：

```cpp
for (Instruction &I : BB) {
  auto *BO = dyn_cast<BinaryOperator>(&I);
  if (!BO) continue;

  Value *LHS = BO->getOperand(0);
  Value *RHS = BO->getOperand(1);
  if (LHS == RHS) continue;

  ...
}
```

这和函数里使用提前返回的好处一样：减少循环的嵌套，更容易说明条件为什么成立，并且让读者一眼看出后面不会冒出一个他们还得往脑子里压上下文的 `else`。如果循环很大，这对可理解性是很大的收获。

#### `return` 之后不要再写 `else`

出于和上面类似的理由（减少缩进、更容易阅读），请不要在打断控制流的语句之后使用 `else` 或 `else if`。这类语句包括 `return`、`break`、`continue`、`goto` 等。例如：

```cpp
case 'J': {
  if (Signed) {
    Type = Context.getsigjmp_bufType();
    if (Type.isNull()) {
      Error = ASTContext::GE_Missing_sigjmp_buf;
      return QualType();
    } else {
      break; // Unnecessary.
    }
  } else {
    Type = Context.getjmp_bufType();
    if (Type.isNull()) {
      Error = ASTContext::GE_Missing_jmp_buf;
      return QualType();
    } else {
      break; // Unnecessary.
    }
  }
}
```

更好的写法是：

```cpp
case 'J':
  if (Signed) {
    Type = Context.getsigjmp_bufType();
    if (Type.isNull()) {
      Error = ASTContext::GE_Missing_sigjmp_buf;
      return QualType();
    }
  } else {
    Type = Context.getjmp_bufType();
    if (Type.isNull()) {
      Error = ASTContext::GE_Missing_jmp_buf;
      return QualType();
    }
  }
  break;
```

在这个例子里，还可以再好一些：

```cpp
case 'J':
  if (Signed)
    Type = Context.getsigjmp_bufType();
  else
    Type = Context.getjmp_bufType();

  if (Type.isNull()) {
    Error = Signed ? ASTContext::GE_Missing_sigjmp_buf :
                     ASTContext::GE_Missing_jmp_buf;
    return QualType();
  }
  break;
```

想法是减少缩进，并减少阅读代码时必须跟踪的代码量。

注意：这条建议不适用于 `constexpr if`。`else` 子句中的语句可能是被丢弃的语句，去掉 `else` 可能导致意外的模板实例化。因此下面的例子是正确的：

```cpp
template<typename T>
static constexpr bool VarTempl = true;

template<typename T>
int func() {
  if constexpr (VarTempl<T>)
    return 1;
  else
    static_assert(!VarTempl<T>);
}
```

#### 把谓词循环变成谓词函数

人们很常写只为算出一个布尔值的小循环。写法有很多种，下面是其中一种：

```cpp
bool FoundFoo = false;
for (unsigned I = 0, E = BarList.size(); I != E; ++I)
  if (BarList[I]->isFoo()) {
    FoundFoo = true;
    break;
  }

if (FoundFoo) {
  ...
}
```

我们更希望用一个谓词函数（可以是 static）配合提前返回：

```cpp
/// \returns true if the specified list has an element that is a foo.
static bool containsFoo(const std::vector<Bar*> &List) {
  for (unsigned I = 0, E = List.size(); I != E; ++I)
    if (List[I]->isFoo())
      return true;
  return false;
}
...

if (containsFoo(BarList)) {
  ...
}
```

这样做的理由很多：它减少缩进，并把常常可以被其他检查同一谓词的代码共享的逻辑抽出来。更重要的是，它迫使你给函数起一个名字，并迫使你为它写一条注释。在这个随手举的例子里，这增加不了多少价值。但如果条件很复杂，这会让查询该谓词的代码对读者容易理解得多。读者不必面对「我们如何检查 BarList 是否包含 foo」的行内细节，可以信任函数名，带着更好的局部性继续往下读。

### 低层问题

#### 给类型、函数、变量和枚举项起好名字

选得差的名字会误导读者并造成缺陷。描述性的名字有多重要，再怎么强调也不过分。在合理范围内，选择与底层实体的语义和角色相符的名字。除非缩写广为人知，否则避免缩写。选好名字之后，务必对该名字使用一致的大小写。不一致会迫使调用者要么记住 API，要么查一下才能知道确切拼写。

一般而言，名字应使用驼峰形式（例如 `TextFileReader` 和 `isLValue()`）。不同种类的声明有不同规则：

- 类型名（包括类、结构体、枚举、typedef 等）应当是名词，并以大写字母开头（例如 `TextFileReader`）。
- 变量名应当是名词（因为它们表示状态）。名字用驼峰，并以大写字母开头（例如 `Leader` 或 `Boats`）。
- 函数名应当是动词短语（因为它们表示动作），命令式的函数应当用祈使语气。名字用驼峰，并以小写字母开头（例如 `openFile()` 或 `isFoo()`）。
- 枚举声明（例如 `enum Foo {...}`）是类型，因此应遵循类型的命名约定。枚举的一种常见用途是作为联合的判别符，或子类的指示符。用于这类用途时，应带 `Kind` 后缀（例如 `ValueKind`）。
- 枚举项（例如 `enum { Foo, Bar }`）和公有成员变量应像类型一样以大写字母开头。除非枚举项定义在它们自己的小命名空间里或类内部，否则枚举项应带有与枚举声明名对应的前缀。例如，`enum ValueKind { ... };` 可以包含 `VK_Argument`、`VK_BasicBlock` 这样的枚举项。仅仅作为方便常量的枚举项不必加前缀。例如：

```cpp
enum {
  MaxSize = 42,
  Density = 12
};
```

作为例外，模仿 STL 类的类可以使用 STL 风格的成员名：小写单词以下划线分隔（例如 `begin()`、`push_back()` 和 `empty()`）。提供多种迭代器的类，应给 `begin()` 和 `end()` 加上一个单数前缀（例如 `global_begin()` 和 `use_begin()`）。

下面是一些例子：

```cpp
class VehicleMaker {
  ...
  Factory<Tire> F;            // Avoid: a non-descriptive abbreviation.
  Factory<Tire> Factory;      // Better: more descriptive.
  Factory<Tire> TireFactory;  // Even better: if VehicleMaker has more than one
                              // kind of factories.
};

Vehicle makeVehicle(VehicleType Type) {
  VehicleMaker M;                         // Might be OK if scope is small.
  Tire Tmp1 = M.makeTire();               // Avoid: 'Tmp1' provides no information.
  Light Headlight = M.makeLight("head");  // Good: descriptive.
  ...
}
```

#### 慷慨地使用断言

把 `assert` 宏用到极致。检查你的所有前置条件和假设。你永远不知道一个缺陷（甚至不一定是你自己的）会在什么时候被断言提前抓住，这能大幅减少调试时间。你正在使用的头文件大概已经包含了 `<cassert>`，所以使用它没有额外成本。

为了进一步帮助调试，务必在断言语句里放上某种错误信息，断言触发时会打印出来。这帮助倒霉的调试者理解为什么要做并强制这条断言，并且希望能提示该怎么办。下面是一个完整例子：

```cpp
inline Value *getOperand(unsigned I) {
  assert(I < Operands.size() && "getOperand() out of range!");
  return Operands[I];
}
```

更多例子：

```cpp
assert(Ty->isPointerType() && "Can't allocate a non-pointer type!");

assert((Opcode == Shl || Opcode == Shr) && "ShiftInst Opcode invalid!");

assert(idx < getNumSuccessors() && "Successor # out of range!");

assert(V1.getType() == V2.getType() && "Constant types must be identical!");

assert(isa<PHINode>(Succ->front()) && "Only works on PHId BBs!");
```

意思就是这样。

过去，人们用断言表示一段不应当到达的代码。典型形式是：

```cpp
assert(0 && "Invalid radix for integer literal");
```

这有几个问题。主要问题是，有些编译器可能不理解这种断言，或者在断言被编译掉的构建里警告缺少返回值。

现在我们有好得多的东西：`llvm_unreachable`：

```cpp
llvm_unreachable("Invalid radix for integer literal");
```

启用断言时，如果这里被执行到，它会打印信息然后退出程序。禁用断言时（也就是发布构建），`llvm_unreachable` 变成给编译器的提示，让它不为这个分支生成代码。如果编译器不支持这一点，它会退回到 “abort” 实现。

用 `llvm_unreachable` 标记代码中一个永远不应当到达的具体位置。这对处理关于不可达分支的警告特别合适，但只要到达某条代码路径无条件地是某种缺陷（不是来自用户输入；见下文），就可以使用。`assert` 的使用应当始终包含一个可测试的谓词（而不是 `assert(false)`）。

如果错误条件可以由用户输入触发，则应改用 LLVM Programmer's Manual 中描述的可恢复错误机制。在不切实际的情况下，可以使用 `report_fatal_error`。

另一个问题是：只被断言使用的值，在禁用断言时会产生「未使用的值」警告。例如，这段代码会警告：

```cpp
unsigned Size = V.size();
assert(Size > 42 && "Vector smaller than it should be");

bool NewToSet = Myset.insert(Value);
assert(NewToSet && "The value shouldn't be in the set yet");
```

这是两个有意思的不同情况。第一种情况下，对 `V.size()` 的调用只对断言有用，我们不希望在禁用断言时执行它。这类代码应把调用移进断言本身。第二种情况下，无论断言是否启用，这次调用的副作用都必须发生。这时应当用 `[[maybe_unused]]` 属性定义该值，以抑制警告。具体来说，推荐写成：

```cpp
assert(V.size() > 42 && "Vector smaller than it should be");

[[maybe_unused]] bool NewToSet = Myset.insert(Value);
assert(NewToSet && "The value shouldn't be in the set yet");
```

在不支持 `[[maybe_unused]]` 的 C 代码里，用 `void` 转换抑制未使用变量警告，如下：

```cpp
LLVMValueRef Value = LLVMMetadataAsValue(Context, NodeMD);
assert(LLVMIsAValueAsMetadata(Value) != NULL);
(void)Value;
```

#### 不要使用 `using namespace std`

在 LLVM 里，我们希望给标准命名空间中的所有标识符显式加上 `std::` 前缀，而不是依赖 `using namespace std;`。

在头文件里，加上 `using namespace XXX` 会污染任何 `#include` 该头文件的源文件的命名空间，造成维护问题。

在实现文件（例如 `.cpp`）里，这条规则更多是风格问题，但仍然重要。基本上，显式的命名空间前缀让代码更清楚，因为立刻就能看出用的是什么设施、它们从哪里来。它也更可移植，因为 LLVM 代码和其他命名空间之间不会发生名字冲突。可移植性这条很重要，因为不同的标准库实现会暴露不同的符号（有可能是它们不该暴露的），而 C++ 标准的未来修订还会往 `std` 命名空间加入更多符号。因此，我们在 LLVM 里从不使用 `using namespace std;`。

这条一般规则的例外（对 `std` 命名空间来说这并不是例外）是实现文件。例如，LLVM 项目中的代码实现的都是生活在 `llvm` 命名空间里的代码。因此，`.cpp` 文件在 `#include` 之后的顶部写一条 `using namespace llvm;` 是可以的，而且实际上更清楚。对按花括号缩进的源码编辑器来说，这减少了文件主体的缩进，也让概念上的上下文更干净。这条规则的一般形式是：任何实现某个命名空间中代码的 `.cpp` 文件，可以使用该命名空间（及其父命名空间），但不应使用任何其他命名空间。

#### 为头文件中的类提供一个虚方法锚点

如果一个类定义在头文件里并且有虚表（它自己有虚方法，或者它派生自有虚方法的类），该类必须至少有一个写在类外的虚方法。否则，编译器会把虚表和 RTTI 复制到每一个 `#include` 该头文件的 `.o` 文件里，使 `.o` 文件膨胀并增加链接时间。

#### 对已完全覆盖的枚举 switch 不要使用 default 标签

如果一个针对枚举的 switch 没有 default 标签，又没有覆盖每一个枚举值，`-Wswitch` 会警告。如果你在一个已经完全覆盖该枚举的 switch 上写了 default 标签，那么以后往该枚举加入新元素时，`-Wswitch` 就不会再响。为了帮助避免加上这类 default，Clang 有警告 `-Wcovered-switch-default`。它默认关闭，但在用支持该警告的 Clang 版本构建 LLVM 时会打开。

这条风格要求的一个连带效果是：用 GCC 构建 LLVM 时，如果你在一个已覆盖的枚举 switch 的每个 case 里都返回，可能会收到「控制流可能到达非 void 函数末尾」之类的警告，因为 GCC 假定枚举表达式可以取任何可表示的值，而不只是各个枚举项的值。要抑制这个警告，在 switch 之后使用 `llvm_unreachable`。

#### 尽可能使用基于范围的 `for` 循环

C++11 引入基于范围的 `for` 循环之后，显式操作迭代器就很少有必要了。对所有新写的代码，我们尽可能使用基于范围的 `for` 循环。例如：

```cpp
BasicBlock *BB = ...
for (Instruction &I : *BB)
  ... use I ...
```

不鼓励使用 `std::for_each()` / `llvm::for_each()`，除非那个可调用对象已经存在。

#### 不要在每次循环时都求值 `end()`

在不能使用基于范围的 `for`、必须手写基于迭代器的循环时，要密切注意 `end()` 是否在每次迭代时被重新求值。一个常见错误是写成这样：

```cpp
BasicBlock *BB = ...
for (auto I = BB->begin(); I != BB->end(); ++I)
  ... use I ...
```

这种写法的问题是，它每次经过循环都会求值 `BB->end()`。我们强烈希望循环在开始前只求值一次。一种方便的写法是：

```cpp
BasicBlock *BB = ...
for (auto I = BB->begin(), E = BB->end(); I != E; ++I)
  ... use I ...
```

眼尖的人会很快指出，这两个循环的语义可能不同：如果容器（这里是一个基本块）正在被修改，那么 `BB->end()` 每次经过循环都可能改变值，第二个循环实际上可能不正确。如果你确实依赖这种行为，请写成第一种形式，并加注释说明这是有意的。

为什么（在正确的时候）我们更喜欢第二种形式？把循环写成第一种有两个问题。第一，它可能比在循环开始时求值效率更低。在这个例子里，代价大概很小，只是每次循环多几次加载。但如果基表达式更复杂，代价会迅速上升。我见过结束表达式实际上是 `SomeMap[X]->end()` 这样的循环，而 map 查找真的不便宜。坚持写成第二种形式，就把这个问题整个消掉了，甚至不用去想它。

第二个（甚至更大的）问题是：把循环写成第一种，是在向读者暗示这个循环正在修改容器（一条注释可以顺手确认这一点）。如果写成第二种，甚至不用看循环体，就能立刻看出容器没有被修改，从而更容易阅读代码、理解它做什么。

第二种形式只是多敲几个字，但我们确实强烈偏好它。

#### 禁止 `#include <iostream>`

特此禁止在库文件中使用 `#include <iostream>`，因为许多常见实现会悄悄向每一个包含它的翻译单元注入一个静态构造函数。

注意：使用其他流头文件（例如 `<fstream>`）在这方面没有问题，有问题的只是 `<iostream>`。不过，对几乎每一种用途，`raw_ostream` 提供的各种 API 都比 `std::ostream` 风格的 API 性能更好。

新代码写文件应始终使用 `raw_ostream`，读文件应使用 `llvm::MemoryBuffer` API。

#### 使用 `raw_ostream`

LLVM 在 `llvm/Support/raw_ostream.h` 中包含一个轻量、简单、高效的流实现，提供 `std::ostream` 的全部常见功能。所有新代码都应使用 `raw_ostream`，而不是 `ostream`。

与 `std::ostream` 不同，`raw_ostream` 不是模板，可以前向声明为 `class raw_ostream`。公开头文件一般不应包含 `raw_ostream` 头文件，而应使用前向声明和对 `raw_ostream` 实例的常量引用。

#### 避免 `std::endl`

`std::endl` 操纵符与 iostream 一起使用时，会向指定的输出流输出一个换行。但除此之外，它还会刷新输出流。换句话说，下面两句等价：

```cpp
std::cout << std::endl;
std::cout << '\n' << std::flush;
```

大多数时候，你大概没有理由刷新输出流，所以更好的做法是使用字面量 `'\n'`。

#### 在类定义里定义函数时不要写 `inline`

在类定义中定义的成员函数隐含就是 inline 的，因此这种情况不要写 `inline` 关键字。

不要这样：

```cpp
class Foo {
public:
  inline void bar() {
    // ...
  }
};
```

要这样：

```cpp
class Foo {
public:
  void bar() {
    // ...
  }
};
```

## 细枝末节

本节描述偏好的低层排版准则，以及我们为什么偏好它们。

### 括号前的空格

只在控制流语句的开括号前放一个空格，普通函数调用表达式和类似函数的宏则不要。例如：

```cpp
if (X) ...
for (I = 0; I != 100; ++I) ...
while (LLVMRocks) ...

somefunc(42);
assert(3 != 4 && "laws of math are failing me");

A = foo(42, 92) + bar(X);
```

这样做的理由并不完全随意。这种风格让控制流运算符更突出，也让表达式读起来更顺。

### 优先使用前置自增

硬性规则：前置自增（`++X`）不会比后置自增（`X++`）更慢，而且完全可能快很多。只要可能，就使用前置自增。

后置自增的语义包括：复制正在自增的值，返回该副本，然后对「工作值」做前置自增。对基本类型来说这不是大事。但对迭代器来说，这可能是大问题（例如，有些迭代器内部含有栈和集合对象……复制一个迭代器还可能调用这些对象的拷贝构造函数）。一般而言，养成始终使用前置自增的习惯，就不会有问题。

### 命名空间的缩进

一般而言，我们尽可能减少缩进。这既是为了让代码在不过度换行的情况下放进 80 列，也是为了让代码更容易理解。为了做到这一点，并偶尔避免深得离谱的嵌套，不要给命名空间缩进。如果有助于可读性，可以在 `}` 处加注释，标明关闭的是哪个命名空间。例如：

```cpp
namespace llvm {
namespace knowledge {

/// This class represents things that Smith can have an intimate
/// understanding of and contains the data associated with it.
class Grokable {
...
public:
  explicit Grokable() { ... }
  virtual ~Grokable() = 0;

  ...

};

} // namespace knowledge
} // namespace llvm
```

当被关闭的命名空间因为任何原因都显而易见时，可以省掉结束注释。例如，头文件最外层的命名空间很少造成混淆。但源文件中写到一半才关闭的匿名命名空间和具名命名空间，大概需要说明一下。

### 限制可见性

函数和变量的可见性应当尽可能受限。

对类成员来说，这意味着使用合适的 `private`、`protected` 或 `public` 关键字限制访问。

对非成员函数、变量和类来说，这意味着如果它不在那个文件之外被引用，就把可见性限制在单个 `.cpp` 文件内。

文件作用域的非成员变量和函数，可以用 `static` 关键字或匿名命名空间把可见性限制在当前翻译单元。

匿名命名空间是一个很好的语言特性，它告诉 C++ 编译器：该命名空间的内容只在当前翻译单元内可见，从而允许更激进的优化，并消除符号名冲突的可能。

匿名命名空间之于 C++，如同 `static` 之于 C 的函数和全局变量。虽然 C++ 里也可以用 `static`，但匿名命名空间更一般：它们可以让整个类对该文件私有。

匿名命名空间的问题是，它们天然会鼓励给函数体缩进，并且降低引用的局部性：如果你在一个 C++ 文件里看到一个随便的函数定义，很容易看出它有没有标成 static，但要看出它是否在匿名命名空间里，就得扫过文件的一大块。

因此我们有一条简单准则：让匿名命名空间尽可能小，并且只把它们用于类声明。例如：

```cpp
namespace {
class StringSort {
...
public:
  StringSort(...)
  bool operator<(const char *RHS) const;
};
} // namespace

static void runHelper() {
  ...
}

bool StringSort::operator<(const char *RHS) const {
  ...
}
```

避免把类以外的声明放进匿名命名空间：

```cpp
namespace {

// ... many declarations ...

void runHelper() {
  ...
}

// ... many declarations ...

} // namespace
```

当你在一个很大的 C++ 文件中间看 `runHelper` 时，没有办法立刻判断这个函数是不是该文件局部的。

相反，当函数标成 static 时，你不需要到文件远处交叉对照，就能知道这个函数是局部的：

```cpp
static void runHelper() {
  ...
}
```

### 不要给 if / else / 循环中简单的单语句体加大括号

写 `if`、`else` 或 `for` / `while` 循环语句的语句体时，我们力求减少不必要的行内噪音。

在以下情况下省略花括号：

- 语句体由一条简单语句组成。
- 这条语句前面没有注释。（如果可以，把注释提到控制语句上方。）
- 如果有 `else` 子句，它也满足上述条件（一条简单语句，没有配套注释）。

其他所有情况都使用花括号，包括：

- 多条语句的语句体
- 带有无法上提的注释的单语句体
- 复杂的单语句体（例如深层嵌套、复杂的嵌套循环）
- `if` / `else if` / `else` 链中花括号不一致（如果其中一块需要花括号，则全部都要有）
- 以一个没有 `else` 的嵌套 `if` 结尾的 `if` 语句（防止「悬空 else」）

下面的例子为这些情况提供准则：

```cpp
// Omit the braces since the body is simple and clearly associated with the
// `if`.
if (isa<FunctionDecl>(D))
  handleFunctionDecl(D);
else if (isa<VarDecl>(D))
  handleVarDecl(D);

// Here we document the condition itself and not the body.
if (isa<VarDecl>(D)) {
  // It is necessary that we explain the situation with this surprisingly long
  // comment, so it would be unclear without the braces whether the following
  // statement is in the scope of the `if`.
  // Because the condition is documented, we can't really hoist this
  // comment that applies to the body above the `if`.
  handleOtherDecl(D);
}

// Use braces on the outer `if` to avoid a potential dangling `else`
// situation.
if (isa<VarDecl>(D)) {
  if (shouldProcessAttr(A))
    handleAttr(A);
}

// Use braces for the `if` block to keep it uniform with the `else` block.
if (isa<FunctionDecl>(D)) {
  handleFunctionDecl(D);
} else {
  // In this `else` case, it is necessary that we explain the situation with
  // this surprisingly long comment, so it would be unclear without the braces
  // whether the following statement is in the scope of the `if`.
  handleOtherDecl(D);
}

// Use braces for the `else if` and `else` block to keep it uniform with the
// `if` block.
if (isa<FunctionDecl>(D)) {
  verifyFunctionDecl(D);
  handleFunctionDecl(D);
} else if (isa<GlobalVarDecl>(D)) {
  handleGlobalVarDecl(D);
} else {
  handleOtherDecl(D);
}

// This should also omit braces.  The `for` loop contains only a single
// statement, so it shouldn't have braces.  The `if` also only contains a
// single simple statement (the `for` loop), so it also should omit braces.
if (isa<FunctionDecl>(D))
  for (auto *A : D.attrs())
    handleAttr(A);

// Use braces for a `do-while` loop and its enclosing statement.
if (Tok->is(tok::l_brace)) {
  do {
    Tok = Tok->Next;
  } while (Tok);
}

// Use braces for the outer `if` since the nested `for` is braced.
if (isa<FunctionDecl>(D)) {
  for (auto *A : D.attrs()) {
    // In this `for` loop body, it is necessary that we explain the situation
    // with this surprisingly long comment, forcing braces on the `for` block.
    handleAttr(A);
  }
}

// Use braces on the outer block because there are more than two levels of
// nesting.
if (isa<FunctionDecl>(D)) {
  for (auto *A : D.attrs())
    for (ssize_t i : llvm::seq<ssize_t>(count))
      handleAttrOnDecl(D, A, i);
}

// Use braces on the outer block because of a nested `if`; otherwise, the
// compiler would warn: `add explicit braces to avoid dangling else`
if (auto *D = dyn_cast<FunctionDecl>(D)) {
  if (shouldProcess(D))
    handleVarDecl(D);
  else
    markAsIgnored(D);
}
```

### 文件使用 Unix 换行

所有文件使用 Unix 换行。CRLF 换行只在例外情况下允许：测试文件意在测试 CRLF 处理，或文件格式本身要求如此（例如 `.bat` 或 `.rc` 文件）。

## 另见

这些意见和建议有很多是从其他来源整理来的。对我们的工作特别重要的两本书是：

1. Scott Meyers 的《Effective C++》。同一作者的《More Effective C++》和《Effective STL》同样有趣、有用。
2. John Lakos 的《Large-Scale C++ Software Design》。

如果有空闲时间，而且你还没读过：去读吧，你可能会学到东西。
