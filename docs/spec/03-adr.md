# PlayerLab ADR

ADR = Architecture Decision Record

本文档用于记录：

- 为什么改变架构方向
- 为什么调整技术路线
- 为什么引入或放弃某个依赖
- 为什么修改核心模块边界

规则：

1. ADR 只追加。
2. 不轻易删除旧 ADR。
3. 如果旧方案废弃，可以标记 Deprecated。
4. 重要架构变化必须先写 ADR，再改 spec。

---

# ADR Template

## ADR-XXXX: Title

### Status

Proposed / Accepted / Deprecated

### Context

为什么需要这个决策？

### Decision

决定怎么做？

### Consequences

会影响哪些模块、迭代或接口？

### Alternatives Considered

考虑过哪些方案？为什么没采用？

### Follow-up

后续需要更新哪些 spec、roadmap 或代码？

---

## ADR-0001: AV Sync 采用渐进式 ffplay 对齐策略

### Status

Accepted

### Context

PlayerLab 在 `Iteration 06` 已具备最小可用的音视频同步能力，但当前实现仍以固定轮询和硬阈值决策为主：

- 视频 Wait 路径依赖固定 tick，调度精度有限
- Wait / Display / Drop 是离散决策，边界附近容易抖动
- 音频时钟未扣除输出缓冲延迟，可能导致视频系统性偏早

另一方面，当前项目仍处于可持续迭代阶段，不适合一次性引入 ffplay 全量复杂度。

### Decision

AV sync 重构采用“渐进式对齐 ffplay”的策略：

1. 保留当前主时钟架构与模块边界
2. 优先引入精确定时视频唤醒
3. 再将视频同步从硬阈值动作改为连续 delay 校正
4. 补充音频输出延迟补偿
5. 暂不引入 serial、flush-only seek、音频 sample 级补偿

### Consequences

- 需要新增一个补充重构 iteration，而不是回写 `Iteration 06`
- `AVSynchronizer` 与 `PlayerController` 会发生职责演进，但整体边界保持不变
- `IAudioOutput` 需要新增延迟查询能力
- roadmap 需插入一个专门的 AV sync refactor iteration

### Alternatives Considered

#### 方案 A：保持当前基础同步实现不动

优点：

- 改动最小

缺点：

- 无法解决视频调度精度与同步抖动问题

#### 方案 B：一次性完整照搬 ffplay 时钟与 serial 体系

优点：

- 理论上最接近参考实现

缺点：

- 改动面过大
- 与当前项目阶段不匹配
- 回归成本高

#### 方案 C：仅修改阈值，不调整调度模型

优点：

- 实现简单

缺点：

- 不能从根本上解决固定轮询带来的时序误差

### Follow-up

需要更新：

- `docs/spec/02-roadmap.md`
- `docs/spec/iterations/iter-07a-av-sync-refactor-ffplay.md`
- 相关实现文件与完成后的 changelog
