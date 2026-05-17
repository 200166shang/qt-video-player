# Iteration 07A：AV Sync Refactor toward ffplay

## 1. Goal

在保持当前播放能力可用的前提下，将基础音视频同步策略演进为更接近 ffplay 的定时与校正模型，优先解决视频调度精度和同步抖动问题。

---

## 2. Background

`iter-06-av-sync.md` 已实现最小可用的 AV sync：

- 有音频时使用音频主时钟
- 无音频时使用系统时钟
- 视频基于 `video_pts - master_clock` 做 Wait / Display / Drop 决策

该方案适合作为起点，但与 ffplay 相比仍有明显差距：

- 视频侧采用固定 15ms 轮询，Wait 后不能精确定时唤醒
- 同步决策采用硬阈值，容易在边界附近震荡
- 音频时钟未扣除输出缓冲延迟，视频可能系统性偏早
- seek 后仍主要依赖 pipeline teardown/reset，而不是更细粒度的过期状态管理

本 iteration 不是重写播放架构，而是在当前 `PlaybackClock + AVSynchronizer + PlayerController` 基础上，进行一次受控的同步重构。

重构目标遵循以下优先级：

1. 先提升视频定时精度
2. 再把硬阈值决策演进为连续延迟校正
3. 最后补上音频输出延迟补偿

---

## 3. Scope

当前 iteration 负责：

- 把视频 Wait 路径从固定轮询改为一次性精确定时唤醒
- 在 `PlayerController` 中引入连续的视频显示时间线概念（类似 `frame_timer`）
- 把 `AVSynchronizer` 从三元动作决策演进为“基于 diff 调整 delay”的策略
- 为音频时钟补充输出缓冲区延迟补偿接口
- 补充文档，明确当前实现与 ffplay 的对齐边界

---

## 4. Not In Scope

当前 iteration 明确不做：

- 完整复制 ffplay 的所有内部结构
- 引入 packet serial / frame serial 机制
- seek 后改为 decoder flush-only 流程
- 音频 sample 级动态拉伸 / 压缩（`swr_set_compensation` 一类能力）
- 外部时钟 / 视频主时钟模式
- 大规模重写解码线程模型或 queue 模型

这一轮只解决“当前同步模型最影响体验的部分”，不把重构范围扩散到整个播放器核心。

---

## 5. Current Constraints

当前 iteration 的约束与临时策略：

- 继续保留“音频优先、无音频则系统时钟”的主时钟策略
- 继续保留当前 seek 后的 pipeline reset 流程
- `PlaybackClock` 的 anchor 模型暂不改为 ffplay 的 `pts_drift` 表达；两者数学等价，当前不为表达形式做重构
- 如果连续 delay 校正与现有代码耦合过深，允许分两步落地：
  - 第一步只上精确定时唤醒
  - 第二步再替换 AVSynchronizer 策略
- 所有改动都必须保持每个阶段可编译、可运行、可回归验证

---

## 6. Tasks

1. 记录当前 PlayerLab 与 ffplay 同步模型的差异，作为本 iteration 的设计基线
2. 修改 `PlayerController::onFramePump`，让 Wait 路径使用 `waitMs` 做一次性唤醒，而不是继续依赖 15ms 轮询
3. 为视频调度引入连续时间线状态，例如 `frameTimer_ / frameDelay_`
4. 重构 `AVSynchronizer`，从 `Wait / Display / Drop` 输出改为目标 delay 计算
5. 在 `IAudioOutput` 增加输出延迟查询接口，并在 `QtAudioOutput` 中实现
6. 在音频时钟更新时扣除 output latency，减小视频系统性超前
7. 为关键同步路径补充可观测日志或调试信息，便于判断 diff、delay、drop、late frame 情况
8. 更新 spec / ADR，明确 serial 机制属于后续而非本 iteration 范围

---

## 7. Minimal Interface Draft

当前 iteration 只引入最小必要接口，不提前设计完整最终模型。

可能涉及的最小接口方向：

```cpp
class AVSynchronizer {
public:
    double computeTargetDelay(double baseDelaySec,
                              double videoPtsSec,
                              double masterClockSec) const;
};
```

```cpp
class IAudioOutput {
public:
    virtual std::optional<double> playedSeconds() const = 0;
    virtual double outputLatencySeconds() const = 0;
};
```

`PlayerController` 侧允许新增最小状态：

```cpp
double frameTimerSec_ = 0.0;
double lastFrameDurationSec_ = 0.0;
bool frameWakePending_ = false;
```

说明：

- `frameTimerSec_` 表示视频显示时间线，而不是主时钟镜像
- `lastFrameDurationSec_` 用于计算下一帧基础 delay
- 命名可以调整，但职责应保持稳定

---

## 8. Acceptance Criteria

本 iteration 的验收标准：

- 普通 mp4 播放时，视频显示不再被固定 15ms tick 明显限制
- 同步边界附近的 Wait / Display 抖动明显减少
- 典型 24fps / 30fps 内容下，视频显示节奏比当前实现更平滑
- 音频输出存在硬件缓冲时，视频系统性超前现象有所收敛
- pause / resume / stop / seek 后不引入新的卡死、重复显示或明显失同步问题
- 项目可编译，现有播放控制能力不回退

如果本 iteration 分阶段落地，则至少应先满足：

- 精确定时唤醒可用
- 基础回归播放正常

---

## 9. Risks

- `QTimer::singleShot` 的实际精度仍受 Qt 事件循环与平台调度影响，收益可能小于理论值
- `frame_timer` 风格时间线若与当前 pause/seek 状态机耦合处理不当，可能引入重复帧或跳时钟
- 音频输出延迟接口在不同平台上的可解释性可能不一致，需要文档说明其“近似值”属性
- 如果同时改定时、校正、延迟补偿，回归定位会变难，因此应分提交或至少分阶段验证

---

## 10. Completion Record

### Date

YYYY-MM-DD

### Summary

一句话总结完成内容。

### Verification

确认验收标准全部满足。

### Notes

若后续继续向 ffplay 靠拢，下一步优先考虑：

- serial 机制
- 音频 sample 补偿
- 更细粒度的调试统计
