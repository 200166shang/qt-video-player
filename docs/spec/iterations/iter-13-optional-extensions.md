# Iteration 13：可选扩展能力

## 目标

按需要引入更专业的音视频能力。

## 可选方向

```text
1. libass：高级字幕渲染
2. OpenAL Soft：替换或补充 QtAudioOutput
3. OpenCV：缩略图生成、帧分析、导出工具
4. SQLite：大型媒体库缓存
5. FFmpeg avfilter：滤镜、旋转、裁剪、画面调整
6. NV12/P010/HDR：高级像素格式和色彩空间
```

## 规则

```text
1. 所有扩展必须通过接口接入。
2. 不允许扩展功能污染核心播放链路。
3. 所有扩展默认通过 Conan option 控制。
4. 默认关闭非必要扩展。
```

