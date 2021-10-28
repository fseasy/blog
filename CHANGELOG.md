# 更新

## 2021.10.28

1. 启动域名：前段时间弄了个fseasy.top的域名，今天把相关改动全部做完：
   - 停止 github.io 的 page （防止重复索引）
   - 修改 gcse, google-analystic 等地址
2. 保持干净：合并了部分Tab页面；同时将代码的文件夹也做了归并；css中去除目前已经无意义的sinabloga/b的嵌套
3. 扩大页面宽度：min-width > 1400px 时，页面size 改为 1280 （看起来不太习惯；但是打开侧边目录时更好一点吧）

## 2021.01.23

1. 修复了索引页 `description` 设置错误的 BUG；优化索引页的 title 显示（Tab名字 + site名字）；
2. 优化 /list 的 categories 排序，现在按数量排序；优化 /list 下日期显示，现在不会把空格删除了；删除了「作品」这个 category 
3. 修改 site title 为 「鱼虾写字」；更新 abount-me
4. 删除 `_includes`, `_layouts` 中标记为 deprecated 的文件

## 2020.10.11

1. 改变ul/ol下的list的缩进

    看这个 https://developer.mozilla.org/zh-CN/docs/Web/Guide/CSS/Consistent_list_indentation

    为了兼容，需要同时设置margin-left, padding-left，其中一个生效即可
    距离默认都是40px；

    我们需要减少这个距离，就需要改成em的相对距离! 设置为1.3em.

    > 后来发现，这个间距最后是否显示OK，跟使用的字体息息相关……

2. 改变字体

    标题用黑体；正文用微软雅黑；引用用楷体（但是显示太差了）。

    > 发现Windows下渲染的确太差了，太粗糙。

3. 改变一些间距

    标题间距，li间距


## 2020.06.20 - 2020.09.13

本来打算增加一个目录的，结果发现mathpage layout里的东西都过时了（还在使用jiathis, duoshuo...），
瞬间觉得之前写得还是挺烂的……

**预期功能：**

1. 文章显示目录 [done]
2. 分享功能 [done]
3. 把TAG页独立为tab页；将“目录”改为“分类”，同时增加搜索（谷歌搜索）能力
4. meta元素显示优化； 引用显示优化； 代码显示优化； 
5. 移动设备显示优化： 头部显示，底部显示； [done]

参考博客： https://github.com/bit-ranger/blog

**实现细节：**

1. 通过搜索，找到了 [share.js](https://github.com/overtrue/share.js/), 挺漂亮的，而且定制化也很强
    
    - 发现只有通过`data-xxx`才能配置成功，通过js里配置不成功
    - 微信分享有问题，变成一个文件…… => 发现原来这个是本地测试的原因——放到github上就没有问题了——因为localhost微信打不开啊 = =

    因为share.js的CDN下载可能存在问题，导致目录的价值也延缓了…… 应该把目录加载放到这个之前！

2. 把share.js、disqus、mathjax、google-code-prettify 全给放到独立的html里，通过include引入，好了太多
3. 忘了当时代码高亮的逻辑了：config里使用了rouge、又用了google-code-prettify，不知道哪个起效；而且style不好看

    - 明确了： rouge是起效了的，会生成`highlighter-rouge`的div, 但是高亮效果不符合预期： 
        我是期望直接四个空格就高亮代码，但是`rouge`需要你用`liquid`的格式写出这个代码是什么语言，才能正确高亮；
        否则就只有一个边框，里面就是个白底黑字。

    - google-code-prettify可以很好自动高亮，但是需要`css class==prettyprint`的元素才起效；
        于是拿js处理了下rouge的结果，给`<pre>`tag增加了code-prettify需要的class，这样就可以直接高亮了

    - 修改google-code-prettify的样式，这个 
        [demo](https://raw.githack.com/google/code-prettify/master/styles/index.html)
      可以查看样式。其中的 `li.L0,li.L1...`就是定义每行样式的。之前就是把除第4的元素设为一样，
      第4行设为其他背景，这样就出现了每5行背景颜色不同的效果——然而我并不喜欢这个效果……
      只需要把这块的特殊处理注释掉，就可以保持每行的背景是一样的了。

4. 在虚拟机，WSL，树莓派 Model 4B 三个环境上测试jekyll全新生成的时间！

    之前一直在虚拟机上做本机测试，觉得jekyll generating实在太慢了，闲来无事，想benchmark一下。
    以当前仓库的全新generating的速度（耗时）为评估值； 
    附属效果： 构建一套与Github Pages官方一致的构建环境，同时找到时间最优的构建机器。

    **构建环境信息：**

    1. Github上构建本地测试环境的说明: 
        [testing-locally](https://help.github.com/en/github/working-with-github-pages/testing-your-github-pages-site-locally-with-jekyll)

    2. Github的依赖环境： [Dependency versions](https://pages.github.com/versions/)

    **安装环境：**

    - 虚拟机：
    
        1. 原来有jekyll环境，是4.0的； 这里先卸载掉！

            ```shell
            gem uninstall jekyll # 或者按提示
            ```

        2. 需要先装ruby, jekyll要求 2.50及以上； 虚拟机上是 2.51, 已经OK

            [使用清华Ruhy Gems源](https://mirrors.tuna.tsinghua.edu.cn/help/rubygems/)

            设置ruby本地目录：

            ```shell
            mkdir ~/.ruby
            echo 'export GEM_HOME=$HOME/.ruby/' >> ~/.bashrc
            echo 'export PATH="$PATH:$HOME/.ruby/bin"' >> ~/.bashrc
            source ~/.bashrc
            ```

        3. 安装 bundler. 

            ```shell
            gem install -V bundler
            ```
        
        4. 安装 `github-pages` gem，这个会构建jekyll等相关依赖环境！

            写一个Gemfile 文件，内容

            ```gem
            # https://jekyllrb.com/docs/github-pages/
            source "https://mirrors.tuna.tsinghua.edu.cn/rubygems/"
            gem "github-pages", group: :jekyll_plugins
            ```

            使用以下命令安装依赖（包括jekyll等等）

            ```shell
            bundle install
            ```

    - WSL: 这个与虚拟机类似；只是因为WSL是ubuntu 14.04, ruby版本只有2.4，需要再装下ruby；

        [安装 rbenv](https://gorails.com/setup/ubuntu/14.04)

        使用 `git clone https://github.com/andorchen/rbenv-china-mirror.git ~/.rbenv/plugins/rbenv-china-mirror`
        替换ruby源；

    - 树莓派 Model 4B: 

        ruby是2.5.5的，可以跟虚拟机一样装；

        需要先安装 `sudo apt install ruby-dev`, 不然编译安装依赖的时候找不到头文件。


    **测试：**

    执行 `bundle exec jekyll serve`

    | Env            | Time(s) | 备注 |
    |----------------|-------|--------|
    |虚拟机           | 42  | 耗时比较稳定  |
    |WSL             | 24  | 第一次花了31s，后面基本是21 - 24s |
    |树莓派 Model 4B  | 18  | 耗时很稳定  |

    **结论：**

    虚拟机性能最差！WSL性能不太稳定；树莓派最好！尴尬，我的y470真的不行了…… 幸好最近不用写什么大程序。
    




5. 目录功能 & 对其做修改

    使用 [jekyll-table-of-content](https://github.com/ghiculescu/jekyll-table-of-contents).
    
    这里称其为 toc.js. 
    需要对其做一些改动！
    
    1. 我们用的是 bootstrap v3，而toc.js 用的是 v2, 所以点击回到顶部的图标无法显示 => 【放弃；意义不大，不使用此功能】

        ```html
        <!-- 将这些放到 header 的元素下-->
        <button class="btn btn-link btn-sm toc-totop-btn back-to-top">
            <span class="glyphicon glyphicon-menu-up"></span>
        </button>

        <!-- self css -->
        .toc-totop-btn {
            margin-bottom: -5px;
            margin-left: -15px;
            color: grey;
        }
        ```
    
    2. 使用 boostrap js里的 scroll-spy功能（滚动跟踪）来实现侧边目录的动态对应，使用affix来实现侧边目录sticky功能 【done】

        遇到些问题：
        
        a. bootstrap的样式必须设置好(`nav nav-tabs` 或者其他)，开始只设置了`nav`，怎么都不行……
        b. 中文header报错 —— 因为 toc.js 里面创建link的时候做了 `EncodeURIComponent`， 中文
            被encode了，这对浏览器而言是ok的；然而这个bootstrap就不行了——它应该就是直接字面匹配的；
            所以，修改了toc.js，把encode给去掉了——或许会导致锚失效？不管了……
        c. affix和scroll-spy是在页面loaded的时候执行的；而引入的Dsiqus插件在无代理的情况下总是下载失败，到时长时间页面难以loaded.
           这导致上述功能长时间无法execute. 解决方法一种是让这两个功能立刻执行，然而这不太可能——要改bootstrap.js. 另一种就是
           让disqus插件慢点执行——或者说在loaded之后再执行。显然第二种更简单——因为只需要在disqus插件上包装一下就好咯。
           想起之前用来模仿的页面，加载disqus是要点击一个按钮再加载的，想想这样挺合理的。于是就让disqus在点击按钮后才加载。
           这样挺好的。 
        d. affix不符合预期；照理应该基于其父元素(content)来决定top的offset（初始应该是0），然而却发现是基于body的，top是
           header的高度，这样就不对了。【done】

           尝试用css3的 sticky 来实现，发现这更是一个大坑…… 关键是要找sticky的跟踪
           对象，但是这个对象是隐式推导的，似乎还只有`overflow`这个属性来控制，
           而且似乎这个属性
           要考虑 parent-path 上的所有 overflow 属性。我们的side-nav太下层了，
           不知道是不是parent设置有问题，导致sticky不work.
           我担心之前css对overflow有修改，
           于是把所有的css都去掉了，在纯粹的html上尝试sticky, 但行为依然是不符合预期的：
           放在有的层级下就可以，放在原来的位置就不行. 实在难受，试了一下午，放弃…… 

           我看bootstrap的demo用的js来实现这个affix (也不是按照文档里说的data-spy来实现)，看看我们也能不能这么来做？

           done. 
           
           照着bootstrap affix 的 live demo，再查了一下，就好了。还是不认真，白走了 css sticky的弯路了。

           bootstrap的 affix 需要设置 .affix 和 .affix-bottom 样式 ，如

           ```
           .affix {
               position: fixed;
               top: 0;
           }
           .affix-bottom {
               position: absolute;
           }
           ```

           其中之前没有设置 `affix-bottom`，导致affix有问题，内嵌style会多一个`position: relative`，导致触发 `affix-bottom`后，
           往回`affix`就失效了；
           搜到了github上的issue，发现不认真的不止我一个 = =

    3. 通过formatter开关是否要生成目录 [TODO]
    4. 目录有2个，分别是“内容顶部”和“侧边导航”。 显示逻辑如下：【TODO】
        
        a. 内容顶部的目录在开启生成目录的时候总是显示； 
        
        b. "侧边导航"目录若设备太窄，不显示
        
        c. 若设备宽度足够，默认不显示；同时在“内容顶部”显示一个“移动到右侧”的按钮；
            
            a. 当点击该按钮时，“内容顶部”目录消失，“侧边导航”目录显示
            b. “侧边导航”右上方有一个“关闭”按钮，点击后，关闭“侧边导航”目录，“内容顶部”目录显示。
            c. 若“侧边导航”宽度太少，可考虑在此动态变化过程中改变容器size；


## 2020.05.30 

惭愧，过去一年并没有更新什么…… 接下来还是多看、多记录吧！

从github-pages 迁移到了 gitee pages. 要说功能，那必须 github 强，gitee 似乎得手动更新pages，有点坑啊； 算啦算啦，还是换回github pages；

又有些本末倒置了呀。

## 2019.05.13

这个域名在2019年看来以及不太合适了，不过暂时还是不太像买域名 = =，还是先继续填充内容吧。2017、18都荒废了，现在应该要重新开始！

把倒闭的多说、jiathis全给去掉了。致敬。

之前本来只想加个qq邮箱的mail2me的，但是最后还是加了disqus，能用的就用，不能用的话也没什么影响，可以通过微博留言，哈哈。


## 2016.10.09

收到微博留言，指出一篇技术文章的问题。感觉有必要加上评论功能了，耗时两个小时（熬夜1个小时），在Disqus、多说、友言中选择了多说，因为发现多少的自定义功能还是非常强大的（我把自己认为多余的元素全部隐藏了），而且CSS配置在多说那边就完成了，不需要改网站的CSS样式。很好，很方便。附上CSS样式（其中圆角矩形的配置是copy的多说中推荐的第二篇文章）：

```CSS
#ds-reset .ds-avatar img{  
width:54px;height:54px; /*设置图像的长和宽*/  
border-radius: 27px;/*设置图像圆角效果,在这里我直接设置了超过width/2的像素，即为圆形了*/  
-webkit-border-radius: 27px;/*圆角效果：兼容webkit浏览器*/
-moz-border-radius:27px;
box-shadow: inset 0 -1px 0 #3333sf;/*设置图像阴影效果*/  
-webkit-box-shadow: inset 0 -1px 0 #3333sf;
}

#ds-reset ds-meta{
display: none;
}

#ds-reset ds-toolbar{
display: none;
}

#ds-thread #ds-reset .ds-replybox{
padding:10px 0px 0px 0px;
}

#dsReplyBoxBlock ds-avatar-visitor{
display: none;
}
```

在评论时显示还是有些小BUG的，不过这对我来说并不重要。
