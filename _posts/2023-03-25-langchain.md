---
layout: post
title: LangChain 学习
date: 2023-03-25
categories: 技术 
tags: LangChain LLM
---
> 让 LLM 调用外部工具形成更强大的应用，这在最近成为一个趋势。从 LaMDA, ToolFormer 到最近火爆的 Visual ChatGPT,
我们看到了这个方向的价值和空间。简单看了下 Visual ChatGPT, 发现其使用了 LangChain 这个工具。
那我们就先来学习下这个工具吧。

## 什么是 LangChain

从 LangChain [代码库][_repo]的描述 *⚡ Building applications with LLMs through composability ⚡*，
我们可以快速 Get 到它是做什么的：

1. 一个利用 LLMs 做应用的开发工具
2. 关键方法是 *composability*——这里翻译成“组合”

LangChain 的 Logo 是 🦜️🔗， 这是两个 emoji 构成的。第一个彩色鸟样子的，查了下是 [parrot][_parrot], 鹦鹉，
应该是取鹦鹉可以说话这个点，表示这个工具通过自然语言作为交互入口；第二个 emoji 是 link, 直观表达了 Chain / 组合
的含义。不得不说，这个工具的名字和logo，还是挺简洁清晰的，是个好名字、好图标（这不会就是人类智慧的体现吧？）

在 README 里，作者说 LLMs 这种变革性的技术让开发者可以创造以前不可实现的应用，但是一个单独的语言模型，还是力有不逮，
给它加点其他来源的计算和知识，才能实现真正的强大。

而 LangChain 就是基于这个认知，着力于开发出用于帮助 LLMs 利用外部能力的工具。介绍里列举了一些应用示例：

1. 在特定文档上做问答, 如 notion-QA
2. 聊天机器人, 如 chat-clangchain
3. 代理工具(Agent)，如 GPT+WolframAlpha. ([WolframAlpha][_wolfram] 是一个在线问答系统)

从功能点的角度看， LangChain 的能力可以落到 6 个点上，由简单到复杂，依次为：

1. LLMs 和 Prompts: 包括对所有 LLMs 的通用接口封装，一些调用 LLMs 的公共能力；对 Prompts 的管理、优化能力
2. Chains: 具备将 LLMs 和其他工具串联的能力；对 chains 提供了标准接口，提供了一些和其他工具结合的中间层，
还对常用应用预制了端到端的chains.
3. 基于特定数据的生成(Data Argumented Generation): 构建了一个特定的 chain, 先从目标数据源里找数据，
再给 LLMs 做生成。比如指定长文摘要、特定数据下的问答。
   > 我们看到目前很多产品都是这个链路。比如 Paper 问答，甚至 new bing.
4. Agents: 所谓 Agent， 就是让 LLMs 作为代理，决定现在要做什么操作、执行操作、对操作结果做解析并基于此结果做后续操作。

   Agent 有很多种（面向不同场景，从后面的文档可知，有通用问答场景、数学计算场景等），
   LangChain 定义了标准的 Agent 接口，并提供了一些标准 Agent 供选择，还有一些端到端的 Agent 的例子。
5. 

[_repo]: https://github.com/hwchase17/langchain "LangChain Github repo"
[_parrot]: https://emojis.wiki/parrot/ "parrot emogis wiki"
[_wolfram]: https://www.wolframalpha.com/ "Wolfram Alpha"