# Issue 跟踪器：GitHub

本仓库的需求与规格都存放在 GitHub Issues 中。所有操作使用 `gh` CLI 完成。

## 约定

- 使用 `gh issue create` 创建 Issue。
- 使用 `gh issue view <编号> --comments` 阅读 Issue 及完整讨论。
- 使用 `gh issue list` 列出并筛选 Issue。
- 使用 `gh issue comment <编号>` 添加评论。
- 使用 `gh issue edit` 添加或移除标签。
- 使用 `gh issue close` 关闭已完成或明确不处理的 Issue。
- 通过 `git remote -v` 推断当前 GitHub 仓库。

## 是否将 Pull Request 作为分诊入口

不将 Pull Request 作为需求分诊入口。

## 发布

当技能要求“发布到 Issue 跟踪器”时，创建一个 GitHub Issue。

当技能要求“读取相关 ticket”时，读取完整的 GitHub Issue，包括评论和标签。

## 关系

在可用时，使用 GitHub Sub-issues 和原生 Issue Dependencies 表达父子关系与阻塞关系。

如果原生依赖关系不可用，则在 Issue 正文中记录：

```text
Blocked by: #<Issue 编号>
```

只有当所有阻塞 Issue 均已关闭时，该 Issue 才可以开始实施。
