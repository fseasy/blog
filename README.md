# Pages

For Web Service

### 更新

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
