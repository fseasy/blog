# Isso

Using [isso](https://github.com/posativ/isso) as self-host comments server.

目前已知BUG： 评论时，如果邮箱非法，isso server返回的是一个html，而在前端毫无反应…… （看起来像评论不成功）。
这个有issue，但没人修。

## 部署方式

1. tmux
2. 使用命令 `uwsgi ./uwsgi.ini` 来启动

    依赖的环境信息，见 uwsgi.ini 中的定义。 使用的是默认的 `python3.6`.

    注意的问题： uwsgi 用的是 `www` 账户，所以注意文件夹的权限（特别是要自己建立 `spooler` 文件夹并修改权限）

    - uwsgi 会自己建 spooler 文件夹，然而它用的是登录账户，而非 `www`, 导致启动后， uwsgi的子进程无权限。
    此bug已有人修，但似乎没有被merge.