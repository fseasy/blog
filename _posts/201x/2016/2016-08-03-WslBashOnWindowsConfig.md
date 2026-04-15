---
layout: post
title: Windows Subsystem for Linux(WSL)的配置
date: 2016-08-03
categories: 技术 
tags: 折腾 WSL
onewords: Win10周年更新正式带来WSL。
---
> 经过一下午+一晚上的折腾，终于比较圆满地配置好了WSL。

## WSL是什么

`Windows Subsystem for Linux(WSL)` 是Win10周年更新正式面向开发者发布的功能。通过WSL，我们可以在Windows上编译、运行Linux上的二进制代码。注意，它不是套了一个虚拟机，而是通过拦截Linux程序的内核调用，用NT内核对应的系统接口完成系统调用来达到近乎无性能损失地良好兼容。(说错了不要打我.)

WSL也被叫做 `Bash on Windows`, Github上有一个可以提Issues的[项目](https://github.com/Microsoft/BashOnWindows).

查看[维基百科](https://zh.wikipedia.org/wiki/Windows_Subsystem_for_Linux)，有更加专业地定义。

查看[官方FAQ](https://msdn.microsoft.com/en-us/commandline/wsl/faq).

## WSL的优势

不再需要笨重的虚拟机，同时有更加良好的性能。因为Linux程序直接运行在Windows上，拥有与普通Windows程序相同的物理资源。与Windows本身的文件交互更加简单，因为Windows的文件系统直接挂载在`mnt`下，不用依靠虚拟机的挂载（不过，中文名还是会乱码，因为WSL的默认编码依然是UTF-8）。

## WSL的不足

`bash.exe`依然是控制台程序——即所有的Linux命令都显示在Windows控制台窗口下。这，不算很美观。

虽然Windows10周年更新已经完善了cmd的显示能力——终于支持全部字体了！但是似乎总感觉有些缺陷，如不能关闭控制台下按键出错的声音，不能多开Tab，控制台内容在窗口变化时可能不能正确重新布局等。

以及，一些Linux程序仍然不能再WSL上运行。官方文档上说，目前还不能运行GUI程序，然后民间以及说可以跑GUI了——[FYI you can run GUI Linux apps from bash](https://www.reddit.com/r/Windows10/comments/4ea4w4/fyi_you_can_run_gui_linux_apps_from_bash/)。 此外，某些系统调用从Linux转向Windows可能还没有完成，甚至因为二者内核设计的差异，可能最终都不能支持。这会导致某些依赖这些功能的程序不能很好的工作。比如，在实际中我发现目前ping还不能工作，因为icmp包似乎还没有被支持，当然这应该不是什么大问题。

最后，可能踩到坑。要知道Ubuntu已经够多坑了，WSL作为新的一个系统形式，可能会有很多潜在的问题。不过，使用WSL是为了什么呢？当然只是为了开发方便了。而在开发上，现在看来G++、GCC都没有什么问题，应该影响不大。

## WSL怎么开启

直接看[官方教程](https://msdn.microsoft.com/en-us/commandline/wsl/install_guide)就好，图文并茂。

## WSL如何配置（使用默认CMD方式）

直接 `win+Q`, 输入`bash`，打开就进入到bash了。你可以认为这就可以了。

当然，我们可以改下字体——从网上下载一个`YaHei Consolas Hybird`， 在设置中选择该字体，把字号调大些，看起来就已经很舒服了。调透明度什么的就随意了。

接下来说怎么消除按键无效时嗡嗡声（由于CMD没有选项去关闭bell，所以只有去修改软件的设置）：

1. 去掉在shell状态下的嗡嗡声：

        vim ~/.inputrc
        应该是个空文件，输入
        set bell-style none

2. 去掉vim状态下的嗡嗡声：

        vim ~/.vimrc
        加上
        noeb vb t_vb=
    
    做个广告，我的轻量级vim配置,[即下即用](https://github.com/fseasy/VIM-plugin-and-config)。

上面配置后，大部分情况下就不响了。

因为总觉得CMD不是很舒服（无标签，心理暗示），从网上搜索下了一个`ConsoleZ`，折腾了一下，发现与CMD相比没有什么实质进步。选择放弃。

最后，我们可以给bash建立一个快捷方式，使得每次进如bash时都在一个确定的目录（比如`~`下），同时还可以美化图标。方法很简单，就是在`C:/Windows/System32`下找到`bash.exe`， 然后右键建立快捷方式，然后再在快捷方式上右键，设置其中的`目标`，如我的： `C:\Windows\System32\bash.exe ~`, 然后再更改图标，可以从内置icon中选一个，也可以自定义。最后，我们把快捷方式固定到开始菜单上吧。

## WSL如何配置（使用SSHD+XShell方式）

以前用虚拟机时，我就喜欢后台跑虚拟机，然后用XShell通过ssh登陆到虚拟机。这种方式可以保证终端输出是非常漂亮的，因为XShell5非常漂亮实用，XTerm+透明+字体+快捷复制粘贴+多Tab（其实相比目前cmd，就配色和多Tab上有优势）；而且虚拟机到后台，只跑一个ssh-server，没有UI绘制也不卡。

现在抛弃虚拟机，我决定还是尝试一下用这种方式来配置。

过程中遇到了一些问题，基本都是通过搜issues解决的。以下直接贴出完整流程，并附上来源。

1. WSL上安装ssh-server

    直接apt-get:

        sudo apt-get install openssh-server openssh-client

    在apt-get前，请修改默认源（完全没速度），使用Ubuntu[官方Wiki](http://wiki.ubuntu.org.cn/模板:14.04source#.E6.95.99.E8.82.B2.E7.BD.91)上的最好（不要使用163的，GCC、G++安装会有问题）. 注意WSL的Ubuntu是14.04LTS.

        sudo vim /etc/apt/sources.list # 修改源
        sudo apt-get update
    
    OK,上述应该没什么问题. 装完后也装上了ssh-keygen工具。

    参考: [Ubuntu环境下SSH的安装及使用](http://blog.csdn.net/netwalk/article/details/12952051)

2. 为sshd工具生成认证key

    多次试错，只需要一个命令：

        sudo /usr/bin/ssh-keygen -A

    自动会在正确位置生成所有需要的key。
    
    参考：[SSH: Could not load host key: /etc/ssh/ssh_host_rsa_key](https://bbs.archlinux.org/viewtopic.php?id=165382)

3. 修改ssh-server配置

    这个很关键，不然连不上。

        sudo vim /etc/ssh/sshd_config

    把其中3条配置改变一下：

        ListenAddress 0.0.0.0  #去掉原配置的注释即可；监听本地回环
        UsePrivilegeSeparation no # yes->no；不知道为什么，不改连接会理解关闭
        PasswordAuthentication yes # no->yes；不改只能使用秘钥登陆

    然后最好改下配置中的Port，

        Port XXXX # 默认的22可能需要特殊权限（Windows系统上），没有做过测试。

    参考： [any status update on ssh server?](https://github.com/Microsoft/BashOnWindows/issues/300)

4. 启动ssh-server

    一条命令：

        sudo /usr/sbin/sshd # 其实我做这步是用的是： sudo /etc/init.d/ssh start

    启动之后，保持窗口开启，我们使用XShell来登陆。

5. 测试能否建立连接

    配置XShell的用户名、密码(开启WSL时的设置)，服务器ip（127.0.0.1）、端口(与配置文件一致)。

    如果上面没有跳过的话，应该可以顺利连接成功。

6. 使其后台运行

    WSL上，如果所有bash窗口都关闭的话，相当于Linux系统关机了。所有的后台守护进程都完蛋了。
    
    我们有必要使用Windows上的工具，来使得其在后台运行一个bash，且此bash里面开启了sshd进程。

    这里有点复杂了！

    有两点要求： 

    1. Windows上后台运行bash

    2. bash里运行sshd进程

    首先从2开始：

    `bash.exe -c "XXXX"` 以上命令就可以打开bash，并且理解运行命令`XXXX`，但是跟我们的需求有不满足的地方：首先，我们sshd是需要sudo的！而上述命令没法输入sudo的密码！其次，如果命令理解运行结束或者跳到后台，那么bash理解就结束了，然后因为没有bash存在，所有后台程序也就挂了。

    所以，针对此问题，我们也要解决两点，首先还是解决第二点——不让sshd后台。很简单，使用`-D`参数即可。
    故我们的启动命令变成了：

        `sudo /usr/sbin/sshd -D

    接着说如何直接以管理员运行sshd而不输入密码：

    有两种方式，任选一种

    第一种： 将WSL默认登录用户设为root，自然拥有root权限。相关见[Command Reference](https://msdn.microsoft.com/en-us/commandline/wsl/reference). 在**`cmd`**里输入如下命令：

        lxrun /setdefaultuser root

    因为root默认存在，也不需要密码切换，所以可以立即生效。注意，这么切换后以后登录都默认使用root了，切换回来再使用这个命令就好了。如果接受一直使用root用户的话，可以选这种。否则选下面吧。

    第二种： 利用`sudoers`使得开启进程的sudo命令`sudo /usr/sbin/sshd -D`可以被任何用户执行而不需要输入密码。

        #在bash中输入：
        sudo visudo -f /etc/sudoers.d/sshd
        # 然后键入如下命令：
        %sudo ALL=(ALL) NOPASSWD: /usr/sbin/sshd -D
        # 按Ctrl+X退出，会提示保存，一定要修改保存的文件名，把 `sshd.tmp`中的`.tmp`删去
        # 因为按照README中的内容，如果有`.`，那么这个文件就无效
        # 关于为何？我也不算很懂。反正上述一定要保证没有错误。
        # 因为这个文件一搞错，sudo就不能使了。sudo不能使了，那么你就改不了这个文件了！这不就
        # 成一个环了？我当时就懵了，不知道该怎么办——直到我找到了第一种方法。对，
        # 直接以root登陆就好了。
        # 总之，个人还是倾向第二种吧，因为以后来说更加安全。
        # 做完之后，可以新开终端（防止记住root密码），以非root身份输入 `sudo /usr/sbin/sshd -D`, 如果不问密码，并且成等待输入的状态——OK，完成了。

    完成bash命令启动后，如何让其在Windows后台运行？

    不废话了（要过12点了），直接在一个你喜欢的位置，写两个文件（同一目录下）：

        # 文件1 ： startsshd.bat
        # 内容是：
        cd C:\Windows\System32
        bash.exe -c "sudo /usr/sbin/sshd -D"

        # 文件2 : runinbackground.vbe (其实文件名随意啦，只是后面要用到罢了)
        # 内容是：

        dirPath = createobject("Scripting.FileSystemObject").GetFile(Wscript.ScriptFullName).ParentFolder.Path
        shellPath = dirPath & "\" &"startsshd.bat"
        ' wscript.echo shellPath
        set ws=wscript.createobject("wscript.shell")
        ws.run shellPath & " /start",0

    写完，直接点 `runinbackground.vbe`, 如果不保错，且此时在你看不到任何打开的Bash的情况下XShell可以连上，那么恭喜，真的没有问题了！如果报文件找不到，那么肯定是文件名命名与脚本不一致，或者复制时有特殊（不可见）字符！请手打或者plain模式（查看HTML源码）复制。

    参考： [How to run sshd as a windows service ?](https://github.com/Microsoft/BashOnWindows/issues/612)

7. 使其在用户登陆时自动运行

    这就要用到Windows的计划任务功能了。

    `Win+Q` , 然后输入`taskschd.msc`. 点开就是一个设置计划任务的程序。

    点击菜单栏的操作，选择`创建基本任务`，

    1. 第一步，填任务名称和说明

    2. 触发器选 `当用户登录时`，当然其他也可以，随意。

    3. 操作，选 `启动程序`

    4. 通过`浏览`,定位到刚刚的`runinbackground.vbe`,

    5. 完成。

    如果有更多需求，就找到刚刚建立的任务，然后选择属性，进行配置就好。比如可以配置禁用只有交流电源时才启用任务、警用任务运行3天后自动关闭等。

    注意属性里的`隐藏`，应该不是指`隐藏窗口`，因为试过没有用...

    参考： [启动任务计划程序](https://technet.microsoft.com/zh-cn/library/cc721931(v=ws.11).aspx) , [How to run sshd as a windows service ?](https://github.com/Microsoft/BashOnWindows/issues/612)

最后，终于完成了。觉得学到了很多，当然，也可以认为完全是瞎折腾啊... 

开心就好。











