# 端侧自动字幕运行时、模型与许可候选

研究日期：2026-09-07  
对应决策票据：[调查端侧自动字幕运行时、模型与许可候选](https://github.com/200166shang/qt-video-player/issues/12)

## 结论

首个 **AI-assisted Subtitles** 实现应以 **whisper.cpp + OpenAI Whisper `base` 多语言模型** 为基线，在 `macOS arm64` 上先完成准确率、实时因子、峰值内存和词级时间质量验证，再进入 Windows。运行时应封装在项目自己的 ASR 端口之后；`sherpa-onnx + 同一套 OpenAI Whisper 权重的 ONNX 转换产物` 保留为对照候选，而不是首个实现。

这个选择的主要依据是：

- whisper.cpp v1.9.2 是无外部运行时依赖的 C/C++ 实现，官方列出 macOS、Windows、Android、iOS，并提供 C API；这与 Qt/CMake 和未来共享 C++ 核心的集成路径最短。[whisper.cpp README（v1.9.2）](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/README.md)
- OpenAI 明确声明 Whisper 的代码和模型权重均使用 MIT 许可；whisper.cpp 本身也是 MIT。相比之下，sherpa-onnx 运行时虽为 Apache-2.0，但其模型目录包含多个不同作者与训练数据来源，不能把运行时许可推定为所有模型权重的许可。[OpenAI Whisper README](https://github.com/openai/whisper/blob/86098128c0b4f24f0e2aa2994de830614b474227/README.md#license)、[OpenAI Whisper LICENSE](https://github.com/openai/whisper/blob/86098128c0b4f24f0e2aa2994de830614b474227/LICENSE)、[whisper.cpp LICENSE（v1.9.2）](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/LICENSE)、[sherpa-onnx LICENSE（v1.13.7）](https://github.com/k2-fsa/sherpa-onnx/blob/917bed95c8e5c7c18aa4d69fea42e9ef8ef0a60e/LICENSE)
- `base` 是仍支持中文和其他语言的最小合理默认起点：官方 ggml 产物约 142 MiB、加载内存约 388 MB；`tiny` 约 75 MiB/273 MB，可作为低资源回退；`small` 约 466 MiB/852 MB，可作为高质量可选包。实际中文质量不能由模型尺寸表推断，必须用目标素材测量。[whisper.cpp 模型与内存表（v1.9.2）](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/README.md#memory-usage)、[模型文件清单（v1.9.2）](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/models/README.md#available-models)、[OpenAI 模型与语言说明](https://github.com/openai/whisper/blob/86098128c0b4f24f0e2aa2994de830614b474227/README.md#available-models-and-languages)

这是一项候选选择，不是性能验收。官方资料不能证明 PlayerLab 的目标设备上一定达到实时，也不能证明自动生成的时间位置足以直接成为最终字幕；这些必须由后续原型和基准票据回答。

## 候选比较

| 维度 | whisper.cpp v1.9.2 | sherpa-onnx v1.13.7 | 对 PlayerLab 的含义 |
| --- | --- | --- | --- |
| 四平台 | 官方列出 macOS（Intel/Arm）、Windows（MSVC/MinGW）、Android、iOS | 官方矩阵覆盖 Windows/macOS x64/arm64、Android 多架构和 iOS arm64 | 两者都满足长期平台边界；首版只需验证 macOS、Windows |
| C++/Qt 集成 | C API、CMake、依赖面小，可作为静态库置于媒体/AI 基础设施层 | C/C++ API、CMake；另有 Kotlin、Swift 等绑定，但携带 ONNX Runtime | 两者都不应让 Qt 类型进入持久化的 Editing Project；whisper.cpp 首次接入更窄 |
| 模型范围 | 聚焦 Whisper/兼容模型 | Whisper、Zipformer、Paraformer、CTC 等多类模型 | sherpa-onnx 更适合未来做模型竞赛或加入流式识别；首版不需要这份广度 |
| 时间信息 | C API 中 `token_timestamps` 标记为 experimental，并暴露 token 起止时间；OpenAI 参考实现用 cross-attention + DTW 生成词级时间 | Offline Whisper 在 v1.13.7 中可启用 token timestamps，以 cross-attention + DTW 生成 token 起点和时长；通用结果对象注明时间戳“when available” | 两者原生首先得到的是 token 对齐，不等同于稳定的中文“词”；必须保留原 token 时间，再由字幕分段层聚合、校正 |
| CPU/GPU/NPU | CPU 基线；Apple 路径有 Accelerate/Metal，Core ML 可把 encoder 放到 ANE；Windows 可选 CUDA、Vulkan、OpenVINO；Android 有 OpenCL/Hexagon 构建后端但设备覆盖需验证 | CPU 基线；公开配置主要为 CPU、CUDA、CoreML；底层 ONNX Runtime 另有 NNAPI、QNN、DirectML 等 EP，但 sherpa-onnx 并未因此自动获得完整支持 | 不建立“跨平台统一 NPU”承诺。首版以 CPU 正确性为基线，再逐平台开启经过基准验证的后端 |
| 许可 | 运行时 MIT；OpenAI 原始代码和权重 MIT | 运行时 Apache-2.0；具体模型权重、词表和训练数据需逐包审计 | 商业化可逆性上，首选仅使用 OpenAI 官方权重或可复现转换，禁止无清晰模型许可证的包进入发布清单 |
| 风险 | token 时间 API 仍标注 experimental；Whisper 解码成本随模型增加；中文断词没有天然空格边界 | 二进制和依赖面更大；时间支持随模型而异；移动硬件 EP 覆盖不是统一保证；模型许可更分散 | 在自有接口中保存 `token/span/confidence-or-null/model-id`，不把任何运行时返回结构写进工程格式 |

平台与 API 事实来自各项目的版本化源码：[whisper.cpp 平台/API/加速说明](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/README.md)、[whisper.cpp 时间参数](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/include/whisper.h)、[sherpa-onnx 平台矩阵](https://github.com/k2-fsa/sherpa-onnx/blob/917bed95c8e5c7c18aa4d69fea42e9ef8ef0a60e/README.md#supported-platforms)、[sherpa-onnx Offline Whisper DTW 实现](https://github.com/k2-fsa/sherpa-onnx/blob/917bed95c8e5c7c18aa4d69fea42e9ef8ef0a60e/sherpa-onnx/csrc/offline-recognizer-whisper-impl.h)、[sherpa-onnx C++ 结果结构](https://github.com/k2-fsa/sherpa-onnx/blob/917bed95c8e5c7c18aa4d69fea42e9ef8ef0a60e/sherpa-onnx/c-api/cxx-api.h)。

## 推荐的产品与集成边界

### 1. 运行时端口

编辑领域层只依赖一个稳定的 `OnDeviceSpeechRecognizer` 语义端口，输入为已解码的单声道 PCM 与时间基准，输出至少包含：

- 识别语言、运行时/模型标识与模型内容哈希；
- 原始 token 文本、起止时间和可选置信信息；
- 由独立分段器产生的可编辑字幕段；
- 取消、进度、错误和资源不足状态。

`whisper.cpp`、未来的 `sherpa-onnx` 或平台原生实现都留在适配器层。Editing Project 保存规范化后的字幕结果和生成元数据，不保存运行时对象，也不要求重新打开工程时仍安装同一模型。

### 2. 模型分发

不要把 142 MiB 的默认模型直接塞入应用安装包。使用按需下载的模型包：清单固定 `model-id`、来源、权重许可证、文件哈希、尺寸、最低运行时版本和语言集合；下载到应用管理的共享模型缓存，支持断点/失败重试、完整性校验、删除与更新。用户导入的 Source Media 不离开设备；首次模型下载需要网络仍符合项目对 **On-device AI** 的定义。

首批建议：

- 默认：OpenAI Whisper `base` multilingual 的官方权重，经项目可复现工具转换为 ggml；
- 低资源回退：`tiny` multilingual，仅在基准显示质量可接受时暴露；
- 可选高质量：`small` multilingual，在内存与耗时预算验证后提供；
- 不采用 `.en` 变体作为默认，因为它们仅面向英文；不采用来源或权重许可不明确的微调模型。

whisper.cpp 官方同时提供预转换下载与从 OpenAI 权重自行转换的脚本；为建立供应链记录，发布流程应优先使用“固定 OpenAI 权重哈希 + 固定转换器版本 + 自有产物哈希”的可复现路径。[whisper.cpp 模型转换说明（v1.9.2）](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/models/README.md)

### 3. 平台加速策略

- macOS/iOS：先测 CPU/Accelerate 与 Metal；Core ML/ANE 作为第二组实验，因为官方路径只加速 encoder，且首次加载会发生设备侧编译。[Core ML 说明（v1.9.2）](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/README.md#core-ml-support)
- Windows：CPU/AVX 为最低基线；按硬件占有率再测 Vulkan、CUDA 或 OpenVINO，不能让 CUDA 成为功能前提。[whisper.cpp GPU 后端（v1.9.2）](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/README.md#nvidia-gpu-support)
- Android：CPU/NEON 为兼容基线；OpenCL/Hexagon 只能在明确设备矩阵上启用，不能把“能编译”视为性能或兼容性保证。[ggml 后端构建选项（v1.9.2）](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/ggml/CMakeLists.txt)

如果后续改走 ONNX Runtime，应先以 CPU/XNNPACK 建立正确性基线，再测 NNAPI/CoreML；官方移动指南明确指出硬件 EP 的性能取决于设备和模型，算子被拆分回退时反而可能变慢。[ONNX Runtime 移动部署指南（v1.29 文档）](https://onnxruntime.ai/docs/tutorials/mobile/)、[Execution Providers](https://onnxruntime.ai/docs/execution-providers/)

## 词级时间的定义与风险

项目需求中的“词级时间”应解释为“保留足以重新断句的细粒度 token/词对齐”，而不是承诺每个中文词都由模型直接给出可靠边界：

1. Whisper 原生输出包含子词 token；OpenAI 参考实现通过 cross-attention + DTW 提取 word timestamps。[OpenAI `transcribe.py`](https://github.com/openai/whisper/blob/86098128c0b4f24f0e2aa2994de830614b474227/whisper/transcribe.py)、[OpenAI `timing.py`](https://github.com/openai/whisper/blob/86098128c0b4f24f0e2aa2994de830614b474227/whisper/timing.py)
2. whisper.cpp v1.9.2 将 token timestamps 明确标为 experimental，因此输出必须经过测试、边界单调化、静音/VAD 约束和字幕段级人工校对。[whisper.cpp `whisper_full_params`](https://github.com/ggml-org/whisper.cpp/blob/306c88f4d1286aec1bf96e544632897886af5501/include/whisper.h)
3. 中文没有基于空格的天然词边界。持久化时应保留原 token 对齐；展示层可按标点、停顿、字符数和行宽合并为 Subtitle Track 段。用户编辑段文本后，不应假装旧 token 时间仍与新文本逐词对应。

## 进入实现前必须通过的基准

后续原型应使用同一组有权使用的真实短视频语料，在至少一台 Apple Silicon Mac 和一台无独显 Windows 设备上比较 `tiny/base/small`，并记录：

- 中文、英文和中英混说的 CER/WER；专名与标点错误单独统计；
- 30 秒、5 分钟、30 分钟音频的实时因子、首结果时间、峰值 RSS、温度/功耗趋势；
- token 起止时间相对人工标注的误差分布，以及自动分段后需要人工调整的比例；
- 取消延迟、并行播放时的掉帧/音频中断、模型下载失败与损坏恢复；
- CPU 基线与每个平台加速后端的结果一致性。

建议的首版门槛应由该基准票据决定，当前研究不虚构数值门槛。若 `base` 在最低桌面设备上不可接受，再在同一端口下比较 sherpa-onnx 的中文模型；任何替代模型进入产品前必须具备可归档的权重许可、训练数据说明、内容哈希和目标语料质量报告。

## 明确排除的路径

- 不以 Apple Speech、Android 或 Windows 的平台语音 API 作为共享首选：它们不能提供同一模型、同一时间语义和四平台一致的可复现实验；以后可以作为平台适配器或系统能力回退。
- 不直接把 ONNX Runtime 当作产品级 ASR：它是推理运行时，仍需自行承担 Whisper 前后处理、解码、时间对齐和模型兼容；若选择 ONNX 路线，优先复用 sherpa-onnx 已实现的这些层。
- 不在当前决策中承诺移动端实时识别、后台持续识别或统一 NPU 加速；首版场景是对 Editing Project 的已导入媒体执行可取消的离线任务。

## 版本范围

本报告核对的是 whisper.cpp **v1.9.2**（commit `306c88f`）、sherpa-onnx **v1.13.7**（commit `917bed9`）、ONNX Runtime **v1.29.0** 文档，以及 OpenAI Whisper `main` commit `8609812`。运行时、硬件后端和模型目录变化很快；真正开始实现时应重新锁定依赖版本并复查许可与发布说明。
