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

## 其他

1. 小技巧：Markdown的blockquote, 如果想换行，可以在最原始文本一行的末尾加**2个额外空格**，就可以！([ref](https://stackoverflow.com/questions/26991997/multiple-line-quote-in-markdown))

2. 代码带行号渲染，参考： [Jekyll: Syntax Highlighting And Line Numbers](https://www.bytedude.com/jekyll-syntax-highlighting-and-line-numbers/)，效果其实也不算特别好…… 或许有其他更好的方案（除了hightlight.js等只高亮的，还想有折叠、copy等博客平台常有的功能）