---
layout: post
title: LightGCN 代码
date: 2023-05-17
categories: 技术
tags: lightgcn
---

之前已经写了一个博客了，这里再记一下其他内容。

直接看论文中提及的代码： https://github.com/gusye1234/LightGCN-PyTorch

（1）每一次训练的输入：

* 使用的是 BPRloss，每一个样本要有一个正例、一个负例。

注意这里的正例负例形式： (user, positive_item, negative_item)

可以看到： 正例是 user-positive_item，负例是 user-negative_item ，也就是两条 u-i 边！

可以推测这个是图上的一个无监督的 loss；

图上只有 U-I 边，所以loss 也是基于 U-I 边的； 

正例就是用户有交互的样本（也是随机采样的），负例是 随机采样的，且保证不在正例中；

* 一次训练，要有 $TOTAL_INTERACTION 个样本
  * 也就是共有 TOTAL_INTERACTION 个循环，
  * 每个循环，随机选1个 user，从交互行为中随机1个正样本出来
  * 从不存在的边里，随机1个负样本出来

（2）sparse-graph

* 构建的 Graph，行、列的 size 都成为了 n_user + n_item

  * 同时把 U x I, IxU 部分都赋值
  * 对 UxI, IxU 部分都做 norm，norm 的因子是  sum(eges) ** (-0.5)
