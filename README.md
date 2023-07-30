# Page

[![](https://img.shields.io/badge/blog-servering-green.svg)](https://blog.fseasy.top)

For Web Service. 

## 博客核心目标

1. 传递情感
2. 记录经验
3. 呼吁大众

[CHANGELOG](CHANGELOG.md)

## 部署注意事项

1. 现在依赖 `blog-extra-file` 这个仓库。它存储博客额外的、非关键文件(如非核心图片等)。服务器部署会通过 Nginx 自动设置位置（通过 `alias`），其访问路径为 `/bef/$PATH`. 本地部署 Jekyll server 不能设置路径映射，只能尝试把文件夹放到 _site 下。尝试了很久， jekyll 始终不能将符号文件自动拷贝到 `_site`（ `safe=false` 不行），所以就只能将 blog-extra-file 这个仓库拉到根目录，并改名为 `bef` 了——因为已经为当前的 .gitignore 添加了 `bef`, 所以不会影响当前仓库的 git. 只是这样有可能会让每次构建都拷贝文件，造成写浪费（实际上不确定是否会重复拷贝啦）。

## 博客撰写注意事项

1. `Front Matter` 自定义参数：

   ```ini
   # 设置 Toc 渲染：
   # - disable 禁止； 
   # - force_enable 强制渲染； 
   # - 其他值，根据层级数量阈值内部判断是否要渲染
   toc: "disable"|"force_enable"|any-other-or-empty
   # 设置 mathjax 渲染：
   # - disable 禁止引入 Mathjax 依赖
   # - force_enable 强制引入 Mathjax 依赖
   # - 其他值，根据内容判断是否要引入 Mathjax 依赖
   mathjax: "disable"|"force_enable"|any-other-or-empty
   ```

2. 添加内部跳转
   正向、反向跳转
   
   ```markdown
   source(from)部分: `<span id="j_ex_orb_s"></span>[⇂](#j_ex_orb_t)`
   target(to)部分: `<span id="j_ex_orb_t"></span>[↾](#j_ex_orb_s)`
   ```

   原理比较简单：通过 span 的 id 创建锚点；再通过 Markdown 的链接实现页面内跳转。
   写这里方便复制，注意修改 id.

3. 文本段落居中

   ```html
   <div class="text-center">
      <p>还我普通人</p>
      <p>把摄像头扛到大街上去</p>
   </div>
   ```

4. 文本 span 样式：

   `.text-highlight`: 高亮样式

## 其他

1. 小技巧：
   
   - Markdown的blockquote, 如果想换行，可以在最原始文本一行的末尾加**2个额外空格**，就可以！([ref](https://stackoverflow.com/questions/26991997/multiple-line-quote-in-markdown))

   - mathjax 中要写连字符，不能用 `-`, 会被变成减号，用 `\mbox{-}`. 如 $abc\mbox{-}def$；
     > 在 lark 文档里这个方法不行，[推荐方法](https://katex.org/docs/supported.html#symbols-and-punctuation)是用 `\text{-}`, `\text{--}` 或者 `\text{\textendash}`

   - 数学公式里的花括号显示问题： 在 span-level 的公式里，用 `${a, b}$` 最后渲染出来没有花括号；
     这是因为 Latex 里`{}` 都[是特殊字符，必须转义才行][curly_braces_tex]。
     然而，就算用 `$\{a, b\}$` 也还是不行！原来，Jekyll 预处理时把 `\{\}` 中转义符吞掉了(kmarkdown 渲染器). 要在 markdown 里用 `\\{\\}` 最终在页面里才能看到 ${}$. 然而这样的做法又不通用（不是标准写法）

     目前可行的方案是用 `\lbrace \rbrace` 来显示 `{}`.

     参考 [mathjax curly brackets dont show-up using jekyll][curly_braces_jekyll], 关键词 `curly brackets`, `curly braces`

2. 代码渲染，现在使用 [Prism](https://prismjs.com/). 
  
  - 发现 Prism 渲染的bug：`c++` 不能被正确渲染，`cpp` 可以。后续用 `cpp` 表达。


[curly_braces_tex]: https://tex.stackexchange.com/questions/123050/quick-question-about-curly-braces-not-showing-up

[curly_braces_jekyll]: https://stackoverflow.com/questions/41312777/mathjax-curly-brackets-dont-show-up-using-jekyll
