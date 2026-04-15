---
layout: post
title: Python中的with与上下文管理装饰器contextmanager
date: 2016-11-16
categories: 技术 
tags: Python with contextmanager
onewords: Python的魔法。
---
> 本文介绍了Python中with语句的背后细节，以及contextmanager装饰器的实现。

让我产生了解with和contextmanager需求的是tensorflow中的代码：

    g = tf.Graph()
    with g.as_default(), tf.device("/cpu:0"):
        #blabla

以前只知道`open(xxx)`可以和`with`一起用，这样就不用手动处理文件操作异常、关闭等操作了。最初看到`tf`里的这些语句，在控制台看了下`g.as_default()`和`tf.device()`的返回值，发现都是一个名为`contextlib._GeneratorContextManager`的东西。第一眼看到英文名称，大概知道是一个上下文对象，没有深究。后来，因为在不同的地方，我都要写这条`with`语句，但是在不同地方写同一的东西显然是比较危险的（冗余、改动麻烦），因此就产生了用一个函数封装的想法。可是，这个怎么封装到一个函数中，且可以使用with语句调用？完全不懂，不得不去查这一系列究竟是如何实现的？

以上是冗余的背景，下面开始做一下查询的记录。

## with语句

Python官方文档[with-statement](https://docs.python.org/3/reference/compound_stmts.html#the-with-statement)把with语句的处理说得非常细致了，这里翻译一下...

**with语句的功能**

用上下文管理器封装语句的执行，使原来的`try...except...finally`变得更加方便。即通过使用with语句，可以自动调用对象的资源释放、异常捕获等操作，减少用户手动调用的繁琐与可能遗漏的危险。

**with表达式的格式**

    with_stmt ::=  "with" with_item ("," with_item)* ":" suite
    with_item ::=  expression ["as" target]

即 `with` 关键字， 后面跟`with_item`, 后面又可跟多个可选的`, with_item`, 最后再跟`:`，然后是`suite`，即with作用域内的代码。后面的解释说如果是有多个的`with_item`，那么其实际上就是嵌套的`with`，即：

    with A() as a, B() as b:
        suite
    
    <==>
    
    with A() as a:
        with B() as b:
            suite

此外，`with_item`表示为一个表达式 后跟可选的`as` `target`.

**处理流程**

或许先把[PEP343](https://www.python.org/dev/peps/pep-0343/)里的代码贴出来比较好：

    with EXPR as VAR:
        BLOCK

    <==>

    mgr = (EXPR)
    exit = type(mgr).__exit__  # Not calling it yet
    value = type(mgr).__enter__(mgr)
    exc = True
    try:
        try:
            VAR = value  # Only if "as VAR" is present
            BLOCK
        except:
            # The exceptional case is handled here
            exc = False
            if not exit(mgr, *sys.exc_info()):
                raise
            # The exception is swallowed if exit() returns true
    finally:
        # The normal and non-local-goto cases are handled here
        if exc:
            exit(mgr, None, None, None)

代码里的变量表示与前面表达式里的命名不一样，不过应该是可以无缝切换的。后面还是以代码里的变量名为准。

1. 执行表达式`EXPR`，并将结果赋值给`mgr`, 这个变量一般来说是`contextmanager`装饰器产生的变量（如其名），不过结合对`file`的理解，其实知道这个对象有`__enter__`和`__exit__`就ok。

    > Python3中，io.py里只有一些接口，似乎是用C语言实现的，看不到源代码，也没有doc。不过通过在控制台执行相应函数，可以看到函数执行结果：`__enter__()`返回文件对象本身；`__exit__()`关闭文件。

2. 把`mrg`的`__exit__`函数存储起来, 赋值给`exit`变量。这个名称很有玄机啊，似乎是故意覆盖global的`exit`函数？应该是这样，不然没法将用户在`BLOCK`对exit的调用捕获到（如果不能捕获，那么资源销毁就做不到了）。

    update: 上述说法是错误的。资源销毁因为是放在finally块中的，所以必然可以销毁。这里命名为exit也没有太大的含义，改成其他名字也无所谓的...

3. 调用`mgr`的`__enter__`,将其值存储起来，之后会将其作为`as`语句的赋值。

4. （接着是两个try块，内部的try-except是用来处理BLOCK异常的，而外部的一场块，正如注释所言，是处理内部BLOCK无异常、或者内部BLOCK存在非局部跳转的逻辑的。非局部跳转这里不是很懂，暂时跳过吧。）看第二个try块，即`VAR = value`, 完成as语句中的赋值。这里看出是把`__enter__()`返回的结果作为as的赋值——而不是`(EXPR)`的结果。

    > 所以类似`tf.Graph().as_default() as g`中，并不是把`as_default()`的返回值给了`g`, 而是把`as_default()`内部`yield`的（就是Graph实例）赋给了g。这里注意 。

    同时需要注意的是，这里`as`的赋值已经在`try`里面了，所以`as`出错也会被捕获到。

5. as赋值之后，就是`BLOCK`的执行。如果无异常，那么走`finaly`分支；如果有异常，那么走`except`分支。用`exc`布尔变量来防止两个分支被同时执行。

6. 异常处理。`fanally`分支就是调用mgr的`__exit__()`函数，后面三个参数都是`None`，方便库实现时对状态的判断——理论上，`mgr`应该在此条件下正常释放资源；`except`分支，将异常信息（sys.exc_info() 返回最近一次异常的信息，返回的数据类型是元组，即(type, value, traceback)，恰恰就是exit函数定义的后3个参数）传给`mgr`的`__exit__`函数，此时`mgr`内部可能需要对此异常做处理——或者捕获下来，释放可能的资源，返回状态码；或者直接报错，应该都没有问题。

以上就是`with`的分析，说了这么多，其实我觉得需要记住的with语句调用`EXPR`的两个函数，以及这两个函数的功能：

**`(EXPR)`返回的对象得有`__enter__(self)`和`__exit__(self, type, value, traceback)`两个函数。第一个函数用来返回赋值给`as`指示的对象；第二个函数用来释放资源、处理异常。**

强调，as对象赋值，不是`EXPR`的值，而是`__enter__`返回的值；其次，`__exit__`必然会被调用，只是传入的参数可能是None，或者是发生的异常信息。

## 为类（对象）加入with语句的支持

基于对上面`with`原理的了解，我们可以很简单地写出基础的支持`with`语句的类。以下是一个实例，同时也能够加深理解：

    class MgrInteface(object):
        def __enter__(self):
            print("Enter called")
            return "returned object from __enter__(self)"

        def __exit__(self, type, value, traceback):
            if type is None:
                print("no exception.")
                print("doing clear")
            else:
                print("exception occured. supress or raise.")
                if type is IOError:
                    print("catch IOError")
                    return True
                else:
                    print("{} not catch. will be raise.".format(str(type)))
                    return False

    def main():
        with MgrInteface() as s:
            print("'as' vlaue is: {}".format(s))
            raise IndexError
            raise IOError

    if __name__ == "__main__":
        main()

这种实现是非常简单、直观的。不过Python语言的贡献者们觉得这样还是不够方便。或者说，其应用场景有限（只能用于类），于是有了`context manager`。

## 为函数加入with语句的支持（使用contextmanager decorator）

`contextmanager`是`contextlib`包里的一个函数，是装饰器的实现。[官方文档](https://docs.python.org/3/library/contextlib.html#contextlib.contextmanager)里给出的定义是：`contextmanager`是一个用于定义 with语句中用到的上下文管理器 的工厂函数，其是一个装饰器。使用此函数可以不必单独创建一个类或者定义`__enter__`和`__exit__`函数，只需作用于一个**特定格式的生成器函数**就能创建一个上下文管理器。

上面是一个总的定义，下面开始相对细致的介绍一下。不过`contextmanager`很精妙（就是说很复杂...），所以先介绍一下怎么用，然后再说其背后的实现。

### 使用contextmanager来创建上下文管理器对象


上面的定义中，强调了**特定格式的生成器函数**，是因为如果我们要使用`contextmanager`，只需要定义一个满足此“特定格式”的函数即可——这是我们唯一需要做的地方。

这种格式是怎样的？

直接把官网的例子拿过来：

    from contextlib import contextmanager

    @contextmanager
    def tag(name):
        print("<%s>" % name)
        yield
        print("</%s>" % name)

    with tag("H1"):
        print("title")

    ---- output ---

    <H1>
    title
    </H1>

从这个函数中我们可以总结出该格式： 以`yield`分段，前面部分是`__enter__`的逻辑，后面部分是`__exit__`的逻辑。

为什么用`yield`（生成器）？因为生成器运行到`yield`就会终止，并且返回`yield`的值；下次再调用该函数，从会从`yield`之后继续调用！是不是很神奇？通过生成器的巧妙运用，就在一个函数中实现了两个函数需要实现的逻辑！这应该是该`contextmanager`与`with`语句交互的底层逻辑了，即with语句实际调用的都在这一个函数中定义了。而在此底层逻辑之上，又是怎样封装起来使得其能够与`with`兼容，就是下一小节介绍的内容了。

上面的例子是简化的版本，因为为了完整、异常安全，一般需要加上一个`try...finally`块，看下tensorflow中`Graph.as_default()`是怎么实现的吧：
    
    # tensorflow/python/ops.py
    
    def as_default(self):
      return _default_graph_stack.get_controller(self)
    
    # tf使用2个空格缩进
    # as_default其实是另一个函数的调用

    =>
    
    # 真实的调用
    # 使用contextmanager装饰器，不用看具体代码，看整体的框架即可
    
    @contextlib.contextmanager
    def get_controller(self, default):
    """A context manager for manipulating a default stack."""
        try: 
          self.stack.append(default)
          yield default
        finally:
          if self._enforce_nesting:
            if self.stack[-1] is not default:
              raise AssertionError(
                  "Nesting violated for default stack of %s objects"
                  % type(default))
            self.stack.pop()
          else:
            self.stack.remove(default)

从上面的代码中可以看到，其使用了`try...finally`块，其中`try`块包含的就是`__enter__`部分，`finally`块包含`__exit__`部分。其实从这里看出，`try...finally`块也不是必须的——只要`__enter__`部分确定不会抛出异常。

以上，我们就实现了用`contextmanager`装饰器来为一个函数增加with语句支持。回顾一下关键点，即只需要保证这个函数中包含一个`yield`语句（生成器），然后用contextlib.contextmanager装饰即可。

### 追踪contextmanager的背后

“源码之下，了无秘密”，让我们带着侯捷老师的教诲（...），去看下源代码实现。

首先看下`contextmanager`装饰器实现：

    # contextlib.py

    def contextmanager(func):
        @wraps(func)
        def helper(*args, **kwds):
            return _GeneratorContextManager(func, args, kwds)
        return helper

首先，这是一个标准的基于函数实现的装饰器（其实我也是刚刚才懂的...）。

> 插入装饰器的介绍

咳，为了完整地记录下调研过程，再插入一下装饰器的本质：

    @decorator_name
    def func(*k, *kw):
        ...
    =>
    # 把被装饰函数作为@xxx中的`xxx`函数的参数传入（并调用该函数），
    # 返回的结果再覆盖原来的函数名。
    func = decorator_name(func) # 传入、调用、覆盖

看到了吗——装饰器的本质就是这么简单。如果多个装饰器嵌套，那么就是嵌套调用（没有见到，就不深究了.）！解释一下，首先装饰器的本质是一个 返回可调用*对象* 的*对象*。这个*对象*既可以是函数，也可以是实现了`__call__`的类对象。装饰器对象，一般接受一个函数名（函数指针）作为参数，返回一个针对此函数封装的可调用对象，并用此对象再覆盖掉原来的函数名。 装饰器应该是来自于设计模式中的装饰器模式，通俗来说就是——我们不仅仅是大自然的搬运工（调用其他函数），我们还在自然水的基础上消毒（在此之外还做一些额外的工作）。

此外，上述`helper`还有个装饰器`@wraps`, 查文档可知该装饰器完成文档的更新——即用户看`__doc__`或者帮助时，不要让他们看到空荡荡的被替换了对象的文档，而是依然返回原函数的文档。这样一来装饰器就完全对用户不可见了。

> 结束装饰器的介绍

接上，经过`contextmanager`装饰器作用后，我们的原函数已经变成了`_GeneratorContextManager(func, args, kwds)`（即原函数名指向的对象已经不再是原来那个函数，而是装饰器函数contextmanager返回的这个对象实例了）。简要看下这个对象（类）是怎么定义的：

    class _GeneratorContextManager(ContextDecorator):
        def __init__(self, func, *args, **kwds):
            self.gen = func(*args, **kwds)
            ...

        def __enter__(self):
            try:
                return next(self.gen)
            except StopIteration:
                raise RuntimeError("generator didn't yield") from None

        def __exit__(self, type, value, traceback):
            if type is None:
                try:
                    next(self.gen)
                except StopIteration:
                    return
                else:
                    raise RuntimeError("generator didn't stop")
            else:
                if value is None:
                    # Need to force instantiation so we can reliably
                    # tell if we get the same exception back
                    value = type()
                try:
                    self.gen.throw(type, value, traceback)
                    raise RuntimeError("generator didn't stop after throw()")
                except StopIteration as exc:
                    # Suppress the exception *unless* it's the same exception that
                    # was passed to throw().  This prevents a StopIteration
                    # raised inside the "with" statement from being suppressed
                    return exc is not value
                except:
                    # only re-raise if it's *not* the exception that was
                    # passed to throw(), because __exit__() must not raise
                    # an exception unless __exit__() itself failed.  But throw()
                    # has to raise the exception to signal propagation, so this
                    # fixes the impedance mismatch between the throw() protocol
                    # and the __exit__() protocol.
                    #
                    if sys.exc_info()[1] is not value:
                        raise

在这个类里终于看到了熟悉的`__enter__`和`__exit__`, 与最开始介绍的基础版本，看看在这里它们都是怎么工作的吧：

`__enter__`，调用`gen`的`next()`方法，并返回`next()`值。其中`gen`就是`func(...)`, 我们知道`func`是我们最开始定义的带`yield`的函数，比如那个我们从官方文档上copy下来的生成封闭HTML标签的函数：
    
    @contextmanager
    def tag(name):
        print("<%s>" % name)
        yield
        print("</%s>" % name)

我们知道调用带`yield`的函数，其返回的就是一个`生成器`,即`gen`就是一个生成器。正如前面所言，在`__enter__`中调用了该生成器的`next()`方法, 则函数会执行到`yield`的位置，返回该值，并且停住，等待下次`next()`的调用（后面看到，下次调用就是在`__exit__`中）。这时，`__eneter__`就完成了任务。所以，`yield`和之前的部分，在`__enter__`中被执行了。也即是说相对于基本的`__enter__`, 用`contextmanager`装饰的函数中，`yield`及之前的部分就是等价的逻辑。

在`__exit__`的逻辑，和传统的`__exit__`是类似的——先判断由`with`传入的信息，看是否有异常发生，如果无，则调用`next()`，即函数中`yield`之后的部分被执行，且因为后面不再有`yield`，故当函数后部分正常执行后，会抛出`StopIteration`异常，捕获之；如果没有抛出迭代停止的异常，要么是发生了其他异常，这时在原函数就会抛出；要么是继续`yield`, 故这里抛出`RuntimeError`，说明该函数没有符合格式要求。如果在`with`块中有异常，还是要调用`gen`来执行后续的收尾工作。不过后面的异常处理就没有太看明白了。主要是生成器的`throw`操作具体会做什么，没有再深入的研究。


ok，再次感觉说得非常啰嗦。总结一下，这个部分还是非常巧妙的。关键点就是利用了生成器可以`返回值、停住、下次在停住位置之后继续`的性质，然后再在`__enter__`和`__exit__`中分别利用该性质，从而完成了在一个函数中实现本应该在两个函数中分步完成的逻辑。实在是妙啊！当然，这中间的各种特性，如装饰器、生成器，返回函数对象等，还是让我在整个过程中常常陷入混淆。现在还是笔用得少，把结果记在纸上，真的会清晰很多。

## 问题如何解决

就是代码里一个其实可以忽略的请求，导致了这差不多一天的调研及记录，然而让我们回过头来，看看当我们理解了`Graph.as_default()`, `tf.device()`背后的秘密之后，能够解决我们之前的问题呢：如何在一个地方写配置，然后在多处调用，且与`with`语句兼容？

在跟踪的过程中还是有思路的：

首先既然要兼容`with`,那么必然有`__enter__`和`__exit__`逻辑，至于具体是用类、还是用contextmanager装饰的函数，这个倒无所谓。

`__enter__`逻辑里，我们分别显式调用`as_default()`和`device()`返回的mgr的`__enter__`即可，并且还返回`as_default()`的返回值（即`Graph`）.

`__exit__`逻辑里，分别显式调用各自的`__exit__`即可，注意调用顺序与`__enter__`相反（应该是相反的吧，或许也是无所谓的——只要这两个资源是独立的，不过我看`as_default`其实真正是在把图放到一个栈里了，没有去看`device`在做什么。反序应该最稳）。

一种可能的实现如下（已实验，暂时没有问题）：

    def _set_context(self):
        g_mgr = self._graph.as_default()
        g = g_mgr.__enter__()
        d_mgr = tf.device("/cpu:0")
        d_mgr.__enter__()
        yield g
        d_mgr.__exit__(*sys.exc_info())
        g_mgr.__exit__(*sys.exc_info())


##　END

文章终于写完，花了一天，想想对于结果驱动的项目而言，这个时间开销必然是多余的。此外，在深入与赶紧脱离泥潭的纠结里，烦躁也会产生。现在各种调度还是把握不好啊。不过通过这个调研，也可以发现Python这门语言是何其精妙，其封装程度是非常高的。其使用起来非常方便，研究起来又能学有所获，的确是一门适合各种人的语言。当然，拥有太多的封装，太多的语法糖，对于想要理解其背后原理的人来说，也是很苦恼的。这么一想，C、C++依然有其忠实的拥趸也是一件很合理的事。不过，C++也在越来越精妙的路上前进。这么说，程序员的“懒惰”品质始终是主流。