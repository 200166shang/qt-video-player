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

## Iteration 03: Media Info Reader

### Date

2026-05-08

### Summary

实现本地媒体文件打开与 FFmpeg 媒体信息读取链路，UI 可展示容器/视频/音频信息并显示打开错误。

### Added

- `src/core/MediaSource.h`（本地媒体源抽象）
- `src/core/MediaInfo.h`（媒体信息数据结构）
- `src/core/PlayerController.*`（统一打开入口与 UI 通知）
- `src/ffmpeg/FFmpegGlobal.*`（FFmpeg 全局初始化）
- `src/ffmpeg/FFmpegDemuxer.*`（本地文件打开与流信息读取）

### Changed

- `src/ui/ControlBar.*` 增加 `openRequested` 信号，Open 按钮可触发上层打开流程
- `src/ui/MainWindow.*` 串联 `ControlBar -> PlayerController -> MediaInfoPanel`
- `src/ui/MediaInfoPanel.*` 从静态占位改为动态显示文件路径、格式、时长、视频和音频信息，并支持错误提示
- `CMakeLists.txt` 纳入 iteration 03 新增模块文件

### Fixed

- 修复 FFmpeg 帧率读取时 `av_guess_frame_rate` 的参数 const 不匹配编译问题

### Notes

- 本迭代仅实现媒体信息读取，不包含视频播放、音频播放、seek
- 验证通过：`cmake --build build -j8` 成功
- `scripts/build.sh` 仍存在现有 Conan profile `gnu17` 与 `C++20` 要求冲突问题（与本迭代功能无关）

---

## Iteration 05: Audio Decode + Audio Output

### Date

2026-05-09

### Summary

实现音频解码、重采样与 Qt 音频输出链路，并接入音量、静音、音频暂停/恢复控制。

### Added

- `src/core/AudioFrame.h`（音频帧数据结构）
- `src/ffmpeg/FFmpegResampler.*`（音频重采样到 48kHz、stereo、S16）
- `src/ffmpeg/FFmpegAudioDecoder.*`（音频流解码线程与音频帧队列）
- `src/audio/IAudioOutput.h`（音频输出抽象接口）
- `src/audio/QtAudioOutput.*`（Qt Multimedia 音频输出实现）

### Changed

- `src/core/PlayerController.*` 接入音频解码与输出管线，新增 `setVolume / setMuted / setPaused`
- `src/ui/ControlBar.*` 增加静音按钮、音量滑块、暂停/恢复信号
- `src/ui/MainWindow.cpp` 串联 UI 音频控制到 `PlayerController`
- `CMakeLists.txt` 新增 Qt6 Multimedia 依赖并纳入 iteration 05 新文件

### Fixed

- 修复 `FFmpegResampler` 对 `av_channel_layout_default` 返回值误用导致的编译错误
- 修复 `QtAudioOutput` 中 `QAudioSink` 不完整类型导致的析构编译错误

### Notes

- 验证通过：`cmake --build build -j8` 成功
- 本次未实现 AV 同步（属于 Iteration 06）
- 本次未实现 seek/播放速度/完整状态机（属于 Iteration 07）

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
