# Iteration 04：视频软解码 + OpenGL 渲染

## 目标

完成最小视频播放链路：FFmpeg 解码视频帧，OpenGL 渲染显示。

## 范围

实现：

```text
1. FFmpegVideoDecoder
2. PacketQueue
3. FrameQueue
4. 解码线程
5. VideoFrame 数据转换
6. YUV420P OpenGL 三纹理上传
7. YUV -> RGB fragment shader
8. 保持视频宽高比显示
```

## 推荐第一版格式

优先支持：

```text
YUV420P
```

后续支持：

```text
NV12
RGB24
RGBA
```

## 验收标准

```text
1. 本地 mp4 可以显示动态画面。
2. 画面通过 OpenGL 渲染。
3. UI 不明显卡死。
4. stop 后解码线程安全退出。
5. 切换文件不会崩溃。
```

