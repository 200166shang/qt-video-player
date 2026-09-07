# Iteration 07C: ffplay-style Seek Refactor

## 1. Goal

将当前 seek 从 `pipeline teardown/restart` 重构为更接近 ffplay 的运行时 seek 流程，使 seek 成为播放器内部的一次状态切换，而不是一次“重新打开播放器”。

---

## 2. Background

`Iteration 07B` 完成后，PlayerLab 已具备较清晰的 `PlayerCore + read worker + audio/video decoder` 多线程播放管线，但 seek 仍然明显不对。

当前实现的核心问题：

- `PlayerCore::seek()` 直接调用 `startPipeline(...)`，先 `teardownPipeline()`，再重新 `open/start` 整条链路
- `FFmpegReadWorker` 只在 `open(..., startPositionSec)` 阶段执行一次 `av_seek_frame(...)`
- decoder 收到当前的 flush packet 后会 `break` 退出线程，flush 语义仍然等同于“结束”
- seek 过程中没有 packet serial / frame serial，旧 epoch 的数据无法被系统性淘汰
- 音频输出缓冲与 `pendingVideoFrame_` 只是在 restart 中被粗暴重置，行为依赖 teardown 时序

这也是为什么当前 seek “看起来能跳”，但语义并不正确：

- seek 行为和 open 行为耦合在一起
- 暂停态 seek、连续 seek、接近 EOF seek 都容易出现时钟错乱或旧帧残留
- 后续若继续做网络播放、字幕、硬解，restart 式 seek 会把复杂度继续放大

ffplay 的 seek 参考基线不是“重新 open 输入源”，而是：

- UI/控制层只设置 `seek_req / seek_pos / seek_rel / seek_flags`
- read thread 在运行中执行 `avformat_seek_file(...)`
- seek 成功后 flush 各 packet queue，并向 decoder 发送 flush 控制包
- decoder 执行 `avcodec_flush_buffers(...)`，但线程不退出
- packet / frame 挂载 serial，新旧 epoch 数据通过 serial 自动淘汰
- clock 在 seek 后对齐到新的目标时间，播放管线继续运行

这次 iteration 的目标不是 100% 复制 ffplay 的所有结构，而是把 seek 的语义骨架改正确。

---

## 3. Scope

当前 iteration 负责：

- 为 seek 引入独立的运行时请求路径，而不是复用 `startPipeline()`
- 让 `FFmpegReadWorker` 在 read thread 内处理 seek 请求
- 将 `av_seek_frame(...)` 切换为更接近 ffplay 的 `avformat_seek_file(...)` 语义
- 重构 queue 控制协议，区分 data / flush / eof，而不是复用单一种类的空 packet
- 引入 packet serial、decoder serial、frame serial，建立 seek epoch
- seek 后对 video pending frame、audio output buffer、playback clock 做定向重置
- 保持 decoder thread 常驻，seek 时执行 codec flush 而不是退出重建
- 为暂停态 seek 提供正确行为：保留 paused 状态，并能在新位置准备/显示首帧
- 为连续 seek 提供 coalescing 策略：同一时刻只处理最新的 seek 请求
- 补充日志与可观测性，便于验证 seek 前后各线程、queue、serial 与 clock 行为

---

## 4. Not In Scope

当前 iteration 明确不做：

- 不完整复制 ffplay 的全部 `VideoState` 结构
- 不引入字幕 packet/frame queue 与 subtitle seek 刷新
- 不实现音频 sample 级 time-stretch / shrink
- 不引入独立 OpenGL render thread
- 不在本轮同时重写 AV sync 主体算法
- 不处理硬解码器的 seek 特殊兼容逻辑
- 不在本轮引入 chapter seek、byte seek UI、A-B loop 等高级 seek 功能
- 不重写现有播放速度控制策略

本轮只解决“seek 语义与结构正确”这一件事。

---

## 5. Current Constraints

当前 iteration 的约束与临时策略：

- `PlayerController` 继续作为 UI 唯一入口，UI 层只发起 `seek(targetSec)`
- `PlayerCore` 继续运行在独立线程，通过 event loop 接收控制命令
- OpenGL 渲染继续留在主线程；seek 后的视频首帧仍通过现有 `videoFrameReady` 路径回主线程显示
- `QtAudioOutput` 允许继续使用 stop/reset/reopen 作为“清空旧音频缓冲”的实现手段，不强求一步到位实现底层非破坏性 flush
- queue 仍使用 mutex + condition variable，不引入 lock-free 结构
- seek 目标仍以时间 seek 为主，先不暴露 byte seek 到 UI
- 对于没有精确索引的媒体，允许 seek 后通过解码自然回落到最近关键帧附近，再由后续解码推进到目标时间

---

## 6. Tasks

1. 为 `PlayerCore` 新增独立 seek 命令路径，去掉 seek 对 `startPipeline()` 的直接依赖
2. 在 `FFmpegReadWorker` 中增加运行时 seek 请求状态：`seekReq / seekTarget / seekRel / seekFlags`
3. 为 read thread 增加被 seek 唤醒的等待机制，避免只能靠重新 open 生效
4. 将 read worker 的 seek 执行切换为 `avformat_seek_file(...)`
5. 重构 packet queue item，显式区分 data / flush / eof 三种控制语义
6. 在 queue 或 packet item 上增加 serial，seek 时 bump serial
7. 改造 `FFmpegVideoDecoder`，flush 时执行 `avcodec_flush_buffers(...)` 并继续解码
8. 改造 `FFmpegAudioDecoder`，flush 时执行 `avcodec_flush_buffers(...)` 并继续解码
9. 为 `VideoFrame` / `AudioFrame` 增加 serial 字段
10. 在 `PlayerCore` 中只消费当前 serial 的 frame，丢弃旧 serial 的 frame
11. seek 时清空 `pendingVideoFrame_`，重置 `frame_timer` 与相关视频时间线状态
12. seek 时重置音频输出缓冲与首帧 PTS 基线，避免旧 PCM 残留
13. seek 时重建 `PlaybackClock` 锚点，使 position/AV sync 从新 epoch 重新开始
14. 明确 paused seek 语义：seek 后保持暂停，并尝试推进到目标附近的首帧预览
15. 增加 seek 请求、seek 执行、serial 变化、flush/eof、旧帧丢弃、clock 重建日志
16. 更新 ADR / roadmap；实现完成后再更新 changelog

---

## 7. Minimal Interface Draft

当前 iteration 只引入 seek 重构所需的最小接口草案。

```cpp
enum class QueuedPacketKind {
    Data,
    Flush,
    Eof,
};

struct QueuedPacket {
    AVPacket* packet = nullptr;
    QueuedPacketKind kind = QueuedPacketKind::Data;
    int serial = 0;
};
```

```cpp
struct SeekRequest {
    double targetSec = 0.0;
    double relSec = 0.0;
    int flags = 0;
};
```

```cpp
class FFmpegReadWorker {
public:
    bool open(const MediaSource& source, MediaInfo& outInfo, std::string& outError);
    void start(PacketQueue<QueuedPacket>* videoPacketQueue,
               PacketQueue<QueuedPacket>* audioPacketQueue);
    void stop();

    int requestSeek(const SeekRequest& request);
    [[nodiscard]] int currentSerial() const;

private:
    void readLoop();
    bool performSeek();
};
```

```cpp
struct VideoFrame {
    int width = 0;
    int height = 0;
    double ptsSec = 0.0;
    int serial = 0;
    ...
};

struct AudioFrame {
    double ptsSec = 0.0;
    int serial = 0;
    ...
};
```

```cpp
class PlayerCore : public QObject {
    Q_OBJECT

public slots:
    void seek(double targetSec);

private:
    void beginSeek(double targetSec);
    void finishSeekAnchor(double targetSec, int serial);
    bool isFrameObsolete(int frameSerial) const;
};
```

说明：

- seek 请求只描述“要跳到哪里”，不再承担 pipeline 重建职责
- read worker 是 seek 真正的执行者
- queue item 和 frame 都带 serial，用于淘汰旧 epoch 数据
- decoder 在 flush 后继续活着，只有 stop 时才真正退出

接口名允许在实现时微调，但职责边界不要回退。

---

## 8. Acceptance Criteria

本 iteration 的验收标准：

- 项目可编译
- 本地媒体播放中 seek 不再触发整条 pipeline teardown/restart
- read worker 能在运行中处理 seek 请求
- decoder 在 seek 后不会因为 flush 直接退出线程
- seek 后旧 packet、旧 frame、旧音频缓冲不会继续被消费或播放
- `positionChanged` 在 seek 后能稳定落到新的时间区域，不出现明显回跳
- 播放中 seek 后能恢复连续播放
- 暂停态 seek 后保持暂停，并显示或准备目标附近的首帧
- 连续快速 seek 时，最终落点以最后一次请求为准
- 接近 EOF seek、从 EOF seek 回中间位置都不死锁、不黑屏、不残留旧音频
- 现有 play/pause/stop 基本行为不回退

---

## 9. Risks

- queue control item 与现有 `PacketQueue<AVPacket*>` 差异较大，改造面会跨 read/decode/core 多处
- 如果 flush / eof 语义拆分不彻底，decoder 可能在 seek 后错误进入 drained 状态
- `QtAudioOutput` 若无法可靠清空旧缓冲，seek 后可能短暂串出旧音频
- paused seek 若只重置 clock 但不主动推进首帧，用户会感觉 seek 没生效
- serial 传播链只要漏掉一个环节，就可能出现旧帧穿透到新 epoch
- 后续字幕与网络播放会复用这套 seek 语义，因此本轮接口命名与职责边界要保持克制

---

## 10. Suggested Phasing

### Phase 1: Seek Protocol

- 增加 seek request 状态
- read thread 支持运行时 `avformat_seek_file(...)`
- 将 flush/eof 语义从“空包”中拆开

### Phase 2: Serial Pipeline

- packet queue、decoder、frame 全链路携带 serial
- seek 时 bump serial，旧数据自动失效

### Phase 3: Decoder Persistence

- decoder flush 后不退出线程
- eof 只表示当前 serial 读尽，不表示 decoder 生命周期结束

### Phase 4: Core State Reset

- seek 时定向重置 `pendingVideoFrame_`、audio buffer、clock、frame timeline
- 保证 playing seek / paused seek 都符合预期

### Phase 5: Validation

- 做连续 seek、EOF seek、暂停态 seek、音视频联合回归
- 根据日志确认 serial、clock、queue 状态符合设计

---

## 11. Completion Record

### Date

2026-05-21

### Summary

完成 ffplay-style seek 的最小语义骨架：read thread 运行时 seek、`Flush/Eof` 控制协议、serial epoch 传播、decoder 常驻，以及 `PlayerCore` 侧的 seek 定向重置。

### Verification

- `cmake --build build -j4`
- `./bin/playerlab_decode_probe "testdata/雨爱 - 杨丞琳.mp4"`

### Notes

- seek 已不再直接依赖 `startPipeline()` 重启整条链路
- `FFmpegReadWorker` 现在在 read thread 内执行 `avformat_seek_file(...)`，并通过 `Flush/Eof + serial` 驱动 decoder 与 frame 失效
- `PlayerCore` 已接入 serial 过滤与 paused seek 预览路径，但 GUI 层 `paused seek / EOF seek / 连续 seek` 仍建议补手工回归
