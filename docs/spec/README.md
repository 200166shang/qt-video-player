PlayerLab Spec 索引

## 1. 目的

本目录包含 PlayerLab 的开发规范。

PlayerLab 是一个 PC 媒体播放器项目，技术栈：

- Qt
- FFmpeg
- OpenGL
- CMake
- Conan

项目通过 AI 辅助工作流（Claude Code / Codex）迭代开发。

本 README 定义：

- 如何阅读 spec
- 如何执行迭代
- 如何修改 spec
- 如何更新 roadmap / ADR / changelog

**Claude Code 执行任何任务前必须先阅读本文件。**

---

## 2. Spec 目录结构

```text
docs/spec/
├── README.md
├── 01-architecture.md
├── 02-roadmap.md
├── 03-adr.md
├── 04-changelog.md
└── iterations/
    ├── TEMPLATE.md
    ├── iter-xx-xxx.md
    └── iter-xxa-xxx.md
```

## 3. 文件职责

### README.md

spec 体系的入口文件。

定义：

- 阅读顺序
- 执行流程
- 更新流程
- spec 规则

### 01-architecture.md

定义架构边界与核心约束。

包含：

- 模块边界
- 依赖方向
- 禁止行为
- 核心抽象
- 线程原则

本文件极少变更。每次迭代前必须阅读。

### 02-roadmap.md

定义当前项目方向。

包含：

- 迭代顺序
- 高层目标
- 当前优先级

Roadmap 允许变更。不要把 roadmap 视为固定不变的长期承诺。

### 03-adr.md

ADR 即架构决策记录。

用于记录：

- 架构为何变更
- 为何新增依赖
- 为何技术路线变化
- 为何进行重构

重要的架构变更必须先更新 ADR。ADR 仅追加。

### 04-changelog.md

记录已完成的迭代结果。

包含：

- 实现了什么
- 变更了什么
- 修复了什么
- 已知限制

只有已完成的迭代才应出现在这里。

### iterations/TEMPLATE.md

所有未来迭代的模板。新迭代应从此模板创建。

### iterations/iter-xx-xxx.md

定义具体的实现任务。

包含：

- 目标
- 背景
- 范围
- 不在范围内
- 约束
- 任务
- 最小接口草案
- 验收标准
- 风险
- 完成记录

只有当前迭代需要详细展开，未来迭代可保持粗略。

### iterations/iter-xxa-xxx.md

定义已完成迭代之后插入的补充 / 重构迭代。

适用于：

- 对已完成能力做结构性重构
- 在不推翻 roadmap 主线的前提下插入专项优化
- 为后续迭代清理技术债

命名规则：

- 以最近一个已完成 iteration 为锚点
- 使用字母后缀表示插入顺序
- 示例：`iter-07a-av-sync-refactor-ffplay.md`

目的：

- 不重写历史 iteration 编号
- 不批量重命名未来 iteration 文件
- 在 changelog / roadmap / ADR 中保留稳定引用

---

## 4. 必要阅读顺序

执行任何迭代前，按顺序阅读：

1. `docs/spec/README.md`
2. `docs/spec/01-architecture.md`
3. `docs/spec/02-roadmap.md`
4. `docs/spec/iterations/` 下当前迭代的 spec

如果架构方向变更，阅读并更新：

- `docs/spec/03-adr.md`

迭代完成后，更新：

- `docs/spec/04-changelog.md`

---

## 5. 执行流程

每个迭代：

1. 阅读 spec 索引
2. 阅读架构契约
3. 阅读 roadmap
4. 阅读当前迭代 spec
5. 检查当前项目状态
6. 输出实现计划
7. 仅实现当前迭代
8. 构建项目
9. 验证验收标准
10. 更新 changelog

---

## 6. Spec 更新流程

### 6.1 仅当前任务发生变更

更新当前迭代 spec。

示例：

- 调整当前任务范围
- 简化当前实现
- 调整临时策略

### 6.2 未来迭代顺序变更

更新：

- `docs/spec/02-roadmap.md`

示例：

- 字幕迭代提前
- 网络播放延后
- 插入渲染器重构迭代

如果只是插入一个补充 / 重构 iteration，优先新增带字母后缀的 iteration 文件，而不是整体重编号。

示例：

- `iter-07a-av-sync-refactor-ffplay.md`
- roadmap 中新增 `Iteration 07A`

### 6.3 架构方向变更

先更新：

- `docs/spec/03-adr.md`

再根据需要更新：

- `docs/spec/01-architecture.md`
- `docs/spec/02-roadmap.md`
- 当前迭代 spec

示例：

- 渲染架构变更
- 替换音频后端
- 依赖管理策略变更
- 播放管线结构调整

### 6.4 迭代完成后

更新：

- `docs/spec/04-changelog.md`

记录：

- 实际实现
- 变更
- 修复
- 已知问题

---

## 7. AI 执行规则

Claude Code / Codex 必须遵守以下规则。

### 7.1 禁止提前实现未来迭代

除非当前迭代明确要求，否则不得添加高级功能。

未来功能示例：

- 硬件加速
- libass
- 高级渲染器
- 高级调度器
- 高级字幕系统
- 完整媒体库

### 7.2 保持架构边界清晰

禁止：

- 将播放逻辑写入 MainWindow
- 让 UI 包含 FFmpeg 头文件
- 让 UI 直接控制解码器
- 让 UI 直接操作 packet/frame 队列
- 将所有播放逻辑堆在一个大文件里

### 7.3 保持 OpenGL 为渲染主方向

允许调试路径。但最终渲染架构必须保留：

- 渲染器抽象
- OpenGL 渲染器
- 纹理上传通路
- shader 管线方向

### 7.4 保持迭代可构建

每个迭代结束时必须处于以下状态：

- 可编译
- 可运行
- 可验证

### 7.5 实现与 spec 冲突时

不要悄悄绕过 spec，而应：

1. 暂停实现
2. 说明冲突
3. 更新当前迭代 spec 或 ADR
4. 继续实现

---

## 8. 当前优先级

**当前活跃迭代：** Iteration 01 — Bootstrap UI + OpenGL Placeholder

**当前短期目标：**

- 建立稳定的项目结构
- 创建 Qt 主 UI
- 创建 OpenGL 视频占位控件
- 引入渲染器抽象
- 为 FFmpeg 集成做准备
