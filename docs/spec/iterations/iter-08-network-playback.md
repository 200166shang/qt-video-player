# Iteration 08：网络播放

## 目标

支持网络 URL 播放。

## 范围

支持：

```text
1. http/https mp4
2. hls/m3u8 初步支持
3. rtsp 作为可选验证
```

## 设计要求

```text
1. 本地文件和网络 URL 都通过 MediaSource 表达。
2. PlayerController 不区分本地和网络细节。
3. FFmpegDemuxer 内部处理 avformat_network_init。
4. 网络打开不能阻塞 UI。
5. 网络失败要有错误提示。
```

## 验收标准

```text
1. 输入 http mp4 URL 可以播放。
2. 输入错误 URL 有错误提示。
3. 网络打开过程中 UI 不冻结。
4. 可以停止网络播放。
```

