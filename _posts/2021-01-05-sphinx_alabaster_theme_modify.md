---
layout: post
title: 使用 Sphinx 构建文档
date: 2021-01-05
categories: 技术 
tags: sphinx alabaster read-the-docs
---
> Sphinx + Read-The-Docs 部署文档； Sphinx alabaster theme的小优化记录。

最近又捡起好久以前弄的基于 Sphinx 的 [paper-touching](https://github.com/fseasy/paper-touching) 仓库。 

## 1. 使用 Read-The-Docs

最开始自己还在通过 Github 自带的 `/docs` 方法来部署生成网页。这种方式，我需要本地运行 Sphinx 的 `make html`,
然后把build的结果移动到 `/docs` 目录下，再 push 到 Github上。比较繁琐，增加了编译后的改动提交。

然后想起很早前从别人那里了解到的 [Read-The-Docs](https://readthedocs.org/), 想着这个应该可以支持这种场景吧。
简单一尝试，轻松就部署了，实在是强大！不得不感慨，开源世界真的厉害！

另外对 Read-The-Docs 的盈利方式感兴趣。从官方的[文档](https://docs.readthedocs.io/en/stable/advertising/index.html)来看，
主要就是 Ads (Ethical Advertising)， 
赞助商（目前是 MS Azure, cloudflare, and you?, haha）, 
以及商业使用(Read the docs for Business.)

## 2. Sphinx alabaster 小优化：宽度，对齐方式，侧边栏目录

Sphinx 可以指定主题，我选择的是 `alabaster`（对应的仓库是 [alabaster](https://github.com/bitprophet/alabaster)）.

默认的主题主要有 3 个小问题：

1. 整体宽度太窄：侧边栏窄，导致目录显示要换行，不好看；内容域太窄，排版不好；
2. 段落字体对齐似乎是左对齐，导致右边对不上，很影响美观
3. 侧边目录展示的是全局目录（就是整个站点的顶级目录），这在我的项目里不太合适——只希望展示当前页面的目录

search 一下， 找到了答案。

首先，针对问题1、2，我们用alabaster 文档 [customization](https://alabaster.readthedocs.io/en/latest/customization.html)
里的 `Custom stylesheet` 方法来解决。

先定义一个 `custom.css`, 放到 `_static` 里面就好了。然后

1. 拿浏览器的 F12 工具，定位到想调整的元素上，获得 stylesheet 类名
2. 再去 [/alabaster/static/alabaster.css_t](https://github.com/bitprophet/alabaster/blob/master/alabaster/static/alabaster.css_t)
 里面找到对应的类定义，复制到我们的 `custom.css` 里
3. 调整css的内容为我们期望的值即可

具体的，对上面的需求，

1. 宽度，主要是 `div.document`, `div.footer`, `div.sphinxsidebar` 这些
   
   注意把 side bar 的 `position: fixed;` 复制上，不然侧边栏不会滚动。

2. 对齐，主要是加上 `text-align: center;`

对于问题 3，我在 stack-overflow 上找到了答案，这里找不到具体的页面了，在 sphinx 项目的 `conf.py` 该做如下改动即可：

    html_sidebars = { 
        '**': ['localtoc.html', 'searchbox.html'] 
    }

其中的 `localtoc.html` 就是当前页的目录了，相应的有个 `globaltoc.html` 就是全局的目录。此外，还有第三方的插件做 fulltoc 的，
就是把全局和局部合起来，这个自己没有尝试。

此外，我还调整了下 `font-family` 和 `<p>` 下的行间距。当然，整体上还是随便调了调，毕竟一调到凌晨2点，一调中午就没了……

最后，附上调完的css[链接](https://github.com/fseasy/paper-touching/blob/main/source/_static/custom.css) .

-> 显示不是关键，关键是内容啊。自己总是忘了……
