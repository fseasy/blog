# Scripts

博客辅助脚本，使用前需激活虚拟环境：

```sh
cd scripts_ && source .venv/bin/activate
```

依赖见 `pyproject.toml`，系统层面需要 `exiftool`（fix-gallery-assets-create-time.py 使用）和 `ffmpeg`（gen-year-in-review-gallery-yaml.py 使用）。

## 脚本列表

### new_post.py

根据标题 slug 创建新的博客文章。生成的模板文件放在 `_posts/YYYY/` 下。

```sh
python new_post.py reading fusheng
python new_post.py "reading fusheng"
```

### fix-gallery-assets-create-time.py

修复照片/视频的创建时间戳（EXIF 元数据和系统文件时间），同时将文件名标准化为 `IMG_YYYYMMDD_HHMMSS-xxx.ext` 或 `VID_` 前缀格式。

两种运行模式：
- `auto` — 自动扫描目录处理所有媒体文件
- `manual` — 根据 mapping 文件手动处理

```sh
# 自动模式
python fix-gallery-assets-create-time.py auto -p /path/to/media

# 手动模式
python fix-gallery-assets-create-time.py manual -p /path/to/media -m mapping.txt
```

时间解析优先级：文件名（IMG/VID/MVIMG/MEITU/mmexport 格式）> EXIF 元数据。如有 GPS 信息则通过时区转换，否则使用系统本地时间。解析失败或年份为当前年的文件会记录到 `suspicious_files.txt`。

### gen-year-in-review-gallery-yaml.py

从原始图片/视频生成 gallery YAML 数据文件，用于 `gallery.html` 组件。

处理流程：
1. 扫描源目录，按子目录分组
2. 图片转为 WebP，视频转为 WebM（VP9，极致压缩）
3. 提取首帧作为视频封面（WebP）
4. 收集尺寸、UTC 时间戳等元数据
5. 输出 `gallery_data.yml`

```sh
python gen-year-in-review-gallery-yaml.py -i /input/dir -o /output/dir
```

### migrate_bef_links.py

将 Markdown 文件中的 `/bef/` 外链替换为 Jekyll include 语法 `{% include bef.html path='...' %}`。支持 dry-run 预览。

```sh
# 预览变更
python migrate_bef_links.py --dry-run

# 执行迁移
python migrate_bef_links.py
```
