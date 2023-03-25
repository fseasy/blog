---
layout: post
title: LangChain 学习
date: 2023-03-25
categories: 技术 
tags: LangChain LLM
---
> 让 LLM(Large Language Model) 调用外部工具形成更强大的应用，这在最近成了一个趋势。
从 LaMDA, ToolFormer 到 Visual ChatGPT 再到引轰动的 ChatGPT plugins,
我们看到了这个方向的价值和空间。最近自己也在想如何用 LLMs 来驱动已有的工具，但没有想好一个妥善的实现方式。
基于此，我先调研了下 Visual ChatGPT, 发现其使用了 LangChain 这个工具。
它有开源代码且独立于 LLMs， 读了 README, 感觉它很符合自己的期望啊。那就深度优先，先来学习下这个工具吧。

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

1. LLMs: 构建一个 LLM 的实例并获取其返回结果。这个比较简单，可以认为就是已有 API 的封装。
   但这个实例是后续步骤的基础(被称为是 *one primitive*)。
2. Prompts 模板：实现了一个提示模板类。似乎就是 `Python str.format` 的一个封装，但这同样是基础模块。
3. Chains: 给的例子是预先定义的 `LLMChain`：
   
   用 `chain = LLMChain(llm=llm, prompt=prompt)` 实例化后，
   直接`chain(prompt_input_var)` 调用就可以完成 1. 提示模板填充 2. 请求 LLM 并返回 这个两个步骤。

   看到这里觉得还 OK， 只是工程上的封装，还没有体现出 LLMs 驱动工具的功能；
   不过 Chain 这个点已经看出来了——是要通过定义类来实现的。


[_repo]: https://github.com/hwchase17/langchain "LangChain Github repo"
[_parrot]: https://emojis.wiki/parrot/ "parrot emogis wiki"
[_notion_qa]: https://github.com/hwchase17/notion-qa "notion qa"
[_chat_langchain]: https://github.com/hwchase17/chat-langchain "chat langchain"
[_gpt_wolframalpha]: https://huggingface.co/spaces/JavaFXpert/Chat-GPT-LangChain "Chat-GPT-LangChain"
[_wolfram]: https://www.wolframalpha.com/ "Wolfram Alpha"
[_quickstart]: https://langchain.readthedocs.io/en/latest/getting_started/getting_started.html