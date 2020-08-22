---
layout: post
title: Tensorflow Embedding操作
date: 2017-04-13
categories: 技术 
tags: Tensorflow
onewords: Tensorflow学习记录——Embedding.
---
> 包含Embedding的创建、初始化、PADDING向量归零、设置值等。

上次要写`Python多进程编程`，因为太晚决定第二天写，结果就搁置了快两个月了... 所以今天就得熬夜了. padding向量归零的部分在[爆栈](http://stackoverflow.com/questions/37255038/embedding-lookup-table-doesnt-mask-padding-value)和[issues](https://github.com/tensorflow/tensorflow/issues/2373)里都写了，这里再写一遍，ORZ，增加搜索索引量不容易啊，哈哈哈

## 创建、初始化Embedding Table

    # create and initialize
    with tf.variable_scope("Embedding"):
        # see http://stats.stackexchange.com/questions/47590/what-are-good-initial-weights-in-a-neural-network
        r = tf.sqrt(tf.cast(6 / EMBEDDING_DIM, dtype=DTYPE)) # => \sqrt( 6 / embedding_dim )
        lookup_table = tf.get_variable("lookup_table", shape=[WORDS_NUM, EMBEDDING_DIM],
                                       initializer=tf.random_uniform_initializer(
                                                   minval=-r, maxval=r),
                                       trainable=True)

如上，创建Embedding Table其实就是创建一个 `rank = 2` 的Tensor（即Matrix），现在普遍还是用`tf.get_variable`来得到，方便共享吧。需要注意的参数基本都列出来了。

特别地，`initializer`的值就是变量的初始化方式，也就是Embedding Table的初始化方式。一种是可以使用TF本身提供的一些初始化函数，包括`tf.random_unifrom_initializer`, `tf.truncated_normal_initializer`等；第二种就是使用固定值初始化。如果我们有预训练的Embedding，想要把这个预训练的导进来，可以直接使用这种方式。当然通过`initializer`方式指定初始化的Embedding，也就需要保证加载进来的预训练的Embedding和当前创建的Embedding变量维度相同。如果有不同的话，可以采用挨个赋值的方法。后面会介绍如何对变量某行赋值。最终，预训练的Embedding可以就是Python list, numpy ndarray等，也可以通过placeholder传。

## PADDING向量归零

用DNN batch地做NLP任务，必然跳不过padding。padding的方法这里不述（`tf.batch`, 这个我也没用过= =；或者自己拿原生Python做，上手简单）。

但是有个问题，padding之后得到索引，然后用

    tf.nn.embedding_lookup(lookup_table, id_input)

来得到Embedding输入，这里就有个问题——PADDING 对应的Embedding不再是0向量了。

不是0向量有什么问题吗？ 这个得看具体的任务、模型——如果是Seq2Seq，那么不是0向量显然影响最后的输出（如果input是反向输入）；如果是CNN模型，那似乎也是有问题的（卷积结果不好说）；如果是序列标注任务，其实通过传入`seq_len`, 并且处理好loss，应该是不会有问题的。

不过，**追求精确**应该是我们的目标！

关于这个问题的讨论，见[Embedding lookup table doesn't mask padding value](https://github.com/tensorflow/tensorflow/issues/2373), 可以看到Torch里有`LookupTableMaskZero`, comments里看到`Theano`也支持。 不得不说现在TF还是粗糙了点。

既然目前官方还没做，就只能自己处理了。说下解决方法，第一种，乘上一个mask矩阵！这个矩阵(二阶Tensor)把PADDING位置的值设为0，其余为1。我们同样在此Tensor上做`embedding_lookup`, 得到一个输入对应mask矩阵； 接着mask矩阵和原始的输入Embedding矩阵做element-wise乘积，就得到了归零化的输入Embedding结果。

    @ http://stackoverflow.com/questions/37255038/embedding-lookup-table-doesnt-mask-padding-value

    # build the raw mask array
    raw_mask_array = [[1.]] * PADDING_ID + [[0.]] + [[1.]] * (WORDS_NUM - PADDING_ID - 1)
    with tf.variable_scope("Embedding"):
        mask_padding_lookup_table = tf.get_variable("mask_padding_lookup_table",
                                                    initializer=raw_mask_array,
                                                    dtype=DTYPE,
                                                    trainable=False)

    id_input = [ [1, 2], [1, 0] ]

    embedding_input = tf.nn.embedding_lookup(lookup_table, id_input)
    mask_padding_input = tf.nn.embedding_lookup(mask_padding_lookup_table, id_input)
    # the mask-padding-zero embedding
    embedding_input = tf.multiply(embedding_input, mask_padding_input) # broadcast

第二种方式是自己想的暴力方式：

    # set the original embedding table padding embedding to zero
    # mask_padding_zero_op = tf.scatter_update(lookup_table, 
                                               PADDING_ID, 
                                               tf.zeros([EMBEDDING_DIM,], dtype=DTYPE))
    # explicitly replace value
    # lookup_table = mask_padding_zero_op

总结一下，第一种有计算开销，但是能保证网络不管怎么更新，Embedding输出的padding值都是0向量；第二种没有计算开销，但是需要注意不能再去更新padding位置的Embedding。这个通过在loss回传的时候做mask应该是可以搞定（还不确定）。

## 设置值

如上，使用`tf.scatter_update`就可以设置指定索引的Embedding Table值。`indices`可以是常量、list以及更高阶的输入。不过这里，如果我们只需更新一个，那么scalar就够，如果是指定的一批，那么就用list即可。

看API，有点不太明白[tf.scatter_update](https://www.tensorflow.org/api_docs/python/tf/scatter_update)和[tf.scatter_nd_update](https://www.tensorflow.org/api_docs/python/tf/scatter_nd_update)间的区别。网上也找不到...