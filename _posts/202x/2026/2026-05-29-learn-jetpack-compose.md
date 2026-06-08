---
layout: post
title: Jetpack Compose 学习
date: 2026-05-29
categories: 技术
tags: JetPack-Compose Android
---

> 让 Minimax 整了个 codebase，结果发现发送消息后消息没有刷新出来。是时候真正的开始学习 JetPack Compose 然后手工改了。

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

## Domain / Data 依赖倒置

在 Data 层依赖 Domain 去做 Impl. 在 Domain 层定义 Interface / Model.

通过 Hilt 来绑定 Impl.

Data 数据  -> Domain.Model 的转换，可以在 Data 层定义 mapper, 或者直接写到 Data 层的 Repository 里。
绝对不能写到 Domain 层！ Domain 层对 Impl 是无感的。

这个算是数据设计的通用规范/标准设计模式了，不算是 Compose 特有。

## 数据库字段删除: Hard or Soft

客户端本地数据库（SQLDelight）：强烈推荐 【Hard Delete（物理删除）】

- 手机存储空间极度宝贵：如果用户注销了账号，或者删除了某个用户，你必须把该用户相关的图片、缓存、历史消息全部清理干净。逻辑删除（只改个标志位）会导致垃圾数据永久占用用户的手机空间，时间久了 App 就会变成“存储空间杀手”。

- 极简的级联删除（Cascade）： 在 SQLite 中，你只需开启外键约束，设置 ON DELETE CASCADE。删除了 user 表中的行，该用户的所有消息、状态、群成员记录会被数据库底层自动一并抹去。

服务端数据库：通常推荐 【Soft Delete（逻辑删除）/ 匿名化】

逻辑删除的痛点：
- 查询污染：你几乎所有的查询都必须带上 WHERE is_deleted = 0。
- 联合查询变复杂：查消息时，还得额外 Join 检查发送者是否已被删除。
- 唯一索引冲突：如果 username 是唯一的。用户 A 删除了账号（逻辑删除，记录还在），新用户 B 想用同一个 username 注册，就会因为唯一索引冲突而失败。

服务端删除的最佳实践：【匿名化（Anonymization）】

为了遵守隐私法律（如 GDPR 要求“被遗忘权”），同时又不破坏数据库的关联完整性，服务器端目前最流行的是**“匿名化物理删除”**：
- 当用户选择“注销账号”时，服务器执行一个事务：
  - 将该用户的 username、avatar_uri、email 等所有能识别个人身份的信息（PII）抹去，替换为 Ghost、已注销用户 或随机无意义字符。
  - 将该用户的 is_active 状态设为 false。
  - 保留主键 id 以及他们发过的消息。

- 效果：数据库关联没有断（不需要级联删除海量消息），但用户的个人隐私数据已经被物理擦除了。

特定场景的处理：

1. 用户注销：这个时候直接往服务器发送阻塞请求，待服务器处理完后，本地直接删除数据（数据库+Preference)
2. 服务器通知 xxx 资源已被删除：本地硬删除
3. 本地删除了某个东西：先软删除，同步给服务器；服务器完成同步后，本地再硬删除
