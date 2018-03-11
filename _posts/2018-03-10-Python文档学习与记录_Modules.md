---
layout: mathpage
title: Python文档学习与记录——Modules
date: 2018-03-10
categories: 技术 
tags: Python 学习
onewords: module是什么；Package是什么
---
> 本文主要对Python文档`Modules`做一个学习总结、记录。

## 背景

素来因代码规范而自豪的我（= =），前几天因为`import`相关的问题导致代码没有通过公司的编码规范检查，因为任务优先，当时就选择临时豁免了。
今天算是实现当时的想法，对[6. Modules](https://docs.python.org/2/tutorial/modules.html)做一个学习和记录。
此外，从[How To Package Your Python Code](http://python-packaging.readthedocs.io/en/latest/index.html)学习了package的结构，下一个工程就全面按照这个规范来做，这样就总算是“代码规范”了吧~


## 正文

本文主要由以下内容组成， 

a. Module相关：Module是什么；module相关的知识
b. Package相关：Package是什么；import相关的知识；如何去组织package；开发实践（开发Package时怎么才能方便测试？）

### Module

#### Module是什么

原文的定义是：

> A module is a file containing Python definitions and statements.

就是把Python的定义、声明给放到一起的文件。一个`*.py`就是一个Module，module name就是文件名。Module本身有一个`__name__`的变量，一般存储的就是自己的名字。

Module应该是Python代码组织结构的一环：Package - Module - Class。 Module也是代码逻辑组织与文件组织共同体现。
写Module的意义，当然是为了重用性和可维护吧。公用功能拆分、大逻辑解耦，都需要落实到module上。

Module里可以包含函数、变量的定义，也可以包含一些可运行的代码，`These statements are intended to initialize the module`.

一个Module包含自己独立的符号表(private symbol table)，module内部声明的变量、函数等都在这个符号表里（import进来的也放在这里），类似C++的命名空间。这使得我们不必担心module间重名的问题。

#### 将Module作为Scripts运行

首先是Module与Scripts的区别：scripts应该指可以运行的东西，然而——Module本身就可以“被”运行：`python xx.py` 就会运行这个`xx`module，且作为入口的这个module文件，其`__name__ == "__main__"`，而不再是module自己的名字。有时候，我们为了测试Module，或者让Module的一部分代码是为了被别的Module引入，一部分代码是为了在作为入口时运行，就会利用这个逻辑：

    if __name__ == "__main__":
        # some statements for running as scripts


#### import一个Module发生了什么

假设要import的module是module_a, 猜测是这样的：

1. 看 `module_a` 是不是已经被import了; 如果是，则直接返回；否则继续。
2. 初始化对于`module_a`的符号表
3. 将`module_a`编译成`pyc`或者`pyo`，并运行`module_a`
4. 导入其中的符号表到`module_a`对应的符号表。

已上只是一个**瞎猜**！根据[How can I have modules that mutually import each other?](https://docs.python.org/2/faq/programming.html#how-can-i-have-modules-that-mutually-import-each-other) 猜测的，具体如何待后面再确定。

不过有一点是确定的——一个module只会被引入一次；但是，根据一些额外的信息（编码规范），判定一个module是否已经被引入，应该是按照module名字来的（当然了……），而module的名字是由import时的表示确定的，假设`import a.b.c` 和 `sys.path.append("a/b"); import c` 都会引入`c`，那么其实c被引入了两次，分别是 `a.b.c` 和 `c`.

#### `pyc`和`pyo`相关

这两个都是源程序编译为的机器无关的字节码，跟Java的class一样，是可以独立于操作系统、机器结构等的——只要有对应的interpreter, 都可以正常解释为机器码。

`pyo`是使用`python -O xxx.py`时，被依赖的module被编译成的文件格式，也就是做了优化的格式。相比`pyc`，

>  The optimizer currently doesn’t help much; it only removes assert statements. When -O is used, all bytecode is optimized

官网上面的说法感觉有点矛盾……这里姑且取第一句，认为`pyo`就是把`assert`的语句给删了，仅此而已。除了`-O`, 还有`-OO`的优化，似乎只是把`_doc__`给删了……为了简单，后面就只说`pyc`, 不再提`pyo`.

`pyc`已经是字节码，因此已经可以直接发布了——不再需要源文件。当你的代码不开源时，一定程度上可以用来反工程（`This can be used to distribute a library of Python code in a form that is moderately hard to reverse engineer.`）。根据我之前在某动实习时师兄告诉我的经验，pyc是挡不住的，需要先把python源代码利用Cython给转成C代码，再拿C编译器编译成so，再发布……

当既有`pyc`, 又有`py`是，解释器首先看`pyc`是不是最新的，是就直接用，不是再编。关键是怎么看是不是最新的呢？之前我以为就是比两个文件的时间戳，不过真实的是：

> The modification time of the version of spam.py used to create spam.pyc is recorded in spam.pyc, and the .pyc file is ignored if these don’t match.

把源文档变更时间直接写到pyc里了，的确更加稳妥！

最后，`pyc`只是加速启动时间，跟运行速度无关。

#### `dir`函数和`__builtin__`和`sys`的特殊变量

`dir`是内建函数，可以把Module内的全局变量（module的符号表）返回为list；特别地，如果是在交互式环境直接使用（不带参数），那么返回的是整个交换式环境下的所有名字，变量啊、类啊都混在了一起。而其中就有一个`__builtins__`的特殊名字，是所有内建名字的代替吧。文档里说用`import __builtin__; dir()`可以看到一大串，可以看到包括Errors定义、内建函数、特殊变量等。

最后再说`sys`, 先说`sys.path`, 决定了import搜索路径的东西。之前都是通过操作这个列表来完成不同层级的module的导入的。有一个初始化顺序需要了解一下：


    the directory containing the input script (or the current directory).
    `PYTHONPATH` (a list of directory names, with the same syntax as the shell variable PATH).
    the installation-dependent default.


此外还发现两个有意思的，`sys.ps1`, `sys.ps2`， 第一个是交互式解释器第一层的提示字符串(prompts), 第二个是第二层；默认情况下是`>>>`和`...`；是可以直接赋值的。哈哈。

### Package

#### Package是什么

Package是Module的集合，`a way of structuring Python’s module namespace by using “dotted module names”`. 直观而不严谨的说明，一个package通常包含一个层次化的module集合，对应到文件系统，就是层次化的文件结构。文件系统的层次化用`/`表示，对应到Package，就是`.`. 例如文件系统结构 `a/b`, 对应的mudule表示是`a.b`. 特别地，为了防止意外的引入不期望的文件，一个文件夹要对应到一个Package或Sub-Package，必须包含一个`__init__.py`文件。

应该是，Package只是一个组织表示的命名，真正在Python中真实存在的对象，还是是Module.

#### `__init__.py`中的内容

`__init__.py`可以是一个空文件，仅仅表示这个文件夹对应到一个Package;不过，它也可以作为这个package初始化代码存放的地方。

此外，它还有一个特别的变量`__all__`，它是一个list，只在`from package import *`这种语法出现的时候有意义——相当于在`import *`时人为覆盖了由Python自己枚举的`__all__`. 特别的，Python自己枚举的`__all__`也不是全部的module，而是a. 在`__init__.py`里定义的names； b. 在之前已经显式import进来的属于该package的子模块。不知道自己理解（翻译）对不对，或者说又没有解释清楚——可以参考[import * from a package](https://docs.python.org/2/tutorial/modules.html#importing-from-a-package). 在生产代码中，永远不要使用`import *`，它是代码可读性降低，同时可能让你implicitly覆盖掉import进来的名字。

#### 绝对引用，相对引用（隐式相对引用，显式相对引用）

首先要明确：Package是被引入的单元，而不是被直接运行的单元。

被直接运行的，如前面所说，是`Scripts`, 也就是有`__name__ == "__main__"`的Module. 

应该可以说，绝对引用(absolute import)，相对引用(relative import, including implicit relative import, explicit relative import) 都是在Package下的概念！对于`Scripts`是不存在的，Scripts只有一个 search path的概念：从search path从去找package名字，找到就导入。

上面的说法估计不是正确的，但是Scripts没有 implicit relative import是必然的。因为implicit relative import 依赖的是当前`module.__name__`的层次化表示的名字，而Scripts的`__name__ == "__main__"`，没有层次化名字！ 

前面说了半天，还没有说什么是绝对引用和相对引用。

在Package环境下，如果import一个包，是从package的根目录一级一级引入的，那么这就是绝对引入。否则，如果是相对当前位置，那就是相对引用。特别地，如果用`.`或者`..`明确指明，那就是显式相对引用，否则就是隐式相对引用。不得不举例子了，直接用原文的例子：


    sound/                        Top-level package
        __init__.py               Initialize the sound package
        formats/                  Subpackage for file format conversions
                __init__.py
                wavread.py
                ...
        effects/                  Subpackage for sound effects
                __init__.py
                echo.py
                surround.py
                ...


假设`surround.py`要引入`echo.py`，

a. 如果是 `import sound.effects.echo` 显然是绝对引用
b. 如果是 `import echo`，那么是隐式相对引用
c. 如果是 `from . import echo`，那么是显式相对引用。

再举一个例子，假设 `sourround.py`要引入`wavread.py`, 那么

a. 绝对引用： `import sound.formats.wavread`
b. 隐式相对引用： 不可能
c. 显式相对引用： `from ..formats import wavread`

可以看到绝对引用和显式相对引用是功能完善的，而隐式相对引用，实际是很弱，仅仅是为了方便！但是我认为，也正是因为这种方便，让我们混淆了Package和Scripts的引入概念。

#### `from __future__ import absolute_import`以及正确使用

这里参考的是[python __future__ package的几个特性](http://www.cnblogs.com/harrychinese/p/python_future_package.html)，我觉得写得很棒！

虽然名字是`absolute_import`，但是其真正含义是**禁止隐式相对引用**.

用了`from __future__ import absolute_import`的Module, 如果把这个Module当作Scripts来运行，常常报这个错： ` ValueError: Attempted relative import in non-package. `. 原因就是前面说到的，相对引用的概念只存在于Package下（依赖于`module.__name__`来路由位置），Scripts是没有这个东西的。

**所以，一定要清楚，你写的Module是Package的一部分，还是Scripts**。二者不要想得兼。自己以前总是混写，以后得规范了。

推荐方法如ref的文章所言，Package的Module内用绝对引用或显式相对引用，内部定义一个main函数，然后再写一个scripts调用。当然，这个main放在scripts里也不错。

#### 如何去组织Package

来自[How To Package Your Python Code](http://python-packaging.readthedocs.io/en/latest/index.html), 不赘述。

#### 开发实践

整体思路是，应该按照Packge的组织去写。然后不嫌麻烦的话，可以把Scripts单独放到一个文件夹下；如果嫌麻烦，可以把Scripts直接放到相应的sub-package下，因为是package，自然是从包的位置开始去引入，因此这个scripts的位置不影响其import操作，所以最后调试好了，再把这些Scripts放到一个bin文件夹里。

待实践后确定……