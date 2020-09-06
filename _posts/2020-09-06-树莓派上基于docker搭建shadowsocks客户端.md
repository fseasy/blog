---
layout: post
title: 树莓派上基于docker搭建shadowsocks客户端
date: 2020-09-06
categories: 经验
tags: 树莓派 shadowsocks
---
> 偶尔访问github困难，需要在树莓派上搭建一个ssclient，发现有用树莓派搭建的，但是没有具体记录，这里把过程记录一下。

1. 安装docker

    参见 [树莓派上使用脚本安装docker](https://yeasy.gitbook.io/docker_practice/install/raspberry-pi#shi-yong-jiao-ben-zi-dong-an-zhuang)即可。

2. 安装docker-compose

	官方不支持树莓派

	不过有python的package可以装

    ```bash
	pip3 install docker-compose
    ```

3. 安装 shadowsocks-libev-arm docker, 它有client-arm的功能
	
    从 [docker-hub:shadowsocks-libev-arm](https://hub.docker.com/r/easypi/shadowsocks-libev-arm) 中，
    拷贝 `docker-compose.yml`(
    即[link](https://github.com/EasyPi/docker-shadowsocks-libev/raw/master/docker-compose.yml))
	
    参考其中的示例，修改 `client-arm` 中的环境变量值为具体的ss-server的信息。然后使用

    ```bash
    docker-compose up -d client-arm
    ```

    启动docker.

4. 需要将socks5 转为 http 的代理，方便终端使用

	安装privoxy

	curl --proxy http://127.0.0.1:8000 https://www.google.com

	https_proxy=0.0.0.0:8000 curl https://www.google.com

5. 开机启动

	vim /etc/rc.local

## 主要参考链接
1. [树莓派安装docker](https://yeasy.gitbook.io/docker_practice/install/raspberry-pi#shi-yong-jiao-ben-zi-dong-an-zhuang)
2. [docker安装ss](http://tiven.wang/articles/setup-shadowsocks-proxy-using-docker/)
