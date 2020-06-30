---
layout: mathpage
title: Batch-Normalization 和 Layer Normalization
date: 2020-03-09
categories: 技术 
tags: Batch-Normalization Layer-Normalization
onewords: BN, LN要点理解。
---
> BN和LN是DL中相当重要的技术，这里对其背景（要解决的问题）、方法、作用做一个总结记录。

Batch-Normalization是2015年就有的技术，到如今已经算是标准的结构了，在视觉领域应用广泛；LN是2016年提出的，
是对BN的一种改进，主要用在NLP领域。

下面依次对这两个基础计数做介绍，文章仅是总结关键信息，自己并无经验分享。

## Batch Normalization


### 背景（要解决的问题）

