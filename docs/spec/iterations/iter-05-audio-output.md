# Iteration 05：音频解码 + QtAudio 输出

## 目标

加入音频播放能力。

## 范围

实现：

```text
1. FFmpegAudioDecoder
2. FFmpegResampler
3. AudioFrame
4. QtAudioOutput
5. 音量控制
6. 静音控制
7. 暂停 / 恢复音频
```

## 初始输出格式

统一转换为：

```text
sample_rate: 48000
channels: stereo
sample_format: S16 或 Float
```

## 验收标准

```text
1. 本地视频可以播放声音。
2. 音量滑块有效。
3. 静音按钮有效。
4. 暂停时音频暂停。
5. 停止时音频停止。
```

