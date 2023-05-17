---
layout: post
title: 探究 xx 是如何实现的？
date: 2023-05-17
categories: 技术
tags: C++
---

## `LOG(INFO) << some-exprs` 

指定 serverity 的 Log 是如何实现的？如果 severity 不足以打印信息，`some-exprs` 是否会求值导致无效运算？

从 SentencePiece 的代码 [common.h/LOG(severity)](https://github.com/google/sentencepiece/blob/v0.1.99/src/common.h#L141) 中可以看到如下逻辑：

```cpp
#define LOG(severity)                                                        \
  (::sentencepiece::logging::GetMinLogLevel() >                              \
   ::sentencepiece::logging::LOG_##severity)                                 \
      ? 0                                                                    \
      : ::sentencepiece::error::Die(                                         \
            ::sentencepiece::logging::LOG_##severity >=                      \
            ::sentencepiece::logging::LOG_FATAL) &                           \
            std::cerr << ::sentencepiece::logging::BaseName(__FILE__) << "(" \
                      << __LINE__ << ") "                                    \
                      << "LOG(" << #severity << ") "
```

- 原来 LOG(severity) 是一个 三元表达式啊！
- 当我们写下 `LOG(INFO) << "a";`，宏被扩展后代码是这样的（用 `clang++ -E xxx` 来获取预处理结果，此时宏会被扩展，但还不会被编译）

  ```cpp
  (::sentencepiece::logging::GetMinLogLevel() > ::sentencepiece::logging::LOG_INFO) 
  ? 0 
  : ::sentencepiece::error::Die( ::sentencepiece::logging::LOG_INFO >= ::sentencepiece::logging::LOG_FATAL) 
  & std::cerr << ::sentencepiece::logging::BaseName("test.cpp") << "(" << 13 << ") " 
  << "LOG(" << "INFO" << ") " << "a";
  ```
  
  （为了显示方便，人工增加了换行）

  整体上是 (level判断) ? 0 : Die(true/false) & std::cerr << LOG前缀生成 << 用户真正想输出的内容; 

  如果 level 判断直接失败（低于最低优先级），表达式直接为 0； 否则就执行后面的逻辑：Die 是一个定义的类，如果构造函数参数为true，则析构时调用 Fatal, 这样实现 `LOG_FATAL` 的功能；后面就是正常的输出了，因为 `std::cerr <<` op 会返回自己，所以这个前面一堆宏扩展，后面依然可以接自己的逻辑。非常巧妙。 特别地， `Die` 还实现了 `&` op 

  ```cpp
  int operator&(std::ostream &) { return 0; }
  ```

- 仔细思考一下，有两个点要注意：

  1. 因为 `?:` condition operator 的 parse 优先级 (precedence) 比 `&`, `<<` 表达式都低，所以`:`后面的运算是一个整体，作为 `:` 的一部分被parse到一起。事实上，`?:` 优先级只比 `,` 高，且这个档位上是右边做左边结合，所以其他的右边的操作都会先结合； 可以说，`?:` 后面的任何连续（不是用`,`连接）表达式，都是优先被 parse 的
  2. 既然 `?:` 后面的表达式基本都是先 parse, 那岂不是 true-branch, false-branch 都会先被执行，然后再根据 condition 求值结果来取对应分支的结果？这合理吗？显然，这不合理！我们要知道，parse 是编译器生成代码的前置步骤，具体求值操作的代码，会在 parse 后生成。具体地，按照[eval-order 定义](https://en.cppreference.com/w/cpp/language/eval_order)，`?:` 总是从 condition 开始求值的！然后再基于 condition 结果走其中一个 branch，这与我们的直观理解是匹配的。

  关于 parse, evalution 以及这二者的区别， [stackoverflow-ternary-conditional-and-assignment-operator-precedence](https://stackoverflow.com/questions/7499400/ternary-conditional-and-assignment-operator-precedence) 上也有类似的讨论。自己还瞎写了个答案以作补充（花了一个多小时……）
