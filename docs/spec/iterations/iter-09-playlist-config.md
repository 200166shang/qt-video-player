# Iteration 09：播放列表 + 最近打开 + 配置持久化

## 目标

让播放器具备基础媒体管理能力。

## 范围

实现：

```text
1. 添加本地文件到播放列表
2. 添加网络 URL 到播放列表
3. 双击播放列表项播放
4. 播放结束自动播放下一项
5. 最近打开记录
6. 播放器配置保存和恢复
```

## 数据结构

```cpp
struct PlaylistItem {
    QString title;
    MediaSource source;
    QString durationText;
    QString resolutionText;
    QString codecText;
};
```

## 验收标准

```text
1. 可以添加多个文件。
2. 可以添加 URL。
3. 双击列表项可以播放。
4. 关闭应用后最近打开记录仍存在。
5. 音量、渲染后端等配置可以保存。
```

