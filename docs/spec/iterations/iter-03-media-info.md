# Iteration 03：媒体信息读取

## 目标

使用 FFmpeg 打开本地媒体文件并读取媒体信息。

## 范围

实现：

```text
1. FFmpegGlobal 初始化
2. FFmpegDemuxer
3. MediaInfo 生成
4. 打开本地文件
5. 读取容器信息
6. 读取视频流信息
7. 读取音频流信息
8. 刷新右侧 MediaInfoPanel
```

## 关键流程

```text
UI 点击打开文件
  ↓
PlayerController::open(MediaSource)
  ↓
PlaybackEngine::open()
  ↓
FFmpegDemuxer::open()
  ↓
生成 MediaInfo
  ↓
mediaInfoChanged()
  ↓
UI 刷新
```

## 验收标准

```text
1. 可以选择本地 mp4/mkv/mov 文件。
2. 右侧可以显示文件格式、时长、分辨率、编码格式、帧率、音频采样率等。
3. 不支持的文件会显示错误。
4. 此阶段不要求播放画面。
```

