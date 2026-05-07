# Iteration XX: Title

## 1. Goal

本 iteration 的一句话目标。

例如：

- 建立 OpenGL 渲染占位
- 完成视频解码链路
- 实现音视频同步
- 接入网络播放

---

## 2. Background

为什么需要这个 iteration？

当前项目状态是什么？

这个 iteration 在 roadmap 中处于什么位置？

当前 iteration 想解决什么问题？

---

## 3. Scope

当前 iteration 负责：

- xxx
- xxx
- xxx

这里只写本 iteration 真正负责的事情。

---

## 4. Not In Scope

当前 iteration 明确不做：

- xxx
- xxx
- xxx

这一节非常重要。

用于防止 Claude Code 提前实现未来功能。

例如：

- 不做 hwaccel
- 不做 libass
- 不做完整 subtitle system
- 不做复杂 packet scheduler

---

## 5. Current Constraints

当前 iteration 的约束与临时策略。

例如：

- 当前允许使用 RGB texture path
- 当前不实现完整 YUV420P shader pipeline
- 当前先使用 software decode
- 当前先使用 QtAudioOutput
- 当前先不处理 HDR

如果后续需要重构，可以在未来 iteration 调整。

---

## 6. Tasks

当前 iteration 的具体任务。

示例：

1. 创建 OpenGLVideoWidget
2. 创建 IVideoRenderer
3. 创建 OpenGLVideoRenderer
4. 创建 RendererFactory
5. 建立 UI layout
6. 添加 OpenGL placeholder render

---

## 7. Minimal Interface Draft

当前 iteration 需要的最小接口草案。

只写：

- 当前真的需要的抽象
- 当前真的需要的方法
- 当前真的需要的数据结构

不要提前设计完整最终接口。

允许后续修改。

---

## 8. Acceptance Criteria

本 iteration 的验收标准。

例如：

- 项目可编译
- OpenGLVideoWidget 显示纯色
- Play 按钮点击不崩溃
- UI 层没有 FFmpeg 头文件

---

## 9. Risks

实现当前 iteration 的风险。

例如：

- Qt OpenGLWidget 在部分平台可能不支持多 context
- 后续如果引入 YUV420P，shader 需要重写

---

## 10. Completion Record

本 iteration 完成后填写。

### Date

YYYY-MM-DD

### Summary

一句话总结完成内容。

### Verification

确认验收标准全部满足。

### Notes

后续 iteration 需要注意的问题。
