# Iteration 14：打包发布

## 目标

让 PlayerLab 变成可分发桌面程序。

## 范围

实现：

```text
1. Windows 打包
2. README 编译说明
3. README 使用说明
4. 运行时依赖收集
5. FFmpeg 动态库拷贝
6. Qt 插件拷贝
7. 默认配置生成
```

## 验收标准

```text
1. 新机器上可以运行打包产物。
2. 可以打开本地视频。
3. 可以播放网络 URL。
4. 没有明显缺失 dll / so / dylib。
5. README 可以指导用户完成构建和运行。
```

