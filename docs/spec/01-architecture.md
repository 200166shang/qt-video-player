# PlayerLab Architecture Contract

## 1. 目标

本文档定义 PlayerLab 的核心架构边界。

这些规则用于防止项目在迭代过程中逐渐退化成：

- MainWindow 巨石类
- UI 与 FFmpeg 强耦合
- OpenGL 后补丁式接入
- 解码 / 渲染 / 音频职责混乱
- 不可维护的播放器 demo

如果后续发现某些设计需要改变：

1. 先更新 ADR
2. 再修改本文件
3. 再调整 roadmap / iteration

---

# 2. 架构原则

## 2.1 UI 与播放核心解耦

UI 只负责：

- 界面展示
- 用户输入
- 状态展示
- 调用播放器控制接口

UI 不负责：

- 解封装
- 解码
- OpenGL shader 逻辑
- 音视频同步
- packet/frame 调度

---

## 2.2 OpenGL 是主渲染方向

项目的视频渲染必须围绕 OpenGL 设计。

允许：

- DebugImageRenderer
- QImage fallback
- RGB 临时调试路径

但这些只能作为：

- fallback
- debug
- 临时验证

不能成为最终主路径。

播放器最终必须保留：

- renderer abstraction
- OpenGL renderer
- shader pipeline
- texture upload path

---

## 2.3 FFmpeg 集中管理

FFmpeg 相关代码必须集中在 ffmpeg 模块。

包括：

- demux
- decode
- resample
- scaler
- network init
- media info
- packet/frame handling

禁止 UI 直接依赖 FFmpeg。

---

## 2.4 播放控制统一入口

UI 必须通过统一控制入口控制播放器。

项目必须存在：

- PlayerController

UI 不允许：

- 直接操作 decoder
- 直接操作 packet queue
- 直接操作 AVFormatContext
- 直接操作 render queue

---

## 2.5 模块边界清晰

项目至少需要以下模块边界：

- ui
- core
- ffmpeg
- render
- audio
- subtitle
- playlist
- config
- utils

允许后续调整内部结构。

但不允许：

- 所有代码堆在 ui
- 所有代码堆在 mainwindow
- 所有代码堆在 player.cpp

---

## 2.6 本地与网络媒体统一抽象

本地文件与网络 URL 必须统一表达为媒体源概念。

项目需要保留：

- MediaSource

但具体字段与接口可以后续调整。

---

## 2.7 渲染与音频必须可替换

视频渲染必须具备 renderer abstraction。

音频输出必须具备 audio output abstraction。

项目需要保留：

- IVideoRenderer
- IAudioOutput

但具体接口签名不要求现在完全固定。

---

## 2.8 配置与播放列表独立于 UI

配置、最近打开、播放列表不应该散落在 UI 代码中。

需要独立模块：

- config
- playlist

---

## 2.9 每个迭代必须可运行

每个 iteration 结束后：

- 必须能编译
- 必须能运行
- 必须能验证结果

不允许长期处于：

- 半完成状态
- 无法启动状态
- 大面积 TODO 状态

---

# 3. 模块职责

## 3.1 UI

负责：

- MainWindow
- 控件布局
- 用户输入
- 状态展示
- 进度展示
- 媒体信息展示

禁止：

- include FFmpeg headers
- 写解码循环
- 写 packet/frame queue
- 写音视频同步逻辑
- 写 OpenGL shader 逻辑

---

## 3.2 Core

负责：

- 播放状态
- 播放控制
- 播放流程协调
- 状态通知
- 生命周期管理

---

## 3.3 FFmpeg

负责：

- 打开媒体
- 解封装
- 解码
- 重采样
- 像素格式转换
- 网络初始化
- 媒体信息读取

---

## 3.4 Render

负责：

- 视频帧显示
- OpenGL 渲染
- texture upload
- shader pipeline
- renderer switch

---

## 3.5 Audio

负责：

- PCM 输出
- 音量
- 静音
- 音频播放进度

---

## 3.6 Subtitle

负责：

- subtitle parsing
- subtitle timing
- subtitle render

---

## 3.7 Playlist

负责：

- playlist items
- recent files
- next / previous item logic

---

## 3.8 Config

负责：

- app config
- persistent settings
- renderer/audio backend selection
- playback preferences

---

# 4. 关键抽象（仅固定名字与方向）

项目需要保留以下关键抽象。

允许后续调整接口。

- MediaSource
- PlayerController
- PlaybackEngine
- MediaInfo
- VideoFrame
- AudioFrame
- PlaybackClock
- AVSynchronizer
- IVideoRenderer
- IAudioOutput

---

# 5. 依赖方向

允许：

UI -> Core

Core -> FFmpeg
Core -> Render
Core -> Audio
Core -> Subtitle
Core -> Playlist
Core -> Config

禁止：

UI -> FFmpeg
UI -> Decoder
UI -> PacketQueue
UI -> FrameQueue

Render -> UI business logic
Audio -> UI business logic

---

# 6. 依赖管理

第三方依赖优先通过：

- Conan 2.x
- CMake

管理。

Qt 使用系统安装或 Qt 官方安装器。

不强制通过 Conan 管理 Qt。

---

# 7. 线程原则

UI 线程只负责 UI。

解码、音频输出、同步、渲染调度不应该长期阻塞 UI。

播放器关闭时必须安全退出后台线程。

---

# 8. 变更规则

如果后续需要调整：

- 架构边界
- 模块职责
- 渲染主方向
- 播放控制方式
- 依赖管理方式

必须：

1. 先更新 ADR
2. 再更新本文件
3. 再更新 roadmap / iteration
4. 再修改代码
