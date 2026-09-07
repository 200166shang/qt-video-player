---
status: accepted
---

# 通过合成引擎共享预览与导出语义

PlayerLab 以不读取墙钟的 Composition Engine（合成引擎）作为 Preview 与 Baseline Export 的共享边界：它依据不可变工程状态、精确 Editing Time 或时间区间及输出规格，产生编码前画面、字幕、PCM 或结构化错误。Preview 由实时调度器按音频时钟推进并可跳过迟到的视频呈现，Baseline Export 则由离线调度器完整遍历恒定帧率网格与音频区间；两者共享 Render Semantics，而不要求调度、硬件路径或最终编码字节相同。这样既能以软件参考路径和明确容差验证确定性，又允许平台硬件加速、语义透明缓存与等价的软件回退。
