# Iteration 07B: Player Core Threading Refactor

## 1. Goal

在继续网络播放、字幕、硬解等功能迭代前，先重构播放器核心线程模型。

本 iteration 的一句话目标：

> UI 线程只负责界面与 OpenGL widget 更新，播放核心运行在独立 `PlayerCore` 线程内，并通过单一 read thread、音视频 decode thread、frame/audio queue 组织播放管线。

---

## 2. Background

当前项目已经完成基础播放、播放控制与一次 AV sync 重构：

- `PlayerController` 暴露 open/play/pause/seek/stop 等控制入口
- `PlaybackClock` 与 `AVSynchronizer` 已支持音频主时钟与视频调度
- `OpenGLVideoWidget` 已承担主线程 OpenGL 渲染
- `FFmpegVideoDecoder` 与 `FFmpegAudioDecoder` 已各自拥有 packet/frame queue

但当前播放管线仍存在结构性问题：

- `PlayerController` 仍运行在 UI 线程，并直接管理播放状态、pipeline 生命周期与 pump 逻辑
- `framePumpTimer_` 与 `audioPumpTimer_` 仍由 UI 线程驱动
- video/audio decoder 各自打开输入源，并各自执行 `av_read_frame`
- seek 仍依赖 pipeline teardown/restart，后续不利于演进到 flush-only / serial 机制
- 播放核心、音频消费、视频刷新职责仍集中在 `PlayerController`

因此，在 `Iteration 08: Network Playback` 前插入本重构迭代，先把播放器核心线程边界梳理清楚。

---

## 3. Scope

当前 iteration 负责：

- 新增 `PlayerCore`，承接播放状态机与 pipeline 生命周期
- 保留 `PlayerController` 作为 UI-facing facade
- 将 UI 到播放核心的控制调用改为异步命令派发
- 引入单一 read/demux worker，统一读取输入源并分发 packet
- 改造 video/audio decoder，使其只消费 packet queue 并输出 frame queue
- 将视频同步调度从 UI 线程迁移到 core/video refresh 侧
- 保持 OpenGL 纹理上传与 `paintGL()` 在主线程
- 保持音频作为 master clock，视频 refresh 追随 `PlaybackClock`
- 保证每个阶段可编译、可运行、可回归

---

## 4. Not In Scope

当前 iteration 明确不做：

- 不实现独立 OpenGL render thread
- 不做 shared OpenGL context / offscreen surface
- 不引入硬件解码
- 不实现字幕 pipeline
- 不实现完整 ffplay serial 机制
- 不实现 seek 后 decoder flush-only 流程
- 不做音频 sample 级补偿
- 不重写 UI 外观
- 不引入复杂 lock-free queue
- 不改变当前像素格式支持边界

本轮重点是线程边界与播放管线职责，不扩散到渲染架构、硬解或字幕系统。

---

## 5. Current Constraints

当前 iteration 的约束与临时策略：

- `QOpenGLWidget` 相关 OpenGL 操作继续留在主线程
- `PlayerController` 继续作为 UI 层唯一控制入口
- `PlayerCore` 优先使用 Qt event loop + `Qt::QueuedConnection` 作为命令队列
- packet/frame queue 继续使用 mutex + condition variable
- queue 应逐步具备有界能力，避免 read thread 无限堆积
- `PlaybackClock` 如果跨线程读写，必须补充线程安全策略
- 第一阶段允许 audio output 继续复用现有 `QtAudioOutput` pump/write 模型
- `QAudioSink::notify()` 驱动的独立 audio consumer 留给后续小步演进

---

## 6. Tasks

1. 新增 `PlayerCore`
2. 将 `PlayerController` 瘦身为 UI facade
3. 定义最小播放器命令模型：open/play/pause/stop/seek/volume/mute/rate
4. 新增单一 read/demux worker，统一 `av_read_frame`
5. read worker 按 stream index 分发到 `videoPacketQ` / `audioPacketQ`
6. 改造 `FFmpegVideoDecoder`，移除内部 demux thread 与 input ownership
7. 改造 `FFmpegAudioDecoder`，移除内部 demux thread 与 input ownership
8. 将视频 refresh / wait / drop / display 调度迁移出 UI 线程
9. 梳理 audio pump/write 与 audio clock update 的 core 侧职责
10. 统一 teardown / pause / seek 的队列 abort、clear、thread join 顺序
11. 增加 thread lifecycle、queue size、packet dispatch、decode drained、AV sync 调试日志
12. 更新 changelog，记录本轮重构的实际落地范围与遗留项

---

## 7. Minimal Interface Draft

当前 iteration 只引入最小必要接口，不提前设计完整最终模型。

```cpp
class PlayerController : public QObject {
    Q_OBJECT

public:
    void open(const QString& localFilePath);
    void play();
    void pause();
    void stop();
    void seek(double targetSec);

signals:
    void videoFrameReady(playerlab::core::VideoFrame frame);
    void playbackStateChanged(playerlab::core::PlayerController::PlaybackState state);
    void positionChanged(double currentSec, double durationSec);

private:
    QThread* coreThread_ = nullptr;
    PlayerCore* core_ = nullptr;
};
```

```cpp
class PlayerCore : public QObject {
    Q_OBJECT

public slots:
    void open(QString localFilePath);
    void play();
    void pause();
    void stop();
    void seek(double targetSec);

signals:
    void videoFrameReady(playerlab::core::VideoFrame frame);
    void playbackStateChanged(playerlab::core::PlayerController::PlaybackState state);
    void positionChanged(double currentSec, double durationSec);

private:
    bool startPipeline(double startPositionSec);
    void teardownPipeline();
};
```

```cpp
class FFmpegReadWorker {
public:
    bool open(const MediaSource& source, MediaInfo& outInfo, std::string& outError);
    void start();
    void stop();

private:
    void readLoop();
};
```

说明：

- read worker 只负责 demux
- decoder 只负责 packet -> frame
- core 只负责状态机、生命周期与同步协调
- UI 只负责用户输入、状态展示与主线程 OpenGL 渲染

---

## 8. Acceptance Criteria

本 iteration 的验收标准：

- 项目可编译
- 本地 mp4 可以正常 open/play/pause/resume/stop
- seek 后不死锁、不崩溃，能恢复播放
- UI 线程不再直接执行 FFmpeg read/decode
- video/audio 不再各自打开输入源并各自 `av_read_frame`
- 只存在一个 read/demux worker 负责读取媒体源
- video/audio decoder 只从 packet queue 消费
- video frame 通过 queued signal 回到主线程渲染
- OpenGL 渲染仍由主线程 `QOpenGLWidget` 完成
- 音频仍作为 master clock
- 视频同步策略不明显回退
- teardown 时所有工作线程能正常退出
- 现有基础 UI 控制能力不回退

---

## 9. Risks

- teardown 顺序不当可能导致 `waitPop` 阻塞或 thread join 死锁
- seek 清队列与 decoder flush 时机处理不好，可能显示旧帧
- `PlaybackClock` 跨线程读写若没有保护，可能引入数据竞争
- `QAudioSink` 线程亲和需要谨慎处理，本轮不宜同时大改
- 单 read thread 分发后，如果 queue 无界，高码率或网络流可能堆积内存
- video refresh 移出 UI 线程后，过高的 frame signal 频率可能压迫 UI event loop

---

## 10. Suggested Phasing

### Phase 1: PlayerCore Facade

- 新增 `PlayerCore`
- `PlayerController` 只做 UI facade
- 行为尽量保持接近当前实现

### Phase 2: Single Read Thread

- 引入单一 read worker
- 移除 audio/video decoder 内部 demux thread
- packet 分发到两条队列

### Phase 3: Decode Worker Cleanup

- decoder 职责收窄为 packet -> frame
- queue 生命周期由 `PlayerCore` 管理

### Phase 4: Video Refresh Migration

- 迁移 video sync scheduling
- UI 只接收 ready frame 并 render

### Phase 5: Audio Consumer Follow-up

- 梳理 audio pump/write 与 audio clock update
- 评估是否在后续 iteration 使用 `QAudioSink::notify()`

---

## 11. Completion Record

### Date

2026-05-20

### Summary

完成 `PlayerCore` 线程拆分、`PlayerController` facade 化、单一 `FFmpegReadWorker`、以及 audio/video decoder 的 packet-consumer 重构。

### Verification

- `cmake --build build -j4`
- `./bin/playerlab_decode_probe "testdata/雨爱 - 杨丞琳.mp4"`
- `./bin/playerlab` 启动日志 smoke test

### Notes

- 当前 seek 仍采用 pipeline teardown/restart，未进入 flush-only / serial 机制
- `QAudioSink::notify()` 驱动的独立 audio consumer 仍留待后续小步演进
- GUI 级 open/play/pause/resume/seek/stop 需补人工回归
