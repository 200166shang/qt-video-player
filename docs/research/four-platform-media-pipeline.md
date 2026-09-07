# 四端媒体管线、硬件加速与 FFmpeg 分发约束

> 调研日期：2026-09-07  
> 对应票据：[调查四端媒体管线、硬件加速与 FFmpeg 分发约束](https://github.com/200166shang/qt-video-player/issues/16)  
> 版本范围：Qt 6.10.3、FFmpeg 9.0.1（上游当前稳定版），以及截至调研日各平台公开 SDK/商店规则。

## 结论摘要

四端共享 C++ 编辑语义与 FFmpeg 解封装/软件回退是可行的，但“同一套媒体实现不分平台”并不能同时保证硬件加速、低拷贝和商店分发。事实支持的候选边界是：把压缩包解析、时间戳、编辑调度、音频处理和软件回退放进共享媒体核心；把硬件帧当作带平台句柄、颜色信息、所有权与同步信息的资源；由平台适配层接入 Apple VideoToolbox/Core Video/Metal、Windows Media Foundation/D3D11、Android MediaCodec/AHardwareBuffer/OpenGL ES 或 Vulkan。最终架构仍需用原型验证后另行决定。

对首个 `Baseline Export`，四端操作系统都提供 H.264 硬件编码路径，软件编码也能作为正确性回退。但硬件能力依设备、驱动、像素格式和配置而变，不能写成无条件合同；应用必须做运行时能力查询、失败降级和结果校验。

分发约束是架构输入而非发布末期事项：FFmpeg 默认是 LGPL 2.1+，启用 GPL 部件会使整个 FFmpeg 构建受 GPL 约束，`--enable-nonfree` 产物不可再分发。FFmpeg 官方合规清单推荐动态链接并要求对应源码、构建方式、声明等；iOS 的实际打包方式与 LGPL 的替换/重新链接权利需要在确定闭源和 App Store 分发前做专项法律审查。H.264/AAC 等标准的专利问题与 FFmpeg/Qt 软件许可证是两件事，使用系统编码器也不自动消除专利义务。

## 当前项目与版本基线

- 当前仓库直接使用 Qt Widgets、OpenGL Widgets、Qt Multimedia，以及 FFmpeg 的 `avformat`、`avcodec`、`avutil`、`swscale`、`swresample`。现有路径适合单媒体播放验证，还没有编辑合成/离线导出管线。
- Qt 6.10 官方支持 macOS 13+（x86_64/arm64）、Windows 10 1809+ 与 Windows 11、Android 9/API 28 至 Android 16/API 36，以及 iOS 17+。Android 官方 Qt 包使用 NDK r27c，Qt 建议应用使用相同 NDK 版本以避免符号问题。[Qt 6.10 支持平台](https://doc.qt.io/qt-6.10/supported-platforms.html)
- 调研日 FFmpeg 上游当前稳定版是 9.0.1；Qt Multimedia 6.10.3 自带并测试的是 FFmpeg 7.1.3，且要求自带 FFmpeg 的主版本与 Qt 所用版本匹配。因此，“项目直接使用 FFmpeg 9”与“复用 Qt Multimedia 的 FFmpeg 后端”是两个需要显式协调的依赖策略，不能假设可以混用。[FFmpeg 下载页](https://ffmpeg.org/download.html)；[Qt Multimedia 6.10 后端说明](https://doc.qt.io/qt-6.10/qtmultimedia-index.html#target-platform-and-backend-notes)
- Qt Multimedia 6.10 的默认后端（除 WebAssembly 和嵌入式 Linux）是 FFmpeg；Qt 自带的 Android MediaCodec 原生后端自 6.8 起已弃用，Windows Media Foundation 原生后端自 6.10 起已弃用，新增功能只承诺进入 FFmpeg 后端。这个“弃用”针对 Qt 的原生后端插件，不代表操作系统的 MediaCodec 或 Media Foundation API 被弃用。[Qt Multimedia 6.10 后端说明](https://doc.qt.io/qt-6.10/qtmultimedia-index.html#native-backends)

## 平台事实

### macOS 与 iOS

**解码与编码**

- VideoToolbox 是 Apple 直接访问硬件视频编码和解码的低层框架，核心对象是 `VTDecompressionSession` 与 `VTCompressionSession`；输出/输入围绕 Core Video 像素缓冲区。可用 `VTIsHardwareDecodeSupported` 对指定编码运行时探测硬件解码支持。[VideoToolbox](https://developer.apple.com/documentation/videotoolbox)；[VTIsHardwareDecodeSupported](https://developer.apple.com/documentation/videotoolbox/vtishardwaredecodesupported(_:))
- Apple 提供了专门的离线转码范例：将 `kVTCompressionPropertyKey_RealTime` 设为 false，逐帧向 `VTCompressionSessionEncodeFrame` 提交 `CVPixelBuffer`，完成后调用 `VTCompressionSessionCompleteFrames`。这证明 VideoToolbox 不是只面向实时录制，也可作为离线导出编码端。[Encoding video for offline transcoding](https://developer.apple.com/documentation/videotoolbox/encoding-video-for-offline-transcoding)
- AVFoundation 的 `AVAssetReader` 可读取文件资产或 `AVComposition`，`AVAssetWriter` 可写 QuickTime/MP4、交织多轨并重新编码。它们是另一条更平台化的读写/封装候选路径，但会把编辑行为更多绑定到 Apple 平台语义。[AVAssetReader](https://developer.apple.com/documentation/avfoundation/avassetreader)；[AVAssetWriter](https://developer.apple.com/documentation/avfoundation/avassetwriter)
- FFmpeg 暴露 `videotoolbox` 硬件设备类型，其硬件帧格式是 `AV_PIX_FMT_VIDEOTOOLBOX`；FFmpeg 9 也包含 `h264_videotoolbox` 编码器。因此共享 FFmpeg 核心接入 VideoToolbox 在 API 层是可行候选，但具体像素格式、颜色附件和失败回退仍需原型验证。[FFmpeg VideoToolbox 硬件上下文](https://ffmpeg.org/doxygen/trunk/hwcontext__videotoolbox_8c.html)；[FFmpeg VideoToolbox H.264 编码器](https://ffmpeg.org/doxygen/9.0/videotoolboxenc_8c.html)

**低拷贝 GPU 互操作**

- `CVMetalTextureCacheCreateTextureFromImage` 可从 `CVImageBuffer` 创建与底层 `MTLTexture` 活绑定的 Metal 纹理；平面 YUV 可分别映射亮度与色度纹理。创建/申请像素缓冲时需声明 Metal 兼容，并必须把纹理引用保留到 GPU 命令完成。这支持避免 CPU 像素回读，但不免除颜色转换、资源生命周期和 GPU 同步工作。[CVMetalTextureCacheCreateTextureFromImage](https://developer.apple.com/documentation/corevideo/cvmetaltexturecachecreatetexturefromimage(_:_:_:_:_:_:_:_:_:))

**沙箱与分发**

- Mac App Store 要求 App Sandbox。用户经打开/保存面板选择的外部素材可获得安全作用域访问；跨重启持久访问要保存 security-scoped bookmark。Editing Project 若只保存普通绝对路径，在沙箱版中不足以保证重开后还能访问 Source Media。[macOS App Sandbox](https://developer.apple.com/documentation/security/app-sandbox)；[访问 macOS 沙箱外文件](https://developer.apple.com/documentation/security/accessing-files-from-the-macos-app-sandbox)
- iOS 的文档选择器给外部文件 security-scoped URL；应用需开始/结束安全作用域访问，并保存 bookmark 而不是原始 URL。外部提供方、权限撤销或文件移动都可能使访问失败，工程必须保留“素材丢失/重新定位”的产品语义。[UIDocumentPickerViewController](https://developer.apple.com/documentation/uikit/uidocumentpickerviewcontroller)；[Providing access to directories](https://developer.apple.com/documentation/uikit/providing-access-to-directories)
- Apple 审核规则 2.5.1 要求使用公开 API，2.5.2 要求应用自包含且不得下载、安装或执行会改变功能的代码。随包签名的 FFmpeg/模型运行时与运行时下载可执行插件不是同一种风险；后者不能作为 iOS 扩展媒体能力的默认机制。[App Review Guidelines](https://developer.apple.com/app-store/review/guidelines/#software-requirements)
- macOS 商店外公证要求 Hardened Runtime；Mac App Store 还要求 App Sandbox。动态库、辅助进程、文件访问和签名链应在构建方案中尽早验证。[Preparing your app for distribution](https://developer.apple.com/documentation/xcode/preparing-your-app-for-distribution)；[Configuring the hardened runtime](https://developer.apple.com/documentation/xcode/configuring-the-hardened-runtime)

### Windows

**解码与编码**

- Media Foundation 的 Source Reader 可在应用自有媒体管线中解码并交出样本，不负责呈现时钟或音视频同步，适合由编辑器控制调度。通过 DXGI Device Manager 共享 D3D11 设备时，支持硬件解码；不支持的格式应允许软件回退。[Source Reader](https://learn.microsoft.com/en-us/windows/win32/medfound/source-reader)；[Source Reader 硬件加速](https://learn.microsoft.com/en-us/windows/win32/medfound/processing-media-data-with-the-source-reader#hardware-acceleration)
- D3D11 视频解码以外部创建的 D3D11 设备和 `IMFDXGIDeviceManager` 在管线组件间共享；官方说明明确要求在硬件配置不可用时重新协商为软件解码。[Supporting Direct3D 11 Video Decoding in Media Foundation](https://learn.microsoft.com/en-us/windows/win32/medfound/supporting-direct3d-11-video-decoding-in-media-foundation)
- FFmpeg 当前文档提供 `h264_mf`、`hevc_mf`、`av1_mf` Media Foundation 编码器；硬件编码要求 D3D11，NV12 是硬件编码器更安全的输入格式。文档也给出 D3D11VA 解码、D3D11 缩放再交给 `h264_mf` 的硬件路径示例。[FFmpeg MediaFoundation 编码器](https://ffmpeg.org/ffmpeg-codecs.html#MediaFoundation)

**低拷贝 GPU 互操作**

- `MFCreateDXGISurfaceBuffer` 可以把 `ID3D11Texture2D`（或 D3D12 resource）包装成 `IMFMediaBuffer`；Media Foundation 的 D3D11 解码说明也以纹理数组和 DXGI media buffer 交付解码表面。这给“解码纹理 → GPU 合成 → 编码样本”提供了避免 CPU 往返的原语，但共享设备、子资源、fence/锁和编码器接受的绑定标志必须实测。[MFCreateDXGISurfaceBuffer](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mfcreatedxgisurfacebuffer)；[D3D11 解码表面分配](https://learn.microsoft.com/en-us/windows/win32/medfound/supporting-direct3d-11-video-decoding-in-media-foundation#allocating-uncompressed-buffers)
- FFmpeg 暴露 D3D11VA 硬件设备上下文，可作为共享 FFmpeg 管线持有 D3D11 硬件帧的入口。[FFmpeg 硬件上下文类型](https://ffmpeg.org/doxygen/trunk/hwcontext_8c_source.html)

**沙箱与分发**

- Windows 桌面应用既可不打包，也可用 MSIX/外部位置包获得 package identity；Win32 包可声明 `mediumIL`/`runFullTrust`，不必进入 AppContainer。与 Apple 平台相比，文件系统访问不是天然被统一强制为文档沙箱，但安装目录、包内容只读和商店能力声明仍影响实现。[Windows packaging overview](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/packaging/)；[Understanding how packaged desktop apps run](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes)
- 这意味着首版可以继续使用标准 Win32/Qt 文件路径，同时应把文件定位抽象成平台资源标识，避免共享工程格式依赖 Windows 绝对路径语义。

### Android

**解码与编码**

- `MediaCodec` 可按 MIME 创建系统首选解码器/编码器。视频解码器可配置输出 `Surface`；视频编码器可用 `createInputSurface()` 接收 GPU 生产的原始帧。输入 Surface 模式不会暴露输入 ByteBuffer，结束时用 `signalEndOfInputStream()`。[MediaCodec](https://developer.android.com/reference/android/media/MediaCodec)
- Android 10/API 29 起，`MediaCodecInfo` 可报告 `isHardwareAccelerated()`、`isSoftwareOnly()`、`isVendor()`，并给硬件编解码器的 performance points。能力仍取决于具体设备，不能只按 Android 版本推断。[Media codec performance](https://developer.android.com/media/optimize/performance/codec)
- Android 官方格式表要求 H.264 Baseline 解码，并从 Android 3.0 起提供 H.264 Baseline 编码；Main 编码只是推荐而非所有设备必有。Baseline Export 应运行时选择 profile/level/尺寸并验证输出，而不是把桌面编码参数原样下发。[Supported media formats](https://developer.android.com/media/platform/supported-formats)
- FFmpeg 暴露 `mediacodec` 硬件上下文，但这只证明存在接入点，不证明所有设备上的随机 seek、多实例、颜色元数据和编码表面链路一致。[FFmpeg 硬件上下文类型](https://ffmpeg.org/doxygen/trunk/hwcontext_8c_source.html)

**低拷贝 GPU 互操作**

- `AImageReader_newWithUsage` 可创建 `AIMAGE_FORMAT_PRIVATE` 的不透明图像队列；内容不可由 CPU 直接访问，但可通过 `AImage_getHardwareBuffer()` 获得 `AHardwareBuffer`，再交给 GPU 或硬件编码器。官方明确指出这种格式在无需软件访问时通常更高效。[Android NDK Media / AImageReader](https://developer.android.com/ndk/reference/group/media)
- `AHardwareBuffer` 可绑定到 EGL/OpenGL ES，也可作为 Vulkan external memory。它提供的是共享硬件缓冲原语；支持的格式/usage 组合、同步 fence 和厂商实现仍需在目标设备矩阵上验证。[Native Hardware Buffer](https://developer.android.com/ndk/reference/group/a-hardware-buffer)

**沙箱与分发**

- Android 的 Storage Access Framework 以用户选择的 content URI 授予长期访问；即使调用 `takePersistableUriPermission()`，文件移动或删除后 URI 仍可能失效。因此 Editing Project 需要保存 URI/平台令牌并保留重定位流程，不能假设移动端存在稳定绝对路径。[Storage Access Framework](https://developer.android.com/guide/topics/providers/document-provider)；[共享存储文档访问](https://developer.android.com/training/data-storage/shared/documents-files)
- 自 2026-08-31 起，Google Play 新应用和更新必须 target Android 16/API 36。Qt 6.10 的 Android 支持上限正好包含 API 36；持续发布要求把 target SDK 升级列为维护工作，而非一次性配置。[Google Play target API requirements](https://support.google.com/googleplay/android-developer/answer/11926878)

## 跨平台候选组合（供后续决策，不在本报告中选择）

### 候选一：共享 FFmpeg 主干 + 平台硬件帧适配

- FFmpeg 负责解封装、时间戳、软件解码回退、音频、复用；平台硬件上下文负责 VideoToolbox、D3D11VA、MediaCodec。
- 合成器消费“CPU frame 或 platform hardware frame”的统一抽象，平台 GPU 适配层负责 CVPixelBuffer/Metal、ID3D11Texture2D/D3D11、AHardwareBuffer/EGL/Vulkan。
- 导出优先走 VideoToolbox、Media Foundation、MediaCodec 硬件编码，失败时落回一个许可证可接受的软件编码路径。
- 优点是编辑语义、容器支持和时间戳处理集中；风险是 FFmpeg 硬件上下文与原生 GPU/Qt 窗口系统之间的桥接、移动端分发和硬件差异仍由项目承担。

### 候选二：共享编辑调度 + 完全原生媒体/GPU 后端

- Apple 使用 AVFoundation/VideoToolbox/Metal，Windows 使用 Media Foundation/D3D11，Android 使用 MediaCodec/AHardwareBuffer/OpenGL ES 或 Vulkan；FFmpeg 只用于格式补充或软件回退。
- 优点是系统硬件路径和商店集成最直接；风险是三套媒体行为、时间戳与错误模型会扩大个人开发者的维护面，预览与离线导出的跨平台一致性更难证明。

### 候选三：Qt Multimedia 作为 Baseline Export/预览适配层

- Qt 6.8+ 的 `QVideoFrameInput` 能向 `QMediaRecorder` 提交自定义帧，且只有 FFmpeg 后端支持；它有背压信号，但官方说明队列满时发送失败，若不能丢帧需应用自行排队。可用于快速验证 Baseline Export，而不能仅凭 API 存在就认定它满足帧精确、不可丢帧的最终导出合同。[QVideoFrameInput](https://doc.qt.io/qt-6.10/qvideoframeinput.html)；[QMediaRecorder](https://doc.qt.io/qt-6.10/qmediarecorder.html)
- Qt 的 QRhi 可抽象 Metal、D3D、OpenGL ES、Vulkan，但属于 `GuiPrivate`，官方明确不保证源码或二进制兼容。若将其用于编辑器核心渲染边界，就要接受锁定 Qt 小版本或维护适配层的成本。[QRhi](https://doc.qt.io/qt-6/qrhi.html)

## 许可与商业化约束

1. **固定 FFmpeg 构建清单。** FFmpeg 默认 LGPL 2.1+；`--enable-gpl` 会使整个构建适用 GPL，`--enable-nonfree` 组合不可分发。首个商业可能构建应显式禁止这两个开关，并审计每个外部库；尤其不要无意加入 GPL 的 libx264/libx265。[FFmpeg License and Legal Considerations](https://ffmpeg.org/legal.html)；[FFmpeg License](https://ffmpeg.org/doxygen/trunk/md_LICENSE.html)
2. **不要把“开源库”误当成“无需发布流程”。** FFmpeg 官方 LGPL 清单推荐动态链接，同时要求对应源码、修改 diff、完整构建命令、许可证/关于页/EULA 声明，并允许为调试修改而逆向工程。每个平台的发布产物都应由可复现脚本生成并归档 `configure` 输出与源码 commit。[FFmpeg 合规清单](https://ffmpeg.org/legal.html#License-Compliance-Checklist)
3. **静态链接需要提供重新链接能力。** LGPL 2.1 第 6 节要求满足共享库替换机制，或提供能让接收者修改库后重新链接的目标文件/源码等材料。iOS 支持 framework 形式的动态链接，也支持静态 XCFramework，但“技术上能打包”不等于“已满足 LGPL 与商店条款”；在闭源 App Store 方案锁定前应取得法律意见并做实际提交验证。[GNU LGPL 2.1 第 6 节](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html#SEC6)；[Apple XCFramework](https://developer.apple.com/documentation/xcode/creating-a-multi-platform-binary-framework-bundle)
4. **编码标准专利另行评估。** Qt 6.10 官方也明确警告 H.264 等压缩标准可能产生专利费，且系统服务、第三方库或 Qt 后端实现都可能涉及；Qt 许可证不承担这些费用。软件许可清单不能替代目标市场的专利/商标审查。[Qt Multimedia licenses and attributions](https://doc.qt.io/qt-6.10/qtmultimedia-index.html#licenses-and-attributions)
5. **版本不能由系统 Qt 和自建 FFmpeg 双重决定。** 若项目直接链接一套 FFmpeg，同时 Qt Multimedia 插件再部署另一主版本，进程内符号/插件加载与安全更新会复杂化。后续架构票据必须明确：统一复用 Qt 的 FFmpeg、完全自建并匹配 Qt 主版本，或隔离/移除 Qt Multimedia 媒体后端。

## 后续架构决策前必须验证的原型

这些是由事实暴露出的验证点，不是本票据中的架构选择：

1. macOS：VideoToolbox 解码 `CVPixelBuffer` → Metal Y/UV 纹理 → 合成 → `VTCompressionSession` H.264，记录 CPU 拷贝次数、颜色一致性和 seek 延迟。
2. Windows：D3D11VA/Media Foundation 解码纹理 → D3D11 合成 → `h264_mf` 或 Sink Writer，验证同一 D3D11 设备、NV12 路径、软件回退和 Intel/AMD/NVIDIA 差异。
3. Android：MediaCodec Surface 解码 → AHardwareBuffer/EGL 或 Vulkan → encoder input Surface，至少覆盖两家 SoC；验证随机 seek、flush、多解码器实例、旋转/像素宽高比、fence 和后台/前台切换。
4. iOS：与 macOS 相同的 VideoToolbox/Metal 路径在真机上验证，同时做 App Store 签名沙箱、FFmpeg framework/XCFramework、Source Media bookmark 重开实验。
5. 四端共同验证：H.264/AAC MP4 的时间基、VFR 输入、B 帧、音画同步、导出取消/恢复、颜色范围/矩阵/传递函数、硬件失败转软件后的语义一致性。
6. 对每次发布保存 FFmpeg 源码版本、配置命令、依赖许可证清单、二进制校验和，并把 GPL/nonfree 开关检测加入 CI。

## 可供 Wayfinder 使用的一句话事实

四端共享媒体核心可行，但硬件加速和低拷贝必须通过平台资源适配层实现；后续架构应在“共享 FFmpeg 主干 + VideoToolbox/D3D11/MediaCodec 硬件帧”与“完全原生后端”之间用四个原型作选择，并把 LGPL/iOS 分发、编码专利和可复现 FFmpeg 构建设为前置门禁。
