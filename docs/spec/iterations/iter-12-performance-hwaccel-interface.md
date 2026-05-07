# Iteration 12：性能统计 + 硬解码接口预留

## 目标

建立性能观测能力，并为硬解码做架构准备。

## 范围

实现：

```text
1. 解码 FPS 统计
2. 渲染 FPS 统计
3. 音频缓冲状态统计
4. 视频丢帧统计
5. 当前渲染后端显示
6. 当前解码模式显示
7. DecodeMode 配置项
8. 硬解码接口占位
```

## 暂不实现

```text
1. D3D11VA 真正硬解
2. VAAPI 真正硬解
3. VideoToolbox 真正硬解
4. CUDA/NVDEC 真正硬解
```

## 验收标准

```text
1. UI 可以显示 FPS、丢帧、缓冲等信息。
2. 配置里可以选择 decode mode。
3. 当前默认仍然使用软件解码。
4. 后续实现硬解不需要重写 PlayerController。
```

