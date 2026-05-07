# PlayerLab Changelog

本文档记录：

- 每个 iteration 实际完成了什么
- 哪些模块新增
- 哪些模块修改
- 哪些问题被修复
- 当前已知限制

规则：

1. 只记录已经完成的 iteration。
2. 不预写未来 iteration。
3. 每次 iteration 完成后追加。
4. changelog 记录"结果"，不是设计讨论。
5. changelog 不替代 ADR。

---

## Iteration 01: Bootstrap UI + OpenGL Placeholder

### Date

2026-05-07

### Summary

建立项目骨架、Qt 主窗口 UI、OpenGL 视频占位控件、渲染器抽象接口。

### Added

- CMake 项目骨架（C++20, Qt6 Widgets + OpenGLWidgets）
- `ui/MainWindow` — 主窗口，组装全部控件
- `ui/Sidebar` — 左侧边栏（占位）
- `ui/TopBar` — 顶部标题栏
- `ui/OpenGLVideoWidget` — 中央 QOpenGLWidget（纯色背景）
- `ui/MediaLibraryWidget` — 底部媒体库区域（占位）
- `ui/MediaInfoPanel` — 右侧媒体信息面板（占位）
- `ui/ControlBar` — 底部控制栏（Play/Open 按钮 + 进度条）
- `render/IVideoRenderer` — 渲染器抽象接口
- `render/OpenGLVideoRenderer` — OpenGL 渲染器空实现（glClear）
- `render/DebugImageRenderer` — Debug 渲染器空实现
- `render/RendererFactory` — 渲染器工厂
- `shaders/` 目录占位

### Changed

无（新项目，首次迭代）。

### Fixed

无。

### Notes

- Conan 依赖管理留待 Iteration 02
- 无实际播放能力，UI 控件为占位状态
- 无 FFmpeg 集成
- ControlBar 按钮可点击，无实际播放逻辑
- 进度条保持 disabled 状态（无可播放媒体）

---

# Changelog Template

## Iteration XX: Title

### Date

YYYY-MM-DD

### Summary

一句话总结本 iteration 完成内容。

### Added

- 新增模块
- 新增功能
- 新增接口

### Changed

- 修改了什么
- 重构了什么

### Fixed

- 修复了什么问题

### Notes

- 当前限制
- 已知问题
- 后续注意事项
