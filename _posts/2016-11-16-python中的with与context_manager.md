---
layout: mathpage
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

以前只知道`open(xxx)`可以和`with`一起用，这样就不用手动处理文件异常、关闭操作了。最初看到`tf`的这么写的时候，在控制台看了下`g.as_default()`和`tf.device()`的返回值，发现都是一个`contextlib._GeneratorContextManager`的东西。第一看看到英文名称，大概知道是一个上下文对象，没有深究。后来，因为在不同的地方，我都要写这条`with`语句，但是在不同地方写同一的东西显然是比较危险的（冗余、改动麻烦），因此就产生了用一个函数封装的想法。可以，这个怎么封装到一个函数中，且可以使用with语句调用？完全不懂，不得不去查这一系列究竟是如何实现的？

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

    > Python3中，io.py里只有一些接口，似乎是用C语言实现的，看到源代码，也没有doc。不过通过在控制台执行相应函数，可以看到函数执行结果：`__enter__()`返回文件对象本身；`__exit__()`关闭文件。

2. 把`mrg`的`__exit__`函数存储起来, 赋值给`exit`变量。这个名称很有玄机啊，似乎是故意覆盖global的`exit`函数？应该是这样，不然没法将用户在`BLOCK`对exit的调用捕获到（如果不能捕获，那么资源销毁就做不到了）。

3. 调用`mgr`的`__enter__`,将其值存储起来，之后会将其作为`as`语句的赋值。

4. （接着是两个try块，内部的try-except是用来处理BLOCK异常的，而外部的一场块，正如注释所言，是处理内部BLOCK无异常、或者内部BLOCK存在非局部跳转的逻辑的。非局部跳转这里不是很懂，暂时跳过吧。）看第二个try块，即`VAR = value`, 完成as语句中的赋值。这里看出是把`__enter__()`返回的结果作为as的赋值——而不是`(EXPR)`的结果。

    > 所以类似`tf.Graph().as_default() as g`中，并不是把`as_default()`的返回值给了`g`, 而是把`as_default()`内部`yield`的（就是Graph实例）赋给了g。这里注意 。

    同时需要注意的是，这里`as`的赋值已经在`try`里面了，所以`as`出错也会被捕获到。

5. as赋值之后，就是`BLOCK`的执行。如果无异常，那么走`finaly`分支；如果有异常，那么走`except`分支。用`exc`布尔变量来防止两个分支被同时执行。

6. 异常处理。`fanally`分支就是调用mgr的`__exit__()`函数，后面三个参数都是`None`，方便库实现时对状态的判断——理论上，`mgr`应该在此条件下正常释放资源；`except`分支，将异常信息（sys.exc_info() 返回最近一次异常的信息，返回的数据类型是元组，即(type, value, traceback)，恰恰就是exit函数定义的后3个参数）传给`mgr`的`__exit__`函数，此时`mgr`内部可能需要对此异常做处理——或者捕获下来，释放可能的资源，返回状态码；或者直接报错，应该都没有问题。

以上就是`with`的分析，说了这么多，其实我觉得需要记住的with语句调用`EXPR`的两个函数，以及这两个函数的功能：

**`(EXPR)`返回的对象得有`__enter__(self)`和`__exit__(self, type, value, traceback)`两个函数。第一个函数用来返回赋值给`as`指示的对象；第二个函数用来释放资源、处理异常。**

强调，as对象赋值，不是`EXPR`的值，而是`__enter__`返回的值；其次，`__exit__`必然会被调用，只是传入的参数可能是None，或者是发生的异常信息。

