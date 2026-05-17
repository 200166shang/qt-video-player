# Iteration 06：音视频同步

## 目标

实现基础音视频同步。

## 说明

本 iteration 定义的是“最小可用同步”。

如果要继续向 ffplay 风格演进，例如：

- 精确定时唤醒
- 连续 delay 校正
- 音频输出延迟补偿

请参考后续补充迭代：

- `docs/spec/iterations/iter-07a-av-sync-refactor-ffplay.md`

## 同步策略

```text
1. 有音频时，音频作为主时钟。
2. 无音频时，系统时钟作为主时钟。
3. 视频根据 PTS 与主时钟差值决定等待、显示或丢帧。
```

## 核心计算

```text
diff = video_pts_ms - master_clock_ms

diff > threshold:
    视频太快，等待

diff < -threshold:
    视频太慢，丢帧或立即追赶

abs(diff) <= threshold:
    正常显示
```

## 新增模块

```text
PlaybackClock
AVSynchronizer
```

## 验收标准

```text
1. 普通 mp4 音画基本同步。
2. 暂停恢复后仍然同步。
3. Seek 后能重新同步。
4. 无音频视频也能正常播放。
```
