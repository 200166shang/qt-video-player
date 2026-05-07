# PlayerLab Roadmap

## 1. 目的

本文件定义 PlayerLab 的当前开发方向。

Roadmap 允许变更。

如果实际开发过程中发现：

- 迭代顺序不合理
- 复杂度超出预期
- 架构不匹配
- 依赖存在问题
- 性能瓶颈

可以更新 roadmap。

重大方向变更应记录到 ADR。

---

# 2. 迭代模板

## Iteration XX

### 目标

一句话目标。

### 重点

当前 iteration 的重点。

### 不做

当前 iteration 不做什么。

### 备注

可选备注。

---

# 3. 当前 Roadmap

## Iteration 01

Bootstrap UI + OpenGL Placeholder

**目标：**

建立 Qt 工程骨架、主窗口 UI、OpenGLVideoWidget 占位、renderer abstraction。

**重点：**

- Qt MainWindow
- Sidebar
- TopBar
- ControlBar
- MediaInfoPanel
- OpenGLVideoWidget
- IVideoRenderer
- OpenGLVideoRenderer 占位
- RendererFactory

**不做：**

- FFmpeg
- 实际播放
- 音频输出
- 网络播放

---

## Iteration 02

Conan + CMake + Dependencies

**目标：**

建立依赖管理与基础工程设施。

**重点：**

- conanfile.py
- CMake options
- FFmpeg
- spdlog
- fmt
- nlohmann_json
- stb
- Logger
- AppConfig

**不做：**

- 实际播放
- 解码
- AV sync

---

## Iteration 03

Media Info Reader

**目标：**

通过 FFmpeg 打开本地媒体并读取媒体信息。

**重点：**

- FFmpeg 初始化
- 打开本地文件
- 媒体信息
- 视频流信息
- 音频流信息
- UI 媒体信息展示

**不做：**

- 实际视频播放
- 音频播放
- seek

---

## Iteration 04

Video Decode + OpenGL Render

**目标：**

完成最小视频播放链路。

**重点：**

- 视频解码
- Packet 队列
- Frame 队列
- VideoFrame
- OpenGL 渲染
- 纹理上传
- Shader 管线
- 宽高比处理

**说明：**

允许先使用 RGB texture 路径快速验证。

后续逐步演进到：

- YUV420P 纹理
- NV12
- Shader 色域转换

---

## Iteration 05

Audio Decode + Audio Output

**目标：**

完成音频播放能力。

**重点：**

- 音频解码
- 重采样
- AudioFrame
- IAudioOutput
- QtAudioOutput
- 音量
- 静音
- 暂停/恢复

---

## Iteration 06

Basic AV Sync

**目标：**

实现基础音视频同步。

**重点：**

- PlaybackClock
- AVSynchronizer
- 音频主时钟
- 视频 PTS 调度
- 帧延迟/丢帧策略

---

## Iteration 07

Playback Control

**目标：**

补齐播放器基础控制能力。

**重点：**

- 播放
- 暂停
- 停止
- Seek
- 播放速度
- 时长
- 进度更新
- 播放状态机

---

## Iteration 08

Network Playback

**目标：**

支持网络媒体播放。

**重点：**

- 网络 MediaSource
- 打开 URL 对话框
- FFmpeg 网络初始化
- http/https
- hls/m3u8
- 超时处理
- UI 非阻塞打开

---

## Iteration 09

Playlist + Config Persistence

**目标：**

实现播放列表与配置持久化。

**重点：**

- 播放列表
- 最近打开
- 应用配置
- 播放历史
- 自动播放下一个

---

## Iteration 10

SRT Subtitle

**目标：**

支持外挂字幕。

**重点：**

- SRT 解析
- 字幕时间同步
- 字幕渲染
- 字幕延迟
- 基础字幕样式

**不做：**

- libass
- ass/ssa
- 内嵌字幕流选择

---

## Iteration 11

Player Tools

**目标：**

完善播放器工具能力。

**重点：**

- 全屏
- 截图
- 单帧步进
- 快捷键
- 播放工具

---

## Iteration 12

Performance + HWAccel Preparation

**目标：**

建立性能观测能力并为硬解码预留方向。

**重点：**

- FPS 统计
- 丢帧统计
- 渲染统计
- 解码模式抽象
- 硬解码占位

**不做：**

- 真正的 D3D11VA
- 真正的 VAAPI
- 真正的 CUDA 解码

---

## Iteration 13

Optional Extensions

**候选方向：**

- libass
- OpenAL
- OpenCV
- SQLite
- avfilter
- HDR
- 高级像素格式

**原则：**

默认关闭。按需接入。不污染核心播放链路。

---

## Iteration 14

Packaging

**目标：**

完成可分发桌面程序能力。

**重点：**

- Windows 打包
- Qt 运行时收集
- FFmpeg 运行时收集
- 默认配置
- README
- 构建说明

---

# 4. 当前优先级

当前优先：

1. Iteration 01
2. Iteration 02
3. Iteration 03

完成前三个 iteration 后，再根据代码状态细化 Iteration 04。
