# 00 - 项目概览

## 0. 项目定位

### 0.1 项目名称

```text
PlayerLab
```

### 0.2 项目目标

开发一个基于 **Qt + FFmpeg + OpenGL** 的 PC 桌面播放器工具。

它不是一次性 Demo，而是一个可持续演进的播放器工程，目标是支持：

```text
1. 本地视频播放
2. 网络视频播放
3. 音视频同步
4. 播放列表
5. 媒体信息查看
6. 字幕显示
7. OpenGL 视频渲染
8. 播放控制
9. 截图、全屏、单帧步进
10. 后续扩展硬解码、libass 字幕、OpenAL 音频、OpenCV 工具能力
```

---

# 1. 核心技术栈

## 1.1 第一阶段必选技术栈

```text
C++20
Qt 6 Widgets
Qt OpenGL / QOpenGLWidget
FFmpeg
OpenGL
CMake
Conan 2.x
spdlog
fmt
nlohmann_json
stb
```

## 1.2 第一阶段暂不强制实现，但需要预留接口

```text
libass
OpenAL Soft
OpenCV
SQLite
FFmpeg avfilter
FFmpeg hwaccel
```

## 1.3 技术选型原则

```text
1. Qt 负责 UI 和桌面应用框架。
2. FFmpeg 负责解封装、解码、网络流读取、音频重采样、像素格式处理。
3. OpenGL 是核心视频渲染后端，不是后续可选优化项。
4. 播放核心不依赖 UI 具体控件。
5. UI 不直接调用 FFmpeg API。
6. 所有底层能力通过 core/controller 层暴露给 UI。
7. 第三方库优先通过 Conan 管理。
8. Qt 优先通过系统安装或 Qt 官方安装器接入，不强制用 Conan 管理。
```

---

# 10. 第一阶段开发顺序建议

最推荐你现在从这三个迭代开始：

```text
Iteration 01：项目骨架 + Qt UI + OpenGL 占位
Iteration 02：Conan + CMake + 基础依赖接入
Iteration 03：媒体信息读取
```

这三个完成后，项目就从"空 UI"变成了一个有工程骨架、有依赖管理、有 FFmpeg 接入、有 OpenGL 渲染占位、有媒体信息能力的播放器基础工程。

然后再进入真正的播放链路：

```text
Iteration 04：视频软解码 + OpenGL 渲染
Iteration 05：音频输出
Iteration 06：音视频同步
```

这个顺序比较稳，不容易让 AI 一开始把项目写成一个不可维护的大 Demo。
