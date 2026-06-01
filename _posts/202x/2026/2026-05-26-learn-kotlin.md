---
layout: post
title: Kotlin 学习
date: 2025-05-26
categories: 技术
tags: Kotlin
excerpt: 开始做 ImTrace Android，要用 Kotlin，快速入门下，感觉有些东西挺有意思的。
---

## 变量声明：var 与 val

一开始觉得容易眼花—知道 val 是 `value` 后才恍然大悟。

## ?: Elvis Operator

这个奇怪的名字，原来是来自 Elvis Aaron Presley，上个世纪 icon 型的人物，他标志性的发型，从侧边看，就可这个符号一样。
哈哈，有点意思。

## Unit

即是类型，也是值。多用在函数型参数定义的地方，表示 `void` 的含义。

## 尾随 Lambda

一开始接受不了，感觉把一个很规则化的东西搞得复杂了，但是看下 DS 给的例子，又觉得这东西确实挺精妙的。


```Kotlin
// 测试框架（这看起来像英语）
describe("Calculator") {
    it("should add correctly") {
        expect(2 + 2).toBe(4)
    }
}

// HTML 构建器（看起来像 HTML 本身）
html {
    body {
        h1 { +"Hello" }
        p { +"World" }
    }
}

// 如果用传统写法，这些 DSL 会非常丑陋
```

## 委托语法

入门 Jetpack Compose, 看到这个

```kotlin
var count by remember { mutableStateOf(0) }
```

`by` 将后面的 MutableState<T> 对象的 getValue, setValue 委托给 count var.

而  `{ mutableStateOf(0) }` 就是尾随 Lambda, remember 的第一个参数，省略了圆括号。

## 带接收者的函数类型 (Function literal with receiver )

```Kotlin
fun buildString(action: StringBuilder.() -> Unit): String {
    val sb = StringBuilder()
    sb.action()  // 在lambda内部，this指向sb
    return sb.toString()
}

val html = buildString {
    append("<html>")
    append("</html>")
}
```

这段代码是天书，我真是看不懂。看了 DS 的解释，才大概知道。这里的 `StringBuilder.` 就是所谓的 receiver, 表示 
1. action 函数是在 StringBuilder 上执行的
2. action 函数体内部，自动获得 this 指向 StringBuilder, 也就是内部也在 StringBuilder 作用域下

所以可以认为，这个 Function literal with receiver 是给框架作者用的！用户通过 lambda 尾随函数指定 receiver 要干的的事项，框架作者通过这个 Function 设置串联的逻辑。高阶函数。

最好理解等效的版本（函数版）：

```Kotlin
fun buildStringTraditional(action: (StringBuilder) -> Unit): String {
    val sb = StringBuilder()
    action(sb)  // 要把 sb 作为参数传进去
    return sb.toString()
}

val html2 = buildStringTraditional { sb ->
    sb.append("<html>")
    sb.append("</html>")
}
```

给类增加临时函数版：

```Kotlin
fun StringBuilder.buildStringTemporaryMethod() {
    append("<html>")
    append("</html>")
}

val sb = StringBuilder()
sb.buildStringTemporaryMethod()
```

## DSL

上面有点学习成本的语法，都属于是 DSL. 其实就是语言自己的一些语法糖或者设定。

还有个定义路由的例子：

```Kotlin
// 没有 DSL（传统写法）
fun setupRoutes(router: Router) {
    router.get("/users", handler::listUsers)
    router.post("/users", handler::createUser)
    router.get("/users/{id}", handler::getUser)
}

// 有 DSL（Ktor 框架实际用法）
routing {
    get("/users") {
        call.respond(listUsers())
    }
    post("/users") {
        call.respond(createUser())
    }
    get("/users/{id}") {
        call.respond(getUser(id))
    }
}
```

- 传统代码：告诉计算机**怎么做**（创建对象、调用方法、转换类型）
- DSL 代码：告诉计算机**做什么**（构建字符串、定义路由、描述 UI）

## Kotlin 文化

简洁优于显式: 代码要短，假设读者懂

## Gradle 编译

上来就被编译给干懵了。

1. `libs.versions.toml` 本质上就是一种官方推荐的“最佳实践和设计模式”，它在 Gradle 里被称作 Version Catalog（版本目录）。它和 Python 的 pyproject.toml 有着本质的区别。在 Gradle 中，这个 .toml 文件只是为了解决“当项目有几十个模块时，改一个版本号要改几十遍”的痛点而引入的一个全局变量本子。Gradle 只是在后台默默地帮你生成了一个名为 libs.versions.kotlin 的代码变量。这个变量现在静静地躺在内存里，如果你不在 build.gradle.kts 里去调用它，它对整个构建过程产生不了任何实质影响。
    Gradle 只认 build.gradle.kts, libs.versions.toml 只是版本定义的一种最佳实践而已。

## Kotlin 包可见性

同 package 内部是互相可见的，即使跨文件，也不需要 import.

package 是由文件头部的 package xxx 决定的，而非文件路径！！当然一般来说，对应起来最好。

## 类的定义

```Kotlin
// 在这里就定义好构造函数的参数；
// 其中带 val/var 的，自动成为 class 的属性
// 不带的，就只是参数
class Person(val name: String, inputAge: Int) {
    // 属性、方法默认都是 public
    // 可用 private/protected/internal 来修饰
    private val saveAge = inputAge - 1
    // 可以定义延迟初始化的 var 属性；但访问未初始化的属性要崩溃；一般避免
    lateinit var job: String

    // 初始化方法
    init { 
        println("Person ${name}")
    }
    // 公开方法；一般这种属性设置值只用用 setter
    fun setJob(job: String) {
        this.job = job;
    }
}
```

太多的隐式规则了。

## sealed class, 嵌套定义, class, data class, object, data object

sealed class, 密封类，类似增强版 Enum：可以听一不同类型的子类，然后用在 when 的地方，作用类似枚举。

可以嵌套定义，就是子类在父类的内部直接继承定义，也可以外部定义, 一般都是嵌套定义：

```Kotlin
sealed class State {
    object Idle : State()
    data object Loading : State()
}

data class Success(val data: String): State()
class Error(val msg: String) : State()
```

object, 表示定义的是单例；

class 就是类型，有多实例。

data object 对 toString 等方法有优化，推荐用；

data class 表示是数据类，预定义好了 hash 等方法，类似 Python 里的 dataclass.

而基于 sealed class 下面可以有各种不同类型的子类，这就比普通 Enum 强太多了。