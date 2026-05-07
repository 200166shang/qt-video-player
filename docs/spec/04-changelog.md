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
- `scripts/build.sh` — 一键 CMake 构建脚本

### Changed

无（新项目，首次迭代）。

### Fixed

无。

### Notes

- Conan 依赖管理留待 Iteration 02
- 无实际播放能力，UI 控件为占位状态
- 无 FFmpeg 集成
- ControlBar 按钮可点击，无实际播放逻辑
- 进度条当前为可交互占位（未绑定播放状态）
- 本地构建验证受环境限制：当前机器未配置 Qt6 CMake 包（`Qt6Config.cmake` 不可见）
- 可执行文件目标输出目录约定为项目根 `bin/`

---

## Iteration 02: Conan + CMake + Dependencies

### Date

2026-05-08

### Summary

完成 Conan 依赖管理接入、CMake 依赖链接、日志宏封装、YAML AppConfig、FFmpeg 探针模块。

### Added

- `conanfile.py`（FFmpeg、spdlog、fmt、nlohmann_json、stb、yaml-cpp 依赖及 CMake 生成器）
- `src/utils/Logger.*`（spdlog 日志封装 + `LOG_*` 宏）
- `src/config/AppConfig.*`（默认 YAML 配置加载）
- `src/ffmpeg/FFmpegProbe.*`（FFmpeg 可用性探针）
- `src/utils/StbVersion.*`（stb 接入验证）
- `config/default.yaml`（默认应用配置）

### Changed

- `CMakeLists.txt` 增加第三方依赖 `find_package`、模块化静态库 target 与链接关系
- `src/main.cpp` 增加日志初始化、YAML 配置加载与依赖探针调用
- `scripts/build.sh` 增加 Conan 安装与 Toolchain 自动接入逻辑

### Fixed

- 修复 Conan 依赖解析冲突（`spdlog` 与 `fmt` 版本/后端匹配）
- 修复 Conan target 名称大小写与 CMake 链接名不一致问题
- 修复 `AppConfig` 日志调用在静态库链接阶段的符号解析问题

### Notes

- 验证通过：`conan install` 成功、`cmake configure` 成功、`cmake build` 成功
- UI 目录未引入 FFmpeg 头文件，保持 `UI -> FFmpeg` 禁止依赖约束
- 日志输出包含线程 id、函数、文件与行号（通过 spdlog source location）

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
