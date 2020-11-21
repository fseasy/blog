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

   一个基于事件循环（ Linux 下是epoll， Mac 下是 kqueue, Windows 下是 libevent）实现的协程库。

   这个库，目标是 *Green the World...*, 可怕。

   ****