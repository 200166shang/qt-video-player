# Iteration 01：项目骨架 + Qt UI + OpenGL 占位

## 目标

建立项目基础结构，完成 UI 原型落地，并从第一版建立 OpenGL 渲染抽象。

## 范围

实现：

```text
1. CMake 项目骨架
2. Qt MainWindow
3. 左侧 Sidebar
4. 顶部 TopBar
5. 中央 OpenGLVideoWidget
6. 下方 MediaLibraryWidget
7. 右侧 MediaInfoPanel
8. 底部 ControlBar
9. IVideoRenderer 接口
10. OpenGLVideoRenderer 空实现
11. DebugImageRenderer 空实现
12. RendererFactory
13. shaders/ 目录
```

暂不实现：

```text
1. FFmpeg 解码
2. 实际播放
3. 音频输出
4. 网络播放
```

## 验收标准

```text
1. 项目可以编译运行。
2. 主窗口布局接近 UI 原型。
3. 中央区域使用 QOpenGLWidget，而不是 QLabel。
4. OpenGLVideoWidget 可以显示纯色背景。
5. 播放按钮、打开按钮、进度条点击不崩溃。
6. UI 层没有 include FFmpeg 头文件。
```

