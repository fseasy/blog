---
layout: post
title: stream流到bool的转换不等于good函数的结果
date: 2016-07-06
categories: 技术 
tags: C++ 实践
onewords: 当到达EOF时，good函数立刻变为false，但是stream转为bool仍可能为真。
---
> 当到达EOF时，good函数立刻变为false，但是stream转为bool仍可能为真。stream到bool的转换与good不等价，而与fail && bad等价。

## 情景

对getline完成一个封装，再封装内部需要判断当前文件流是否ok，之前是类似这么写的：

    // ifstream is(fpath) ; --member variable

    string line ;
    bool is_good = getline(is, line).good();
    if(! is_good){ return false; }
    // continuous processing
     
    return true;

大多数情况下，上面是没有问题的！但是，偶然发现，如果最后一行没有换行符，那么上面的代码就会跳过最后一行！

分析一下原因，因为没有空行，所以其首先遇到EOF，然后EOF被置位，所以good就应该返回false，所以最后一行被跳过，没有问题。

但是常规的写法就不会就问题——即即使最后最后一行没有换行，那么最后一行还是能被读出：

    string line;
    while(getline(is, line))
    {
        // normal handle
    }

在MSVC和G++上都测试过，的确可以打印没有换行符的最后一行。

所以，这告诉我们什么？

## 原因

首先，我们知道，`getline(istream&, string &line)`返回的是一个流对象 `istream &` ， 其次，在条件判断环境下， 如上面的`while(getline(is,line))`，会隐式做一个到bool类型的强制类型转换，即`static_cast<bool>(var)`， 而要打印最后一行，这说明即使EOF被置位，其到bool的转换仍然为真。

在手册上找到了该[转换函数](http://www.cplusplus.com/reference/ios/ios/operator_bool/)的说明：

    Returns whether an error flag is set (either failbit or badbit).
    Notice that this function does not return the same as member good, but the opposite of member fail.

可以看到其不是与good等价，而是与fail等价——不检测EOF标志位，而仅仅检测 failbit或者 badbit。应该是到达EOF后，本次的其余标志位都是正常，但下次读取时，failbit被置位了。
