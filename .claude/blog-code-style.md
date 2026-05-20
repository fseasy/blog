---
name: blog-code-style
description: blog (Jekyll)项目的代码风格规范
metadata:
  type: reference
---

# blog 项目代码风格规范

## CSS / SCSS

- **颜色必须用 CSS 变量**：所有颜色从 `assets/css/main.scss` 的 `:root` 中选择，不能 hardcode。
  - 现有变量包括 `--text-dark-color`、`--text-dark-grey-color`、`--text-dark-more-grey-color`、`--brand-color`、`--blockquote-bg`、`--table-border` 等
  - 如果现有变量不够用，再新增，并确保 dark mode 下有对应值
- **通用组件样式** 放到 `assets/css/_sass/_components.scss`，按需添加 `@use` 引用
- **仅在一个文件内使用的样式** 可以 inline 在 HTML `<style>` 中，但颜色仍用 CSS 变量
- 按钮样式用 `.btn-ghost`（已在 `_components.scss` 中定义），不要重复定义

## SVG 图标

- 所有 SVG icon 统一放到 `assets/site/icons.svg`，用 `<symbol>` + `id` 定义
- HTML 中引用：`{% raw %}<svg><use href="{{ '/assets/site/icons.svg' | relative_url }}#icon-xxx"></use></svg>{% endraw %}`
- 不要在 HTML 中 inline 大段 SVG，只 inline 简单无参数的

## JavaScript

- 动态加载 ES module 用 `import(url)` 动态导入，不要用 `script.src` + `type="module"` 配合 `onload`
- 状态管理用 flag 变量（如 `walineLoaded`），避免重复加载已初始化的组件
- 隐藏元素用 `classList.add("hidden")` / `classList.remove("hidden")`，不要 `style.display`

## Jekyll / Liquid

- `site.waline.serverURL` 配置在 `_config.yml` 的 `waline.serverURL` 下
- `page.url` 可获取当前页路径（不含 domain）
