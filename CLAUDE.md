# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A personal blog ("鱼虾写字") built with Jekyll, deployed to GitHub Pages at https://blog.fseasy.top. Content is primarily in Chinese. The blog covers technical topics (C++, algorithms, ML, SLAM), book/movie reviews, and personal reflections.

## Common Commands

### Local Development Server
```sh
./local_serve.sh          # incremental build + serve on 0.0.0.0
./local_serve.sh full     # full (non-incremental) build + serve
```
This runs: `bundle exec jekyll serve -w --incremental --host 0.0.0.0`

### Environment Setup
```sh
./update_local_env.sh     # switch Gemfile to CN mirrors, run bundle update
```
Ruby version is managed via mise (see `mise.toml`): Ruby 3.3.

### Production Build
```sh
./server_build.sh         # switch to US Gemfile, bundle update, JEKYLL_ENV=production jekyll build
```
For server-side builds. Sets `JEKYLL_ENV=production` which disables dev-only plugins.

### Python Scripts (in scripts_/)
```sh
cd scripts_ && source .venv/bin/activate
python gen-year-in-review-gallery-yaml.py   # generate gallery YAML data from images
python fix-gallery-assets-create-time.py    # fix EXIF timestamps in gallery assets
```
Python scripts are managed by `uv` (see `scripts_/pyproject.toml`). Uses `ruff` as linter (indent-width = 2). Python 3.12+.

## Architecture

### Jekyll Structure

- **Two layouts**: `index.html` (listing pages: home, categories, tags, about) and `post.html` (individual blog posts with TOC, comments, sharing, MathJax, Prism.js, side-nav)
- **`_includes/`** - shared HTML partials: `head.html` (meta/SEO), `page_header.html` (nav bar with 文章/分类/标签/About + Google search), `page_footer.html`, `post_meta.html` (date, Chinese word count, categories, tags), `pagination.html`
- **`_includes/extra_fn/`** - feature modules loaded conditionally by layouts: TOC (`content_toc.html`, `side_nav.html`, `load_toc_js.html`), MathJax, Prism.js code highlighting, share buttons, Isso comments (replaces deprecated Disqus), gallery rendering, `link_target_blank_script.html` (external links in new tab), `dynamic_style_setter.html` (runtime CSS adjustments)
- **`assets/js/`** - `toc.js` — 合并了 TOC 生成、双栏布局、侧边栏交互逻辑，按需加载
- **`_includes/component/`** - reusable UI components (e.g., `fold.html` — collapsible callout/details with note/tip/warn/error variants)
- **`_includes/utils/`** - utility includes (e.g., `path-join.html`)

### Styling

Sass with CSS variables for light/dark mode. Entry point: `assets/css/main.scss` which `@use`s partials from `assets/css/_sass/` (variables, base, header, footer, layout, post, side_rail, code, webfont, overwrite3rd). Dark mode uses `prefers-color-scheme: dark` media query inside `:root`.

### Content Organization

- **`_posts/`** - blog posts organized by decade/year directories (e.g., `_posts/201x/2015/`, `_posts/202x/2024/`). Filename format: `YYYY-MM-DD-title.md`
- **`gallery-posts/`** - gallery-style posts with photo/video collections, using PhotoSwipe. These are independent from `_posts/` to avoid bloating the main post list
- **`subject/`** - standalone subject pages (ORB-SLAM2 analysis, ML notes, C++ notes, algorithm posts, reading logs) — these are **not** in `_posts/` but still use `layout: post`
- **`_data/`** - YAML data files for gallery metadata (e.g., `gallery_2025_year_in_review.yml`)
- **`md_templates_/`** - templates for new posts (tech, reviews, misc)
- **`_draft/`** - unpublished drafts
- **`private_/`** - private content (not deployed)

### Custom Front Matter

Posts support these optional front matter fields:
- `toc`: `"disable"` | `"force_enable"` | unset (auto-detect by heading count ≥ 4)
- `mathjax`: `"disable"` | `"force_enable"` | unset (auto-detect by content)
- `use_gallery`: `true` | `false` — loads PhotoSwipe CSS+JS

### Gallery System

Gallery posts reference data from `_data/*.yml`. Each entry has: `relative_path`, `caption`, `dt_utc`, `width`, `height`, `poster` (for video). The `render_gallery.html` include takes `data`, `data_group_id`, optional `base_dir`, and `display_style` (`justified` [default] / `masonry` / `masonry-row`). Third-party gallery libraries: PhotoSwipe (lightbox with video plugin + dynamic captions), Masonry.js.

### blog-extra-file Dependency

External repo `blog-extra-file` stores non-essential static assets (e.g., non-core images), served at `/bef/` path. In production, Nginx routes `/bef/` to the repo's location. In local dev, the `_plugins/symlink_bef.rb` plugin creates a symlink in `_site/bef/` after each Jekyll write (only in development environment). Config is in `_config.yml` under `local_bef_resources`.

## Deployment

GitHub Actions workflow (`.github/workflows/github-deploy.yml`): on push to master, builds with `JEKYLL_ENV=production` using `Gemfile.US` (rubygems.org source), then deploys to GitHub Pages.

## Key Quirks

- **Gemfile switching**: `Gemfile.CN` uses Chinese mirrors, `Gemfile.US` uses rubygems.org. The current `Gemfile` is a copy of one of these (gitignored). Use `update_local_env.sh` or `server_build.sh` to switch.
- **Code highlighting**: Kramdown's built-in highlighter is disabled (`highlighter: none`); Prism.js is used instead. Use `cpp` not `c++` for language tags (Prism bug). Prism and MathJax are conditionally loaded per page based on content detection + front matter flags.
- **MathJax in Markdown**: Use `\lbrace`/`\rbrace` for curly braces (not `\{\}`), and `\mbox{-}` or `\text{-}` for hyphens in math mode.
- **Markdown inside HTML**: Wrap content in `<div markdown="1">` to force kramdown parsing within HTML tags (e.g., inside `<details>`). This is a kramdown-specific extension attribute.
- **Internal jump links**: Use `<span id="anchor_name"></span>` + `[link](#anchor_name)` pattern.
