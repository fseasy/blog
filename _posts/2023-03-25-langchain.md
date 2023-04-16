---
layout: post
title: LangChain 学习
date: 2023-03-25
categories: 技术 
tags: LangChain LLM
---
> 让 LLM(Large Language Model) 调用外部工具形成更强大的应用，这在最近成了一个趋势。
从 LaMDA, ToolFormer 到 Visual ChatGPT 再到引轰动的 ChatGPT plugins, 我们看到了这个方向的价值和空间。

最近自己也在想如何用 LLMs 来驱动已有的工具，但没有想好一个妥善的实现方式。
基于此，我先调研了下 Visual ChatGPT, 发现其使用了 LangChain 这个工具。
LangChain 有开源代码且独立于 LLMs， 读了 README, 感觉它很符合自己的期望啊。那就深度优先，先来学习下这个工具吧。

## README: 什么是 LangChain

*⚡ Building applications with LLMs through composability ⚡*，
从 LangChain [代码库][_repo]的这句简介，我们可以快速了解到它是做什么的：

- 是一个做 LLMs 相关应用的开发工具
- 主要通过组合 LLMs 和其他工具来实现

回过头看 LangChain 的 Logo: 🦜️🔗. 这是两个 emoji 字符。
第一个彩色鸟，查了下是[鹦鹉(parrot)][_parrot]，
对应到 *Lang(uage)*, 应该是取鹦鹉可以说话这个点，突出这个工具以自然语言作为交互入口；
第二个字符是锁链, 对应到 *Chain*, 直观表达了组合串联的思想。
这个工具的名字和 logo，还是挺清晰简洁的，完美体现了人类的智慧（不会是机器取的吧233）。

在 README 里，作者说 LLMs 这种变革性的技术让开发者可以创造以前不可实现的应用，但是一个单独的语言模型，还是力有不逮，
给它加点其他来源的计算和知识，才能实现真正的强大。
LangChain 基于此动机，着力于开发出一套工具来让 LLMs 利用外部能力。而且，这不是构想，其已经围绕几个落地点做出了一些示例：

1. 在特定文档上做问答, 例子是 [Notion-QA][_notion_qa]
2. 聊天机器人，示例为 [ChatLangChain][_chat_langchain]
3. 代理工具(Agent)， Example 是 [GPT+WolframAlpha](_gpt_wolframalpha). 
   [WolframAlpha][_wolfram] 是一个在线问答系统，这个例子部署在 Huggingface 上。

感觉这块做得还挺大的，不过目前 LangChain 相关项目还是挂在 hwchase17 这个个人账户下的，厉害啊！

从功能点的角度看， LangChain 的能力可以落到 6 个点上，由简单到复杂，依次为：

1. LLMs 和 Prompts: 包括对所有 LLMs 的通用接口封装，一些调用 LLMs 的公共能力；对 Prompts 的管理、优化能力
2. Chains: 具备将 LLMs 和其他工具串联的能力；对 chains 提供了标准接口，提供了一些和其他工具结合的中间层，
还对常用应用预制了端到端的chains.
3. 基于特定数据的生成(Data Argumented Generation): 构建了一个特定的 chain, 先从目标数据源里找数据，
再给 LLMs 做生成。比如指定长文摘要、特定数据下的问答。
   > 我们看到目前很多产品都是这个链路。比如 Paper 问答，甚至 new bing.
4. Agents: 所谓 Agent， 就是让 LLMs 作为代理，决定现在要做什么操作、执行操作、对操作结果做解析并基于此结果做后续操作。

   Agent 有很多种（面向不同场景，从后面的文档可知，有通用问答场景、数学计算场景等），
   LangChain 定义了标准的 agent 接口，并提供了一些标准 agent 供选择，还有一些端到端的 agent 的例子。
5. Memory: 用来在一系列 chains/agents 调用中保存状态的。
   LangChain 提供了 memory 的标准接口，一系列 memory 的实现和 chains/agents 使用 memory 的例子。
6. 评估： 生成式模型很难做效果评估，一种新的评估方法是用他们自己去评估自己。
LangChain 提供了一些 prompts/chains 来辅助做这个事情。

## 快速开始

在 [QuickStart guide][_quickstart] 文档里，列举了快速上手的例子，这些例子和上面提到的功能点是基本对应的。

### 基础元素： LLMs, Prompts

1. LLMs: 构建一个 LLM 的实例并获取其返回结果。这个比较简单，可以认为就是已有 API 的封装。
   但这个实例是后续步骤的基础(被称为是 *one primitive*)。

2. Prompts 模板：实现了一个提示模板类。似乎就是 `Python str.format` 的一个封装，但这同样是基础模块。

### 串联: Chains

给的例子是预先定义的 `LLMChain`：
   
用 `chain = LLMChain(llm=llm, prompt=prompt)` 实例化后，
直接`chain(prompt_input_var)` 调用就可以完成 1. 提示模板填充 2. 请求 LLM 并返回 这个两个步骤。

看到这里觉得还 OK， 只是工程上的封装，还没有体现出 LLMs 驱动工具的功能；
不过 Chain 这个点已经看出来了——是要通过定义类来实现的。

### 基于用户输入自动调度： Agents

前面说到，Agent 就是在接受到用户的输入后，让 LLMs 来决定要执行什么操作、以什么顺序来执行。
这是 LLMs 体现 AGI(Artificial General Intelligence, 强人工智能) 能力的重要特征。

LangChain 在实现这个功能时，使用了 3 个概念（抽象/模块）：

1. Tool: 实现特定职责的函数，是调度的最小单元。
工具可以是 *Google Search*, *Python REPL(Read-Eval-Print-Loop)* 等基础元素，也可以是一个 chain.
   需要实现一个统一的接口，目前这个接口是 `(input: str) -> str`. 
   
2. LLM: Agent 的核心能力.
3. Agent: Agents 的具体实体，在 Tools 和 LLM 之上构建的调度器。
   
   至少需要实现以下的逻辑：
   - 基于 LLMs 输出判断是调用工具还是返回给用户
   - 调用哪个工具？
   - 为工具调用准备输入，并将输出反馈给 LLMs

   LangChain 预置了几个 Agent, 其中默认是 *zero-shot-react-description*, 这是基于 [ReAct: Synergizing Reasoning and Acting in Language Models][_react_paper] 论文的方法。咱们后面再看。

给了一个基于用户 Query，内部先做搜索、再调用计算器、最后返回自然语言结果的例子。
这个例子很棒，基本和 LaMDA 论文里的功能类似了。我们仔细看下运行过程：

- 用户输入： *Who is Olivia Wilde's boyfriend? What is his current age raised to the 0.23 power?*
  
  要回答这个问题，需要先找到这个人的 bf，再找 bf 的年纪。最后再算一下幂值。这个问题有点刻意了，不过作为示例也还行。

- Agent 分析输入， 确定步骤：
  
  ```
  Entering new AgentExecutor chain...
  I need to find out who Olivia Wilde's boyfriend is and then calculate his age raised to the 0.23 power.
  ```

- 连续找人、找年龄
  
  ```
  Action: Search
  Action Input: "Olivia Wilde boyfriend"
  Observation: Jason Sudeikis
  Thought: I need to find out Jason Sudeikis' age
  Action: Search
  Action Input: "Jason Sudeikis age"
  Observation: 47 years
  ```

  这里面的 Thought 挺有意思；输入、输出的解析也很关键。

- 计算幂
  
  ```
  Thought: I need to calculate 47 raised to the 0.23 power
  Action: Calculator
  Action Input: 47^0.23
  Observation: Answer: 2.4242784855673896
  ```
  
  调用了计算器，算出结果

- 生成最终结果，返回

  ```plain
  Thought: I now know the final answer
  Final Answer: Jason Sudeikis, Olivia Wilde's boyfriend, is 47 years old and his age raised to the 0.23 power is 2.4242784855673896.
  ```

看完这个流程，觉得真牛啊……

### 记录历史信息： Memory

如果服务是有状态的，那么就需要一个 Memory 来记录历史的信息。这里是用 Chatbot 任务来举例的：
一般地，新的对话依赖前面的对话内容，即 *short-term memory*；
特定情况下（如“你还记得大明湖畔的夏雨荷吗？”），还需要很久之前信息来作为上下文，即 *long-term memory*.

LangChain 在这里没有直接暴露 Memory 接口，而是使用了包含记忆的 *ConversationChain* 来做例子。
从例子里看到，LLMs 本身是无状态的，
通过将 memory 记录的历史数据($\\{\rm{user\mbox{-}input}, \rm{LLM\mbox{-}response}\\} \times T$)
填充进 prompts 来让整个对话看起来有状态。
这块其实还是挺低效的（不知道 LLMs 内部是否有做缓存，按照以前的经验，在一句话解码时肯定是用了缓存的，
但这种模型之外连续增量调用的情况下，不知是否有缓存）。

[_repo]: https://github.com/hwchase17/langchain "LangChain Github repo"
[_parrot]: https://emojis.wiki/parrot/ "parrot emogis wiki"
[_notion_qa]: https://github.com/hwchase17/notion-qa "notion qa"
[_chat_langchain]: https://github.com/hwchase17/chat-langchain "chat langchain"
[_gpt_wolframalpha]: https://huggingface.co/spaces/JavaFXpert/Chat-GPT-LangChain "Chat-GPT-LangChain"
[_wolfram]: https://www.wolframalpha.com/ "Wolfram Alpha"
[_quickstart]: https://langchain.readthedocs.io/en/latest/getting_started/getting_started.html
[_react_paper]: https://arxiv.org/abs/2210.03629 