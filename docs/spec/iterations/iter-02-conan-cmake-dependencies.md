# Iteration 02：Conan + CMake + 基础依赖接入

## 目标

完成工程依赖管理，接入基础库。

## 范围

实现：

```text
1. conanfile.py
2. CMake options
3. FFmpeg 依赖接入
4. spdlog 接入
5. fmt 接入
6. nlohmann_json 接入
7. stb 接入
8. Logger 模块
9. AppConfig 模块
```

## 验收标准

```text
1. conan install 成功。
2. cmake configure 成功。
3. 项目可以链接 spdlog。
4. 项目可以读取默认 JSON 配置。
5. FFmpeg 头文件和库可以在 ffmpeg/ 模块中使用。
6. UI 模块仍然不直接依赖 FFmpeg。
```

