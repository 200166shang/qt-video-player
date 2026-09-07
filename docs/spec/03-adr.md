# PlayerLab ADR

ADR = Architecture Decision Record

本文档用于记录：

- 为什么改变架构方向
- 为什么调整技术路线
- 为什么引入或放弃某个依赖
- 为什么修改核心模块边界

规则：

1. ADR 只追加。
2. 不轻易删除旧 ADR。
3. 如果旧方案废弃，可以标记 Deprecated。
4. 重要架构变化必须先写 ADR，再改 spec。

---

# ADR Template

## ADR-XXXX: Title

### Status

Proposed / Accepted / Deprecated

### Context

为什么需要这个决策？

### Decision

决定怎么做？

### Consequences

会影响哪些模块、迭代或接口？

### Alternatives Considered

考虑过哪些方案？为什么没采用？

### Follow-up

后续需要更新哪些 spec、roadmap 或代码？

---

## ADR-0001: AV Sync 采用渐进式 ffplay 对齐策略

### Status

Accepted

### Context

PlayerLab 在 `Iteration 06` 已具备最小可用的音视频同步能力，但当前实现仍以固定轮询和硬阈值决策为主：

- 视频 Wait 路径依赖固定 tick，调度精度有限
- Wait / Display / Drop 是离散决策，边界附近容易抖动
- 音频时钟未扣除输出缓冲延迟，可能导致视频系统性偏早

另一方面，当前项目仍处于可持续迭代阶段，不适合一次性引入 ffplay 全量复杂度。

### Decision

AV sync 重构采用“渐进式对齐 ffplay”的策略：

1. 保留当前主时钟架构与模块边界
2. 优先引入精确定时视频唤醒
3. 再将视频同步从硬阈值动作改为连续 delay 校正
4. 补充音频输出延迟补偿
5. 暂不引入 serial、flush-only seek、音频 sample 级补偿

### Consequences

- 需要新增一个补充重构 iteration，而不是回写 `Iteration 06`
- `AVSynchronizer` 与 `PlayerController` 会发生职责演进，但整体边界保持不变
- `IAudioOutput` 需要新增延迟查询能力
- roadmap 需插入一个专门的 AV sync refactor iteration

### Alternatives Considered

#### 方案 A：保持当前基础同步实现不动

优点：

- 改动最小

缺点：

- 无法解决视频调度精度与同步抖动问题

#### 方案 B：一次性完整照搬 ffplay 时钟与 serial 体系

优点：

- 理论上最接近参考实现

缺点：

- 改动面过大
- 与当前项目阶段不匹配
- 回归成本高

#### 方案 C：仅修改阈值，不调整调度模型

优点：

- 实现简单

缺点：

- 不能从根本上解决固定轮询带来的时序误差

### Follow-up

需要更新：

- `docs/spec/02-roadmap.md`
- `docs/spec/iterations/iter-07a-av-sync-refactor-ffplay.md`
- 相关实现文件与完成后的 changelog

---

## ADR-0002: 插入 Player Core 线程模型重构

### Status

Accepted

### Context

PlayerLab 当前已经具备基础播放、播放控制与 AV sync 能力，但播放核心仍与 UI 线程耦合较深：

- `PlayerController` 直接管理 pipeline 生命周期、音频 pump、视频 frame pump 与状态机
- video/audio decoder 各自打开输入源，并各自执行 `av_read_frame`
- UI 线程仍通过 timer 驱动部分播放核心逻辑
- 后续网络播放、字幕同步、硬解、flush-only seek 会放大这些结构问题

如果继续直接推进功能迭代，后续功能会更容易堆叠在当前 `PlayerController` 上，增加回归与重构成本。

### Decision

在 `Iteration 08: Network Playback` 前插入 `Iteration 07B: Player Core Threading Refactor`。

该重构采用渐进方式：

1. 保留 `PlayerController` 作为 UI-facing facade
2. 新增独立 `PlayerCore`，承接播放状态机与 pipeline 生命周期
3. 使用 Qt queued connection / event loop 作为 UI 到 core 的命令队列
4. 引入单一 read/demux worker，统一读取输入源并分发 packet
5. 将 video/audio decoder 收窄为 packet queue 到 frame queue 的 worker
6. OpenGL 渲染暂时继续留在主线程 `QOpenGLWidget`
7. 独立 OpenGL render thread、serial、flush-only seek、audio sample compensation 留给后续迭代

### Consequences

- roadmap 需要插入一个补充重构 iteration
- `PlayerController` 职责会明显变薄
- FFmpeg read/demux ownership 会从 decoder 内迁移到 read worker / core 管理
- decoder 接口会发生结构性变化
- queue 生命周期、teardown 顺序、seek reset 策略需要重新梳理
- 后续 `Iteration 08: Network Playback` 可以基于更清晰的 core/read/decode 边界实现

### Alternatives Considered

#### 方案 A：直接继续做 Network Playback

优点：

- 短期功能推进最快

缺点：

- 网络超时、阻塞 read、reconnect、HLS 等问题会直接压到当前 UI-thread-heavy 架构上
- 后续回头重构成本更高

#### 方案 B：一次性实现完整 ffplay 风格管线

优点：

- 长期结构最完整

缺点：

- 改动过大
- 容易同时引入 serial、flush、audio compensation、独立 render thread 等多个风险点
- 不符合当前项目的小步可验证节奏

#### 方案 C：只把 `PlayerController` moveToThread

优点：

- 改动较小

缺点：

- 无法解决双 demux、decoder ownership、queue 生命周期等核心问题
- 只是移动线程位置，不是真正梳理播放管线职责

### Follow-up

需要更新：

- `docs/spec/02-roadmap.md`
- `docs/spec/iterations/iter-07b-player-core-threading-refactor.md`
- 完成实现后更新 `docs/spec/04-changelog.md`

---

## ADR-0003: 插入 ffplay 风格 Seek 重构迭代

### Status

Accepted

### Context

`Iteration 07B` 完成后，PlayerLab 的播放线程边界已经比早期清晰很多，但 seek 仍然沿用临时策略：

- `PlayerCore::seek()` 通过 `startPipeline()` 触发 `teardownPipeline() + reopen`
- `FFmpegReadWorker` 只在 open 阶段通过 `startPositionSec` 执行一次 seek
- decoder 把当前 flush packet 当作结束信号，seek 后依赖线程重建
- packet、frame、audio buffer 没有 serial/epoch 机制来系统性淘汰旧数据

这导致 seek 的结构语义与 ffplay 相差很远，也会直接阻碍后续网络播放、字幕、EOF seek、连续 seek 等场景。

### Decision

在 `Iteration 08: Network Playback` 前插入 `Iteration 07C: ffplay-style Seek Refactor`。

该重构采用“对齐语义骨架，而非一次性照搬全部实现”的策略：

1. seek 请求只负责描述目标位置，不再直接触发 pipeline restart
2. seek 由 read thread 在运行中执行，采用 `avformat_seek_file(...)`
3. queue 控制语义拆分为 data / flush / eof
4. 引入 packet serial、decoder serial、frame serial，建立 seek epoch
5. decoder 在 seek flush 后执行 `avcodec_flush_buffers(...)` 并继续存活
6. `PlayerCore` 在 seek 后定向重置 pending frame、audio buffer 与 clock
7. 暂不完整复制 ffplay 全部 `VideoState`、subtitle seek、sample 级音频补偿

### Consequences

- roadmap 需要插入一个新的补充重构 iteration
- `FFmpegReadWorker`、`PacketQueue`、decoder、`PlayerCore` 的接口都会发生结构性调整
- 当前“flush packet 等于 decoder 结束”的临时约定会被废弃
- `VideoFrame` / `AudioFrame` 需要具备 serial 概念
- 后续网络播放、字幕同步可复用更正确的 seek 语义基础

### Alternatives Considered

#### 方案 A：继续保留 teardown/restart seek

优点：

- 当前改动最小

缺点：

- seek/open 生命周期继续耦合
- 连续 seek、paused seek、EOF seek 语义都不稳
- 后续网络播放与字幕只会继续叠加复杂度

#### 方案 B：一次性完整复制 ffplay seek 与全部相关内部结构

优点：

- 理论上最接近参考实现

缺点：

- 改动面过大
- 会把 subtitle、外部时钟、完整 frame queue 语义等问题一次性卷进来
- 不符合当前项目小步验证节奏

#### 方案 C：只把 `av_seek_frame(...)` 改成 `avformat_seek_file(...)`

优点：

- 表面上更接近 ffplay

缺点：

- 如果没有 flush/eof 拆分与 serial 机制，seek 语义仍然是错的
- 无法解决旧帧、旧音频、旧 epoch 穿透问题

### Follow-up

需要更新：

- `docs/spec/02-roadmap.md`
- `docs/spec/iterations/iter-07c-seek-refactor-ffplay.md`
- 完成实现后更新 `docs/spec/04-changelog.md`
