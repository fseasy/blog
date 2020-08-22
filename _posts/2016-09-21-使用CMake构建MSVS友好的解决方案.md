---
layout: post
title: 使用CMake构建MSVS友好的解决方案
date: 2016-09-29
categories: 技术 
tags: CMAKE VS
onewords: 使用CMake构建出VS友好的项目。
---
> 在MSVC下，通过开启、设置一些变量，并避免一些可能导致未知错误的行为，我们可以构建出更有组织的MSVS解决方案。

![MSVC-SOLUTION-gouped](/assets/img/20160929/msvs_solution.png)

这个基本上都是从各种零散的地方（爆栈、一篇博客）搜到的，然后先急着用了，没有记REFERENCE。这里算是做一个总结和记录吧。

1. 文件夹名一定不要相同。

    即是`p/a/same-name`、`p/b/same-name`和`p/c/d/same-name`这样的情况应该避免。这将导致VS（或者CMake?）在显示（构建）以下功能时出现问题，就是不会成功啦。

2. 在根`CMakeLists.txt`下开启`显示目录`选项。
    
    就是以下命令：

        # dispaly folder in MSVS
        SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

    这个是必须的，否者就不会显示目录结构。

3. 手动组织目录

    如果想要构建一个好的项目解决方案，那么非常有必要手工组织代码的结构。

    这里我还没有弄得非常清楚，只能根据之前的尝试给出可用的方案：

    **对于有编译目标的单元**（即有`ADD_EXECUTABLE`或者`ADD_LIBRARY`），那么我们通过一下命令可以把这个编译单元放到某个目录下：

        SET_PROPERTY(TARGET trivial PROPERTY FOLDER "libraries")      

    如上，`SET_PPRPERTY`第一个参数`TARGET`是固定值,第二个`trivial`就是编译单元的名称，第三个`PROPERTY`，第四个`FOLDER`都是固定值，第五个`libiraries`是要放到的目录下。目录支持层级结构，这里用`/`来划分层级结构，如`libraries/cnn`。

    **对于无编译目标的单元**（比如按功能区分的头文件），使用以下命令来使得其在MSVS显示中能够分组：

        SOURCE_GROUP("lookup_table" FILES ${lookup_table_headers}
                                          ${lookup_table_srcs}


    `SOURCE_GROUP`第一个参数同样是目录路径，路径可以是层级的，用`\\`双反斜杠表示，如`trivial\\lookup_table`就是在`trivial`文件夹下有`lookup_table`；第二个参数是`FILES`固定值，后面的参数就是具体的`FILES`路径（文件）。

    经过测试，发现对于有编译目标的单元，使用`SOURCE_GROUP`没有作用，实际在MSVS显示时都是以编译目标中的*依赖项*分组为头文件和源文件的。但是对于没有编译目标的文件，可以用这个方式来组织。因为如果我们不组织的话，那么显示的时候就是flat的结构，在文件系统中的目录层级就消失了。我们可以使用以下命令来方便的获取一个路径下的所有文件：

        FILE(GLOB lookup_table_headers "lookup_table/*.h*")

    `FILE`下第一个`GLOB`表示使用通配符（？），然后第二个参数`lookup_table_headers`用来存储结果的变量名，第三个参数（还可以后续接任意个）就是通配路径了，应该是支持`*`,`[]`之类的，具体需要查一下吧。
