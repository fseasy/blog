# FSEASY Personal Blog

[![](https://img.shields.io/badge/blog-servering-green.svg)](https://blog.fseasy.top)

For Fseasy blog service. 

## 博客核心目标

1. 传递情感
2. 记录经验
3. 呼吁大众

[CHANGELOG](CHANGELOG.md)

## 部署注意事项

### Asset Digest（Cache Busting）

`_plugins/asset_digest.rb` 通过 `asset_digest` Liquid filter 为 asset 文件生成基于内容的 hash query param：

- **用法**：`{{ '/assets/css/main.css' | asset_digest | relative_url }}`
- **输出示例**：`/assets/css/main.css?6b2c5569`
- **CSS with SASS source**：hash = SHA256(main.scss + `sass_dir` 下所有 .scss/.sass 文件)，确保任意 SASS 修改都触发新 hash
- **Third-party CSS / 其他文件**：hash = SHA256(文件内容)
- **缓存**：结果缓存在 `site.data['asset_digest']`，同一 build 内不重复计算

### blog-extra-file 依赖

1. 依赖 `blog-extra-file` 仓库，存储博客额外文件(如非核心图片等)；对应 `/bef` 路径地址。
2. 本地开发：`_plugins/symlink_bef.rb` 在 post_write 时创建软链接到 `_site/bef`，依赖 `_config.yml` 中的 `bef_process.local` 配置。
3. 生产环境(GitHub Pages)：使用 `_includes/bef.html` include 文件，访问时替换 `/bef/` 为 `bef_process.production.host`。

   **用法**：
   ```html
   <!-- 旧写法 -->
   <img src="/bef/posts/xxx/image.webp">

   <!-- 新写法 -->
   <img src="{% include bef.html path='posts/xxx/image.webp' %}">
   ```

   **配置** `_config.yml`:
   ```yaml
   bef_process:
     local:
       src: "../blog-extra-file"
       dst: "bef"
     production:
       host: "https://bef.fseasy.top"
   ```

   **迁移脚本**：`scripts/migrate_bef_links.sh` 可批量迁移旧链接（交互式，逐文件确认）

   PS：本地 soft link 在 `jekyll serve` watch 模式下会被删除，plugin post_write 方式可规避此问题。

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
   # - 加载 Gallery 相关的 css+js
   use_gallery: true | false
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


5. Gallery 图集系统

   **数据生成**：
   ```sh
   cd scripts_ && source .venv/bin/activate
   python gen-year-in-review-gallery-yaml.py -i <原始图片目录> -o <输出目录>
   ```
   - 脚本会压缩图片/视频为 webp/webm（最大 800x800），生成 YAML 数据文件
   - 支持从文件名（`IMG_20250130_185031.jpg`）或 EXIF metadata 提取拍摄时间
   - 输出：`gallery_data.yml` + 处理后的媒体文件

   **YAML 数据结构**：
   ```yaml
   group_id:
     - relative_path: "group/image.webp"   # 相对路径
       caption: "描述文字"                  # 可选
       dt_utc: "2025-01-30 18:50:31"      # UTC 时间
       width: 800                           # 宽度
       height: 600                          # 高度
       poster: "group/video_poster.webp"   # 视频封面，仅视频有
   ```

   **在 Post 中使用**：
   ```html
   {% include extra_fn/render_gallery.html
      data="gallery_2025_year_in_review"
      data_group_id="albert"
      path_prefix="posts/2025-year-in-review"
      display_style="justified"
   %}
   ```
   - `data`: `_data/` 下的 YAML 文件名（不含扩展名）
   - `data_group_id`: YAML 中的分组 key
   - `path_prefix`: 路径前缀
   - `display_style`: `justified`（行高相同）| `masonry`（瀑布流，列宽相同）| `masonry-row`（瀑布流，基于 masonry.js）

   **相关文件**：
   - `scripts_/gen-year-in-review-gallery-yaml.py` - 生成脚本
   - `scripts_/fix-gallery-assets-create-time.py` - 修复媒体文件 EXIF 时间戳
   - `_data/gallery_*.yml` - Gallery 元数据
   - `_includes/extra_fn/render_gallery.html` - 渲染组件
   - 依赖：`ffmpeg`（压缩）, `exiftool`（时间提取）

6. 可折叠内容 (fold.html)

   使用 `{% capture body %}` + `{% include component/fold.html %}` 模式：

   ```html
   {% capture body %}
   <div markdown="1">
   Content here (supports **markdown**)
   </div>
   {% endcapture %}

   {% include component/fold.html
      type="note"
      summary="Section Title"
      open=false
      content=body
   %}
   ```

   **参数**：
   - `type`: `note` | `tip` | `warn` | `error`
   - `open`: `true` (默认展开) | `false` (默认折叠)
   - `summary`: 折叠块的标题

   **VSCode**：运行 `>j-fold` 命令可折叠/展开整个 capture 块。



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

   - `<details>` 里面要想让 Markdown 被正确解析, 可以把 Markdown 内容用 `<div markdown="1">` 包裹一下，就可以正常解析了。

     原理： `markdown="1"` 是 kramdown（Jekyll 默认的 Markdown 引擎）提供的一个 专用扩展属性，作用非常直接：强制在这个 HTML 标签内部继续解析 Markdown。

2. 代码渲染，现在使用 [Prism](https://prismjs.com/). 
  
  - 发现 Prism 渲染的bug：`c++` 不能被正确渲染，`cpp` 可以。后续用 `cpp` 表达。


[curly_braces_tex]: https://tex.stackexchange.com/questions/123050/quick-question-about-curly-braces-not-showing-up

[curly_braces_jekyll]: https://stackoverflow.com/questions/41312777/mathjax-curly-brackets-dont-show-up-using-jekyll
