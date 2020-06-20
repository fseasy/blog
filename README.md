# Pages

For Web Service

### 更新

#### 2020.06.20

本来打算增加一个目录的，结果发现mathpage layout里的东西都过时了（还在使用jiathis, duoshuo...），
瞬间决定之前写得还是挺烂的……

1. 通过搜索，找到了 [share.js](https://github.com/overtrue/share.js/), 挺漂亮的，而且定制化也很强
    
    - 发现只有通过`data-xxx`才能配置成功，通过js里配置不成功
    - 微信分享有问题，变成一个文件…… 

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


#### 2020.05.30 

惭愧，过去一年并没有更新什么…… 接下来还是多看、多记录吧！

从github-pages 迁移到了 gitee pages. 要说功能，那必须 github 强，gitee 似乎得手动更新pages，有点坑啊； 算啦算啦，还是换回github pages；

又有些本末倒置了呀。

#### 2019.05.13

这个域名在2019年看来以及不太合适了，不过暂时还是不太像买域名 = =，还是先继续填充内容吧。2017、18都荒废了，现在应该要重新开始！

把倒闭的多说、jiathis全给去掉了。致敬。

之前本来只想加个qq邮箱的mail2me的，但是最后还是加了disqus，能用的就用，不能用的话也没什么影响，可以通过微博留言，哈哈。


#### 2016.10.09

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
