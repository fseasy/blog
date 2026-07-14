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

如果直接写一个 Screen，常常要把 ViewModel 作为参数传进来，而它一般依赖 hiltViewModel() 注入, 让 Preview 构造复杂。

最佳实践：状态提升（State Hoisting）与 屏幕组件拆分

为了获得最好的预览体验和代码可测试性，业界标准的做法是采用 “Container - Content” 拆分模式。

Compose 推荐的架构是将组件分为两层：

1. 顶层：Container / Smart Component（智能/有状态组件）
  通常以 Screen 或 Page 结尾。
  这一层专门用来接收 ViewModel，负责订阅数据源、处理生命周期、分发业务事件,处理副作用，。它不需要高频复用，整个页面只有一个。
  不包含任何 UI 逻辑

2. 底层：Stateless Content / Dumb Component（呆萌/无状态组件）
  比如 MessageBubble、AudioPlayerBubble、Avatar。这一层绝对不碰 ViewModel。
  它们只接受最基础的、Immutable 的数据类型（如 String、Boolean、MessageUiModel）以及点击事件的 Lambda 回调。
  完全可以预览！



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

## Flow 与 StateFlow

1. 在 ViewModel 层，有必要将 Flow （冷流）转为 StateFlow （热流）
   1. 避免重复的磁盘 I/O 操作（共享数据源）；否则每次 collect 都会触发上游执行
   2. 应用配置变更（如屏幕旋转）：屏幕旋转时 UI 会重组，时用 StateFlow 配合 started = SharingStarted.WhileSubscribed(5000) 可以避免重新加载数据
   3. UI 层需要状态而非事件：StateFlow 保证任何时候下游能都拿到值
   4. 支持同步获取最新值：可以通过 StateFlow.value 来同步获取值，而不需通过 Flow.first/collect 去异步收集
2. 什么时候没必要？
   1. 此 Flow 会被进一步组合：如果这个 Flow 不是单独暴露给 UI，而只是作为一个大的 UiState 的一部分（通过 combine 合并多个 Flow），那么可以在整体的 UiState 层面做 stateIn
   2. 单次消费：只是读一次 .first, 不需要监听变化
3. Repository 应提供 Flow，ViewModel 负责转为 StateFlow
   1. StateFlow 表示 UI 状态管理，它依赖 AndroidX 的生命周期（scope 参数！一般是 viewModelScope），而 Repository 不应该和特定的 ViewModel 绑定
   2. StateFlow 需要提供初值，而初值和 UI/特定业务 相关联的，在 Repository 层不应该绑定


## Modifier 是上层传进来还是自己内部控制？

通常需要作为参数“传来传去”（传递），但需要结合“内部编写”一起使用。

在 Compose 的设计规范中，推荐的做法是：
- 让外部（调用者）决定组件的大小和位置，
- 让内部（组件自身）决定其具体的内容和业务样式。

## suspend, withContext(Dispatches.IO), viewModelScope.launch

suspend 是一个面向协程的承诺：
1. 这个函数只能在协程环境里跑
2. 内部有挂起的点（协程调度退出当前函数、去跑别的协程的点）

withContext 是说切换“线程“ — 这和协程是正交的。

viewModelScope.launch 是在 viewModelScope 启动一个协程—所谓的启动，就是立刻创建一个 job，
job 里包含了大括号里的代码，然后把这个 job 提交给协程调度器后立刻就返回了！它是异步、非阻塞的。

所以 viewModel 里才频繁用这个 launch：因为它只是提交了任务，任务执行在另外的地方，从而不会阻塞 UI.

## Clean Architecture

清洁架构，要求 UI (presentation) 依赖 domain 层；data 也依赖 domain 层；
然后 domain 层只依赖 JVM 系统的东西。

写起来要多一些转换，但又觉得有一点道理—强迫你去拆分、归类。

特别是开始入门写 UseCase 后，感觉有点茅塞顿开的感觉：业务逻辑写到 UseCase, 依赖的基础操作写到 Repository (domain 层), UI 优先使用 UseCase. 一切都顺畅起来，不用再纠结哪个放哪里了。

当然抽象都是有代价了，前面起到的“类型转换“就是最直接的问题，比如 Android 的 Uri 就需要转成 String.
有洁癖的人有点纠结，但还是架构优先，稍微费一丢丢 CPU，还行吧。

## 避坑：在 UI 的 LaunchedEffect 里去调用一些修改 db 的 side effects

实际遇到的 2 个场景：

1. 进入注册页，需要直接触发注册逻辑 （app 设计使然）
2. 进入 Welcome 页，基于传入的 needCreateTopic 去调用 createTopic 逻辑

之前都是放到 LaunchedEffect(Unit) 里直接调就是。但这在每次 UI 重组时，会导致重复调用！

解决方法：

最推荐的还是，把这些 `进入就 xxx`的逻辑直接放到 viewModel 的 init 里（如果页面刚好对应 1 个 ViewModel，且参数满足要求，写到 init 里最好）；

如果不行，可以在 ViewModel 的调用函数里，增加一个 flag 用于跳过重复创建。因为 UI 触发(LaunchedEffect) 和 viewModel 函数执行都是在主线程，所以只要不把判断+设置的逻辑写到协程里(viewModelScope.launch)，也不用加锁，简单高效。

当然还可以在 UI 层设置 flag, 但用 rememberSavable 来持久化状态，但更麻烦，心理上也觉得不够稳定。

## where to put: 现在要创建一个 topic, 需要往主表和关联的 2 个表各自插数据，这个逻辑应该放到 UseCase 还是 Repository?

**DeepSeek**:

无脑放到 UseCase, Repo 应该只管自己表的 CRUD 操作。

- 如果需要向上暴露事务，可以在 Domain 层定义接口：

  ```kotlin
  // domain/TransactionManager.kt
  interface TransactionManager {
    suspend fun <T> withTransaction(block: suspend () -> T): T
  }
  ```

  然后在 data 层实现这个接口即可。

  ```kotlin
  // data/TransactionManagerImpl.kt
    class TransactionManagerImpl(
        private val db: MyDatabase // SQLDelight 数据库实例
    ) : TransactionManager {
        override suspend fun <T> withTransaction(block: suspend () -> T): T {
            return withContext(Dispatchers.IO) {
                db.transactionWithResult { // 假设 SQLDelight 提供了类似 API
                    block()
                }
            }
        }
    }
  ```

- 还有一种是定义 UnitOfWork, domain 层接口如下

  ```kotlin
    // domain/UnitOfWork.kt
    interface UnitOfWork {
        suspend fun <T> execute(block: suspend (UnitOfWork) -> T): T // execute in transaction
        fun getTopicRepository(): TopicRepository
        fun getTagRepository(): TagRepository
        // ... 可以获取其他 Repository
    }
  ```

  看到有点惊艳，这个一弄，相当于所有的 repo 和操作都可以写了啊。


**Gemini 3.5 Flash**

- 主要取决于这些操作是否需要保证事务原子性（Transaction）以及数据源的复杂程度。

- 如果这三个表的写入操作都是在同一个本地数据库中进行，且必须作为一个完整的事务执行（即要么全部成功，要么全部失败），那么这个逻辑更适合放在 Repository（或直接在 DAO）中。
  - 事务控制（ACID）： 数据库事务属于数据访问层的技术实现细节。UseCase 属于领域层（Domain Layer），不应该感知具体数据库事务
  - 接口一致性： Repository 向外暴露一个干净的接口（如 createTopic(topic, tags, attachments)），内部封装具体的表操作，使上层无需关心底层是如何分表存储的。

- 如果这三个表的写入涉及不同的数据源（例如：先通过网络 API 创建 Topic 拿到 ID，再往本地数据库写入关联表），或者涉及多个不同的 Repository，那么这个逻辑应该放在 UseCase 中。

看了两个的回答，都是受益匪浅啊。架构设计真的挺有意思的。

## Q: Domain model 耦合 UI 需求？


{% capture body %}
<div markdown="1">
Topic 目前有主表 Topics+3 个副表 (topic_members, topic_personal_state, topic_last_message)，
然后我 IM 主页显示 Topic 列表， 这里 有 2 个逻辑其实：1. 筛选出自己加入的 topic 2. 展示 topic, 展示时需要 topics, topic_personal_state 和 topic_last_message， topic_members 的一些部分信息。
我在想 Model 层怎么定义这个对应的 model 和查询逻辑。

目前我准备一次性全部查出来满足条件的 topic，但是返回 model 上，是一股脑独自全部返回？（有点难，特别是 members 可能是多行），还是说筛选出首页需要的字段——也就是单独定义一个首页需要的 Model. 但是这是不是又耦合了 UI 的逻辑？

有点纠结，最佳实践是什么？我想成熟的 IM 应用肯定都有最佳实践吧？
</div>
{% endcapture %}

{% include component/fold.html
   type="note"
   summary="Question"
   open=false
   content=body
%}


{% capture body %}
<div markdown="1">


在成熟的 IM 应用或复杂的移动端架构中，针对你描述的这种“主页会话列表（Conversation List）”场景，业界普遍采用的方案是读写分离（CQRS 思想）以及定义专用的领域查询模型（Read Model / Projection）。

下面为你梳理具体的最佳实践、模型定义方式以及如何解决 1:N（如群成员）的查询痛点。

**最佳实践**：定义专用的“会话”领域模型（并非耦合 UI）

你担心的“单独定义一个首页需要的 Model 会耦合 UI 逻辑”，其实是一个常见的误区。

在领域驱动设计（DDD）中，这个 Model 不叫“UI Model”，而叫 领域查询模型（Domain Read Model / Projection），通常命名为 Conversation 或 TopicSummary。
- 写模型（Write Model）：对应你数据库里的物理表（Topics, topic_members 等），它们用于保证数据的完整性和一致性。
- 读模型（Read Model）：对应业务上的一个概念。在 IM 中，“会话（Conversation）”本身就是一个高阶的业务实体，它天然就包含：主题信息、最后一条消息、个人未读状态、以及部分成员信息。

因此，单独定义一个 Conversation 模型放入 Domain 层，不仅没有耦合 UI，反而是高内聚、低耦合的表现。

**SQLDelight 中的查询逻辑设计**

为了保证性能，不建议分别查询各个表然后在 Kotlin 内存中进行 zip 或 map 组装（尤其是列表数据量大、滑动频繁时）。建议利用 SQLDelight 的多表 JOIN 一次性查出所需字段。

核心挑战：如何处理 1:N 的 topic_members？

主页列表通常不需要完整的成员列表，只需要：

- 成员总数（用于显示群人数）。
- 前几个成员的头像（用于合成群头像，通常最多 4 个）。

如果直接 LEFT JOIN topic_members，会导致数据集膨胀（一个 Topic 因为有 N 个成员而返回 N 行数据），这在分页查询中是灾难性的。

解决方案：

- 方案 A：利用 SQLite 的聚合函数（推荐）
  使用 GROUP_CONCAT 或 JSON 组装（如果 SQLite 版本支持），将前几个成员的头像或 ID 合并成一个字段返回，然后在 Kotlin 中解析。
- 方案 B：冗余设计（工业界常用）
  在 Topics 表中冗余一个 member_count 字段和 group_avatar_urls（以逗号分隔的字符串或 JSON 格式）。在群成员发生变化时，异步更新这两个字段。这样主页查询就变成了纯粹的 1:1 关联，性能极高。

</div>
{% endcapture %}

{% include component/fold.html
   type="note"
   summary="Answer from Gemini 3.5 Flash"
   open=false
   content=body
%}

{% capture body %}
<div markdown="1">

DeepSeek 实在是讨好型人格，另外也有点说不清楚。

```
“是不是需要为 UI 单独设计 domain model？”

答案：不需要。 除非有明确的复用或性能需求，否则让 Domain/Data 层返回“扁平的全部数据”，让 UI 层（ViewModel）按需裁剪和转换，是最佳且最常用的实践。
```

```
你抓到了我表述中一个非常关键且容易误导人的点。非常抱歉，“原样”这个词我用得极不准确，它绝不意味着“不裁剪字段”或“SELECT *”。
```

一点点追问，总算是理解了它的观点吧（或者我期望的观点？）：

- 在 data 层，查询的时候就要基于 UI 需求，写好 SQL，取好需要的字段
- 在 domain 层，对 data 层的数据库字段做转换：最可能的就是类型转换，比如反序列化 avatar 路径，int -> bool 等
- 在 UI 层，针对 UI 逻辑做渲染需求的字段准备：最直观的例子就是时间，要从原始时间戳转为 “xx 分钟前“ 这种用户友好的样式

</div>
{% endcapture %}

{% include component/fold.html
   type="note"
   summary="DeepSeek 深度思考"
   open=false
   content=body
%}

所以结论就是，还是要针对 UI 需求去设计查询、domain model 等。而且这是必要的。

感觉自己对架构分层有点着魔了，而且容易走火入魔入了邪道啊。学习得太少，想得太多。

## Q: Domain 层的 UseCase 如何启动 WorkerManager 以及 chain

{% capture body %}
<div markdown="1">
Android jetpack Compose, Kotlin，我在开发一个类似 IM 的应用

在 domain 层的 UseCase 里，我想定义一个 save message media file 的 usecase, 大概步骤是
- insert basic message info to db, with raw file uri (from SAF)
- copy raw file to internal, and then update db with the copied file path
- generate thumbnail, and then update db with the thumbnail path
- update db to remove the raw file uri

这 4 步，我想在 Androird 层面用 WorkerManager 来依次运行，方便重试+保证稳定性。
我想知道，domain 层怎么写呢？主要是要调用 worker manager 来创建 task
目前想到的方式是定义一个 WorkerManager interface ，同时定义 worker manager 的 task definition，domain 层就是传这些 definition，在 data 层（Worker 层）实际执行，但是还需要支持这种 chain 的模式
另外，这种设计思路整体是对的吗？最佳实践是什么？
</div>
{% endcapture %}

{% include component/fold.html
   type="note"
   summary="Question"
   open=false
   content=body
%}

**Gemini 3.5 Flash**:

{% capture body %}
<div markdown="1">

```Kotlin
// domain/model/MediaProcessingTask.kt
// 定义一些与具体实现无关的任务定义
data class MediaTaskConfig(
    val messageId: String,
    val rawFileUri: String
)

// domain/repository/MediaProcessingScheduler.kt
interface MediaProcessingScheduler {
    /**
     * 调度媒体文件处理任务
     */
    fun scheduleMediaProcessing(config: MediaTaskConfig)
}

// domain/usecase/SaveMessageMediaUseCase.kt
class SaveMessageMediaUseCase(
    private val messageRepository: MessageRepository,
    private val mediaScheduler: MediaProcessingScheduler
) {
    suspend operator fun invoke(rawFileUri: String, chatSid: String): Result<Unit> {
        return try {
            // 1. 立即插入基本消息到 DB (状态为 Sending)，获取 Message ID
            val pendingMessage = createPendingMessage(rawFileUri, chatSid)
            val messageId = messageRepository.insertMessage(pendingMessage)
            
            // 2. 调用调度器去后台执行后续的拷贝、缩略图生成和更新操作
            mediaScheduler.scheduleMediaProcessing(
                MediaTaskConfig(
                    messageId = messageId,
                    rawFileUri = rawFileUri
                )
            )
            Result.success(Unit)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    private fun createPendingMessage(rawFileUri: String, chatSid: String): Message {
        // 构建一个临时的消息对象，包含原始的临时 URI 
        ...
    }
}
```

-> 也就是在 Domain 层面只需要发 1 个 task，然后在 worker 层面内部再去做：可以多个任务串联，也可以直接 1 个 worker 写完。

</div>
{% endcapture %}

{% include component/fold.html
   type="note"
   summary="如果要实现 WorkerManager "
   open=false
   content=body
%}

但是，创建 workmanager 可能并不是最佳的方式：

1. worker manager 调度是有延迟的，有可能调度到的时候，src uri 权限已经没了
   权限丢失的 workaround: 可以直接申请 persistable 权限，但是一个应用可申请 persistable 的权限数量是受限的—所以在完成任务或者彻底失败后，需要 release 掉这个权限！
2. worker manager 同样不能 100% 保证不被杀，都要考虑任务失败的情况

## Q: IM 应用里保持消息文件的最佳实践

{% capture body %}
<div markdown="1">
IM 应用的最佳实践：“即用即弃，即时拷贝”

1. 在 IM 应用中，通用的做法是将 SAF 返回的 URI 视为临时的一次性通道，遵循**“立即读取、立即拷贝、尽早释放”**的原则。
推荐的优化流程：
  - 不需要申请永久权限，而是微调你的架构流程，将“拷贝”这一步的优先级提前或优化：
  - 临时权限的有效期：通过 ActivityResultContracts.GetContent 或 StartActivityForResult 获取的 URI 临时权限，在应用进程存活期间通常都是有效的。

2. 最佳方案：在入队 WorkManager 前（或在 Repository 中）同步完成“拷贝到临时缓存”：
   
  - 当用户在 UI 界面选好文件后，立刻在主进程的协程（Repository 级别）中，将该 URI 的文件流复制到应用的内部缓存目录（例如 context.cacheDir）下一个临时的文件中。
  - 这个拷贝操作是在当前进程生命周期内立即执行的，因此绝对不会遇到权限失效的问题。
  - 拷贝完成后，你得到了一个完全属于你应用私有的临时文件路径（例如 app_cache/temp_12345.jpg）。

3. 将临时文件路径传递给 WorkManager：
  - 此时，你启动 WorkManager，传递的是这个私有临时文件路径，而不是原始的 SAF URI。
  - WorkManager 即使延迟了几分钟甚至几小时执行，它读取自己沙盒内的临时文件也不需要任何系统权限。
  - 后续的 Worker 步骤变为：
  - 从临时目录移动到正式的内部存储目录。
    - 生成缩略图。
    - 数据库更新，并删除临时文件。

因为应用后台、被杀导致流程失败的解决方案

在移动端开发中，“用户强杀 App”通常被视为一种显式的、用户主动中止行为。大多数 IM 应用（包括微信、Telegram 等）在这种极端情况下，会允许本次发送任务失败。

核心流程：

- 状态标记：
  - 在数据库中插入消息时，初始状态设为 Draft（草稿）或 Sending（发送中），并记录临时拷贝的目标路径。

- 异常清理：
  - 每次 App 启动时，或者 WorkManager 发现任务因权限丢失而失败时：
    - 将该消息状态置为 SendFailed（发送失败）。
    - 并在 UI 上给这条消息显示一个**“红色的感叹号”**。
    - 清理掉之前可能拷贝了一半的半截文件。

- 用户行为：
  - 用户在聊天界面看到“发送失败”后，点击“重试”按钮。此时因为之前的临时 URI 已经失效，App 会提示用户“文件已失效，请重新选择并发送”。

总结：IM 开发的黄金法则

| 阶段                                  | 推荐技术                          | 为什么？                                         | 如果被杀怎么办？                                                                                          |
| :------------------------------------ | :-------------------------------- | :----------------------------------------------- | :-------------------------------------------------------------------------------------------------------- |
| **1. 插入数据库 + 拷贝 + 生成缩略图** | **主进程协程 (`Dispatchers.IO`)** | 保证 UI 零延迟响应，立刻显示缩略图，顺滑度最高。 | 冷启动对账机制：检测 `Sending` 状态，无本地文件则标记为 `FAILED`。                                        |
| **2. 小文件上传 (< 20MB)**            | **应用级别的协程 / 任务队列**     | 快速发送，逻辑简单。                             | 冷启动对账机制：检测到有本地文件，自动重新触发上传或标记为 `FAILED`。                                     |
| **3. 大文件发送 / 后台静默发送**      | **WorkManager**                   | 保证应用退到后台、锁屏后，大文件依然能慢慢传完。 | 结合“方案一”的 `releasePersistableUriPermission` 机制，或者确保 Worker 启动前文件已经安全存在于缓存目录。 |

</div>
{% endcapture %}

{% include component/fold.html
   type="note"
   summary="最佳实践推荐"
   open=false
   content=body
%}

最后，缩略图推荐放到永久目录，自定义删除逻辑，或者给用户清理的选项。放到系统的 cache 目录，其并不可控，而 thumbnail 对 IM 是至关重要的！