# Isso

Using [isso](https://github.com/posativ/isso) as self-host comments server.

原始仓库的iss存在如下问题：

- 评论时，如果邮箱非法，isso server返回的是一个html，而在前端毫无反应…… （看起来像评论不成功）。 这个有issue，但没人修。

基于上面的原因，自己fork了一个，并fix掉了上面的问题。 这里部署的，就是自己修改的版本： [isso-fork](https://github.com/fseasy/isso)

## 部署方式

1. 基于 tmux 来运行服务； isso本身是一个WSGI服务，这里用 uwsgi 来运行。
2. 使用命令 `uwsgi ./uwsgi.ini` 来启动

   依赖的环境信息，见 uwsgi.ini 中的定义。 使用的是默认的 `python3.6`.

   注意的问题： uwsgi 用的是 `www` 账户，所以注意文件夹的权限（特别是要自己建立 `spooler` 文件夹并修改权限）

   - uwsgi 会自己建 spooler 文件夹，然而它用的是登录账户，而非 `www`, 导致启动后， uwsgi的子进程无权限。
     此bug已有人修，但似乎没有被merge.

   > 要想避免上面的坑，uwsgi 可以直接用 root 账户。只不过为了安全，还是用的 `www`.

   启动后，按照 `uwsgi.ini` 的配置，会在localhost启动一个服务，本地访问地址见配置项中的 `http`.

3. 使用 nginx 代理外部的请求
   
   在`/etc/nginx/nginx.conf` 中，在整个博客的server conf 中，增加了一段配置项，如下：

   ```conf
    # comments server
    location /isso {
        proxy_pass xxxx; # 上面的URL
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Script-Name /isso;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
   ```

如上，就完成了整个配置。

> 代码同步要避免将配置项放上来。之前把smtp的配置放上来，很快就收到了垃圾邮件，吓得赶紧关了邮箱的smtp……



