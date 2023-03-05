---
layout: post
title: C++ 易混淆知识点记录
date: 2023-02-27
categories: 技术
tags: C++
---


## 1. 变量初始化

有两种： 值初始化、默认初始化。

还有一种是 list-initialization 不知道算哪一种。

另外，还有 copy-initialization 和 direct-initialization 的区分。

还有 brace-or-equal-initializer 

## 2. 类成员初始化

正常来说，有 2 个地方可以做类成员初始化： 

1. in-class member initialization (from c++11)
2. member-initializer-list

initializer-list 是最标准的做法；而 in-class 是一种比较方便的做法，但它有很大的限制： 只能做 copy-initialization 和
list-initialization, 不能做 direct-initialization.

https://stackoverflow.com/questions/28696605/why-class-data-members-cant-be-initialized-by-direct-initialization-syntax

其他区别：

https://stackoverflow.com/questions/27352021/c11-member-initializer-list-vs-in-class-initializer

in-clas member initialization 主要还是为了方便成员做默认值的初始化的——它相当于替代了编译器默认的初始化的值。