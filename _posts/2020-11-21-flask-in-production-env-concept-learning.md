---
layout: post
title: Flask 生产环境部署 概念学习
date: 2020-11-21
categories: 技术 
tags: Flask
toc: disable
---
> 前不久看到同事在周报里说的用 Gunicorn 让 Flask 应用效率更高，于是看了下 Flask 的生产环境部署，发现非常多的“新”名词，于是大概了解了一下。

请先阅读官方文档： [独立 WSGI 容器](https://dormousehole.readthedocs.io/en/latest/deploying/wsgi-standalone.html)

## WSGI 与 Flask

WSGI 全称  Web Server Gateway Interface, 可读作 Whiskey，是 Python 语言定义的、用于 Web 服务器 与 Web 应用程序 间交互的接口、协议。

基于上面的定义，我们可以知道：

1. 它适用于 Python 世界。根据维基百科，最早于 2003 年产生了第一个 Python 实现版本，随后 在 Ruby, JS, Lua 等有一定外溢影响。
2. 接口涉及到 2 方，分别是 Web 服务器 和 Web 应用程序。 
   
   `Web 服务器`，就是类似 Apache, nginx 等，处理来自浏览器的、基于 HTTP 协议的请求的；
   
   而 `Web 应用程序`， 应该可以等同于我们说的*网站*， 需要处理 Web 服务器解析后的 HTTP 请求，并返回给 Web 服务器处理结果，
   最终由 Web 服务器转为符合 HTTP 协议规定的响应，返回给浏览器。

之所以定义 WSGI 因为，是因为当时社区的各个开发者总是倾向于自己从头开始干，而不是重用、发展已有的项目，
导致社区里的 Web Framework 框架多而不强、极为混乱。受 Java 世界 servlet api 通用带来的方便性， [PEP 333][pep_333] 定义了 WSGI.

Flask 就是一个支持 WSGI 接口的 Web Framework. 所以，基于它构建的 Web 应用程序，可以方便地在各种支持 WSGI 的 Web 服务器上运行。


关于接口的具体定义、背景等更多信息，可以参考：

\[1\]: [WSGI-维基百科][wsgi_wiki]

\[2\]: [WSGI接口-廖雪峰][wsgi_liaoxuefeng], 可以简单清晰的大概了解 HTTP 、服务器、WSGI 接口 等内容

\[3\]: [Introducing WSGI: Python's Secret Web Weapon][secret_weapon]

[wsgi_wiki]: https://zh.wikipedia.org/wiki/Web%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%BD%91%E5%85%B3%E6%8E%A5%E5%8F%A3

[secret_weapon]: https://www.xml.com/pub/a/2006/09/27/introducing-wsgi-pythons-secret-web-weapon.html 

[pep_333]: https://www.python.org/dev/peps/pep-0333/

[wsgi_liaoxuefeng]: https://www.liaoxuefeng.com/wiki/1016959663602400/1017805733037760

## 支持 WSGI 的服务器

> Flask 自带一个 WSGI 服务器，使用的是 WerkZeug 提供的工具，仅适用于调试。  
[WerkZeug](https://werkzeug.palletsprojects.com/en/1.0.x/#getting-started) 是一个综合的 WSGI web application 
library. 注意它是一个 Library, 提供了写 WSGI 框架应用的一些实用工具/封装。它本身不是框架， 而是框架（如这里的 Flask ）的依赖。

按照文档，推荐的服务器有：

1. Gunicorn

   所谓的 Green Unicorn. 官方说法是 `一个 移植自 Ruby 的 Unicorn 项目的 pre-fork worker 模型。`
   `它既支持 eventlet ， 也支持 greenlet `.

   里面包含一些名词：

   **pre-fork模型**
      
   一种 Web 服务器运行方式。 其实就是预先 fork 出多个*进程*, 其中 1 个是主进程，其余是工作进程，
   主进程负责任务分发，工作进程负责实际处理。 
   
   特别地，对工作进程， Gunicorn 又有多种实现方式，其中包括 

   a. sync 方式，即阻塞式同步。
   
   b. gthread 方式， 用 Python 的 多线程方案。 即一个工作进程，启动多个线程，并行处理任务。因为 GIL 的限制，效率可能不是最优。

   c. 异步方式。 包括 `eventlet`, `greenlet`, `Gevent` 等方式。

   **eventlet**

   一个基于事件循环（ Linux 下是epoll， Mac 下是 kqueue, Windows 下是 libevent）实现的(网络)协程 (Coroutines) 库。

   内部实现了 GreenThread, GreenPool 以及 网络请求相关的协程包。 这个库，目标是 *Green the World...*

   > 底层的事件循环的库，有 libevent (最古老、广泛), libev（设计更简练、性能更好，但 Windows 上不好）, 
   libuv （开发 node 时重新写的，基于 libev + IOCP 解决 Windows 上的性能问题 => 不过从 Github 上的
   [仓库说明](https://github.com/libuv/libuv)，看起来并不依赖libev）,  
   libhv （国人写的，近期很多宣传…） 

   **greenlet**

   一个 C 写的、在 Python 中应用的轻量级协程库，被 Gevent 依赖。

   greenlet 是 stackless-python 的附产品。

   **Gevent**

   前面已经提到，Gevent 就是应用于 Python 上、在 greenlet 基础上构建的一套 high-level 的网络协程库。

   可以认为， eventlet 和 Gevent 是等价的东西。 可查看 [eventlet-vs-greenlet-vs-gevent][egg_vs]


   [egg_vs]: https://stackoverflow.com/questions/36834234/eventlet-vs-greenlet-vs-gevent

   参考链接： 

   \[1\]: [简单对比 Libevent、libev、libuv](https://developer.aliyun.com/article/611321)

2. uWSGI

   C 写的 Web 服务器，目标是建立一个创建主题服务的全栈工具！ 名称里 WSGI 是为了致敬该项目里第一个开发完成的插件——即支持 Python 
   WSGI 接口的插件。

   核心包含 配置、进程管理、sockets创建、监控、日志、共享内存区、ipc 等，似乎是个大轮子……

3. Gevent

   前面介绍 Gunicorn 时已经介绍了。也可以单独作为服务器。

4. Twisted Web

   是一个 HTTP server（可以被作为 lib 依赖，也可以独立跑）； 
   是一个 HTML 模板引擎；
   是一个 HTTP client 库。

综上，这些都是推荐的、用来在生产环境跑 Flask 应用服务的 Web 服务器。

## 前端搭配 代理服务器（nginx）

利用 Web 服务器 运行 Flask 应用后，如果流量依然很大，可以再在前端搭1个nginx服务器。

作用应该有2：

1. 负载均衡 （假设有后面有多个 Web 服务器实例）
2. 更快地处理静态文件（例如 js, css, img 等文件，并不需要将压力打到 Web 服务器上，而是直接利用“更”高性能、可靠的 nginx 做负载）

参考链接：

\[1\]: [HTTP请求是如何到应用程序的？-三级结构](https://juejin.cn/post/6844903863229612040#heading-1)