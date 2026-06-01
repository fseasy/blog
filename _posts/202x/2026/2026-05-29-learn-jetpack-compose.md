---
layout: post
title: Jetpack Compose 学习
date: 2025-05-29
categories: 技术
tags: JetPack-Compose Android
excerpt: 让 Minimax 整了个 codebase，结果发现发送消息后消息没有刷新出来。是时候真正的开始学习 JetPack Compose 然后手工改了。
---

## `@Composable` 编译器插件的类型修饰符

包名：@androidx.compose.runtime.Composable

作用：告诉 Jetpack Compose 编译器，这是一个基于数据驱动 UI 的函数。

特性：

- 被装饰的函数，底层函数签名被 Compose 编译器重写，增加一个 Composer 实例用于追踪和构建 UI 树
- Composable 函数只能在另一个 Composable 里调用
- 数据驱动：当依赖的数据变化时，UI 自动重绘，这个过程叫 Recomposition, 或者“重组“

和 Reactive Native / Flutter 里的概念都是一致的。 UI = F(state).

重组的动力，来自 2 个：函数接收的外部 parameters, 内部 state.

- 基本可以接受任意参数；参数如果都是不可变的，或者用 @Immutable/@Stable 修饰，那么表明组件不会受到外部重组的影响。
- 内部定义了 mutableStateOf / Flow.collectAsState / SnapshotStateList，就有状态

## Compose 推荐的架构设计

Compose 推荐的架构是将组件分为两层：

1. 顶层：Smart Component（智能/有状态组件）
通常以 Screen 或 Page 结尾。这一层专门用来接收 ViewModel，负责订阅数据源、处理生命周期、分发业务事件。它不需要高频复用，整个页面只有一个。

2. 底层：Dumb Component（呆萌/无状态组件）
比如 MessageBubble、AudioPlayerBubble、Avatar。这一层绝对不碰 ViewModel。它们只接受最基础的、Immutable 的数据类型（如 String、Boolean、MessageUiModel）以及点击事件的 Lambda 回调。

```Kotlin
// 1. 顶层页面：负责提线
@Composable
fun MessageListScreen(viewModel: MessageViewModel) {
    // 将 Flow 转化为 Compose State
    val messageList by viewModel.messageListState.collectAsState()

    LazyColumn {
        items(messageList) { uiModel ->
            // 2. 底层组件：只接受最小化、干净的数据
            MessageBubble(
                content = uiModel.content,
                isStarred = uiModel.isStarred,
                onStarClick = { viewModel.toggleStar(uiModel.id) } // 把动作通过 Lambda 传上来
            )
        }
    }
}

// 3. 底层组件：纯工具人，完美闭环
@Composable
fun MessageBubble(
    content: String,
    isStarred: Boolean,
    onStarClick: () -> Unit
) {
    // 纯粹的渲染逻辑，性能极高，可完美跳过重组，支持任意页面复用，Preview 极其简单
}
```

PS：如果组件依赖的参数多，可以把组件依赖的不可变参数列表放到一个 immutable 的 dataclass 里，命名为 xxxUiModel. 区别与 ViewModel, 这个就是给组件渲染用的直接参数，不可变。这个 UiModel 可以放到和这个组件一个文件里。

## 异步任务

在 Compose 中处理异步任务有 2 种方式：

- LaunchedEffect（自动触发）：如果是页面一加载（或数据一改变）就需要自动执行的异步任务（例如：一进入 LogTimelineScreen 就从底层 SQLDelight 数据库加载消息记录），用 LaunchedEffect。

- rememberCoroutineScope（用户触发）：如果是用户做了一个操作（点击、滑动）后才需要执行的异步任务（例如：点击按钮打开抽屉、点击按钮滚动列表到最底部、点击按钮保存一条新 Log），必须用 scope.launch。

