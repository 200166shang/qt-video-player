# Iteration 10：SRT 字幕

## 目标

支持外挂 SRT 字幕。

## 范围

实现：

```text
1. SrtSubtitleParser
2. SubtitleTrack
3. 字幕按时间显示
4. 字幕字体大小设置
5. 字幕延迟设置
6. Seek 后字幕刷新
```

## 暂不实现

```text
1. ass/ssa 高级字幕
2. libass 渲染
3. 内嵌字幕流选择
```

## 验收标准

```text
1. 可以加载 .srt 文件。
2. 播放时字幕按时间显示。
3. Seek 后字幕正确变化。
4. 字幕延迟设置有效。
```

