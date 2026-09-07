# Qt 6 四端 UI、渲染与部署约束

> 调研日期：2026-09-07  
> 版本范围：Qt 6.11.2 文档（Qt 6.11 当前最新补丁版）；不把 Qt 6.12 Beta 能力视为可用基线。

## 问题与结论

问题是：macOS、Windows、Android、iOS 是否能以 Qt 6 支撑 PlayerLab 的 **Desktop Application** 与未来 **Mobile Application**，以及这些事实如何约束“共享核心 + 平台外壳”。

结论：四个平台都属于 Qt 6.11 官方支持范围，但这并不意味着应共享同一套 UI 或单一原生图形 API。推荐的架构边界是：

1. 共享不依赖窗口系统的 C++/Qt Core 领域与媒体核心；
2. Desktop Application 保持 Qt Widgets，Mobile Application 使用 Qt Quick；若未来采用原生 UI + 嵌入 Qt 内容，则将其视为需要逐平台验证的混合方案；
3. 预览/合成渲染以平台无关接口隔离，分别适配 Metal、Direct3D、Vulkan/OpenGL ES；不把 OpenGL 或 QRhi/`QNativeInterface` 直接暴露为长期稳定的核心接口；
4. 从一开始使用 CMake，并把四端当作四套构建、签名、部署和设备验证流水线，而不是一次编译的不同输出。

这是由官方支持边界推导出的架构结论，不代表零成本复用：视频解码纹理互操作、设备丢失、前后台切换、移动端文件访问和端侧 AI 模型打包仍需各自原型验证。

## 版本与官方支持基线

Qt 6.11.2 是调研时 Qt 6.11 的最新补丁版，标准支持期到 2027-03-17。Qt 6.8 是五年 LTS，但 6.8 后续 LTS 补丁的即时访问仅面向 Qt 商业客户；因此，“长期项目使用 LTS”还取决于未来许可证选择，不能仅由技术偏好决定。[Qt Releases](https://doc.qt.io/qt-6/qt-releases.html)

Qt 6.11 的官方平台矩阵如下；Qt 也明确提醒，后续补丁版可能替换或移除具体配置，所以应在锁定 Qt 补丁版时同步锁定工具链：[Supported Platforms](https://doc.qt.io/qt-6/supported-platforms.html)

| 平台 | Qt 6.11 官方目标 | 架构/工具链要点 |
| --- | --- | --- |
| macOS | macOS 13+ | `x86_64`/`arm64`，Xcode 15 与 macOS 14 SDK 或更高 |
| Windows | Windows 10 1809+、Windows 11 | x86-64：MSVC 2022 或 MinGW-w64 13.1；ARM64：MSVC 2022；不支持 ARM64EC |
| Android | Android 9（API 28）到 Android 16（API 36） | `arm64-v8a`、`x86_64`、`x86`、`armeabi-v7a`；NDK r27c/Clang 17.0.2；JDK 21；Gradle 9.3.1/AGP 9.0.0 |
| iOS | iOS 17+ | `arm64`，Xcode 15 与 iOS 17 SDK 或更高；构建主机必须是 macOS/Xcode |

对 PlayerLab 而言，首个移动发布版本不应承诺一个固定多年不变的最低 OS：它应在进入移动实施时根据选定 Qt 版本重新锁定。Windows 10 也不应成为长期架构假设，因为 Qt 官方已宣布 Qt 6.12 将是最后一个支持 Windows 10 的版本。[Supported Platforms](https://doc.qt.io/qt-6/supported-platforms.html)

## UI 与输入

### Qt Widgets：桌面合适，移动可编译但不适合作为共享 UI

Qt 官方把 Qt Widgets 定位为成熟、功能丰富的复杂桌面 UI，原生桌面观感覆盖 Windows、Linux、macOS；Widgets 的触控支持有限，基本假设仍是鼠标和键盘，整套 Widget UI 主要由软件绘制，硬件加速内容通常是隔离的专用 Widget。[User Interfaces](https://doc.qt.io/qt-6/topics-ui.html)

Qt for Android 的平台层确实连接 Qt Core、Qt GUI、Qt Quick 和 Qt Widgets，且官方将 Widgets 描述为“需要时使用的传统桌面风格组件”。这证明 Widgets 在 Android 上可用，却不是其适合作为触控剪辑器 UI 的证据。[Qt for Android](https://doc.qt.io/qt-6/android.html)

因此，当前 Desktop Application 可以继续使用 Qt Widgets，获得成熟的菜单、停靠窗口、表格/树视图和键鼠交互；但不应让 Widget 类型进入 Editing Project、命令、时间线模型或媒体核心的公开接口，也不应计划把整套 Widgets UI 原样移植到 Mobile Application。

### Qt Quick：移动端首选，也可按需嵌入桌面端

Qt 官方将 Qt Quick 定位为动态、流畅、适合触控和手势的 UI；Qt Quick Controls 默认在 Android 使用 Material Style，在 iOS 使用 iOS Style，在 macOS/Windows 使用对应桌面 Style。[User Interfaces](https://doc.qt.io/qt-6/topics-ui.html) [Styling Qt Quick Controls](https://doc.qt.io/qt-6/qtquickcontrols-styles.html)

Qt Quick 可以显示在 Widgets UI 中，因此 Desktop Application 后续可以仅将预览画布、时间线等高动态图形区域迁移成 Qt Quick，而无需一次性重写全部桌面外壳。[User Interfaces](https://doc.qt.io/qt-6/topics-ui.html)

Android 有两种官方路线：完整 Qt 应用，或将 Qt Quick 作为 Android `View` 嵌入原生应用。后者能放入 Android Fragment；相反，完整 Qt for Android 应用本身不支持 Android Fragments。[Qt for Android](https://doc.qt.io/qt-6/android.html)

### 原生窗口与系统能力是明确的适配边界

Qt 的 `QWindow::fromWinId()` 能包装 macOS `NSView*`、Windows `HWND`、iOS `UIView*` 和 Android `View`，官方示例在四端都演示了把原生控件嵌入 Qt UI；Android 另有明确的原生 `View` 嵌入 Qt Quick 路线。[Window Embedding Example](https://doc.qt.io/qt-6/qtdoc-demos-windowembedding-example.html) [Qt for Android](https://doc.qt.io/qt-6/android.html) 通用 `QWindow` 文档虽允许两个方向的父子窗口嵌入，也明确说明该能力依赖平台插件，除重设父子关系外的操作高度平台相关且未经测试；所以不能据此认定 iOS 混合外壳已经无风险。[QWindow](https://doc.qt.io/qt-6/qwindow.html)

但这些能力不消除平台代码：Android 未覆盖的系统 API 需要通过 JNI/Java/Kotlin 扩展；iOS 的 `Info.plist`、asset catalog、权限、签名和部分 Xcode 设置需要平台配置。应把素材选择、权限、分享、后台任务、系统媒体库等定义为平台服务接口，而非 UI 或媒体核心职责。[Deploying an Application on Android](https://doc.qt.io/qt-6/deployment-android.html) [Platform Notes - iOS](https://doc.qt.io/qt-6/ios-platform-notes.html)

Android 完整 Qt 应用还有生命周期约束：生命周期回调由 Qt 转换为 `QGuiApplication::applicationStateChanged`；Qt 不支持多个 Activity，应用进入不可见状态时 Qt 线程会被挂起。因此预览、音频、自动保存、导出和 AI 任务不能依赖桌面式“窗口隐藏但进程持续正常运行”的假设。[How Qt for Android Works](https://doc.qt.io/qt-6.8/android-how-it-works.html)

## 图形渲染

### Qt Quick 的跨平台渲染层可用，但底层 API 并不统一

Qt Quick 从 Qt 6 起通过 Qt Rendering Hardware Interface（RHI）记录资源和绘制命令，再翻译为 OpenGL/OpenGL ES、Vulkan、Metal 或 Direct3D。当前 Qt Quick 默认后端是 Windows 的 Direct3D 11、macOS 的 Metal，以及其他平台的 OpenGL；应用也能显式请求 D3D12、Vulkan、Metal、OpenGL 等后端。[Qt Quick Scene Graph Default Renderer](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html)

Qt 官方图形矩阵覆盖 OpenGL 2.1+、OpenGL ES 2.0+、Vulkan 1.0+、Direct3D 11/12 与 Metal 1.2+。[Graphics](https://doc.qt.io/qt-6/topics-graphics.html) 对本项目的直接含义是：现有 OpenGL 预览实现可以作为已验证原型，但不能成为四端渲染合同；Apple 平台应预期 Metal，Windows 应预期 Direct3D，Android 至少要覆盖默认 OpenGL ES，并把 Vulkan 作为经过设备能力探测和测试的可选路径。

Qt Quick 场景图允许三类自定义渲染集成：在场景前后发出命令、渲染到纹理后作为节点显示、或用 `QSGRenderNode` 将命令内联到场景图。渲染通常运行在专用线程，直接原生 API 命令必须与场景图使用同一种 API，并遵循 `beginExternalCommands()`/`endExternalCommands()` 等状态边界。[Qt Quick Scene Graph](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html) [QSGRenderNode](https://doc.qt.io/qt-6/qsgrendernode.html)

外部解码器产生的原生纹理可以通过 `QSGTexture` 的 D3D11、D3D12、Metal、OpenGL、Vulkan native interface 包装进 Qt Quick；但官方明确限制这些 native interface 不提供源码或二进制兼容保证，部分包装还只接受 2D RGBA 纹理、要求在渲染线程和场景图初始化后调用。[QSGTexture](https://doc.qt.io/qt-6/qsgtexture.html) [QNativeInterface](https://doc.qt.io/qt-6/qnativeinterface.html)

更重要的是，`QRhi` 本身要求链接 `Qt::GuiPrivate`，官方同样声明不提供源码或二进制兼容保证，只保证与开发时的 Qt 版本配套工作。[QRhi](https://doc.qt.io/qt-6/qrhi.html) 因此可以在“Qt 渲染适配器”内部使用 QRhi/`QSGRenderNode`，但共享媒体核心不应返回 `QRhiTexture*`、`QSGTexture*` 或平台纹理句柄；否则升级 Qt 小版本和切换 UI 外壳都会穿透核心。

### Widgets 桌面的可行过渡

Widgets 主界面是软件绘制，但 Qt 6.7 起提供 `QRhiWidget`，可在 Widget 层级中使用 Metal、Direct3D、Vulkan 或 OpenGL 渲染；默认后端为 macOS/iOS 的 Metal、Windows 的 D3D11、其他平台的 OpenGL。[QRhiWidget](https://doc.qt.io/qt-6/qrhiwidget.html)

这为 Desktop Application 提供两条可验证路线：

- 保留 Widgets 外壳，在一个专用预览 Widget 内完成 GPU 渲染；
- 保留 Widgets 外壳，将 Qt Quick 预览/时间线嵌入局部区域。

二者都不要求当前就把整个桌面 UI 改成 QML。由于 QRhi 兼容性与视频纹理互操作尚未被本项目实测，最终路线应由一个覆盖 macOS Metal 和 Windows D3D11 的预览原型决定。

## 构建与部署

### 共同基线：CMake 与每目标独立产物

Qt 6 的跨平台构建需要宿主工具和目标平台库；交叉编译时二者应使用相同 Qt 版本。[Cross-compiling Qt](https://doc.qt.io/qt-6/cross-compiling-qt.html) Android 多 ABI 包只有 CMake 路线受支持，因此四端共同构建描述应采用 CMake，并以平台 target/source 集合承载平台适配。[Supported Platforms](https://doc.qt.io/qt-6/supported-platforms.html)

“一个仓库”不等于“一个二进制”：媒体依赖、端侧模型、Qt 插件和 QML 模块都必须针对每个 ABI/平台构建并随各自应用包部署。动态加载的插件无法被部署工具完全推断，应显式做发布包设备测试。[The androiddeployqt Tool](https://doc.qt.io/qt-6/android-deploy-qt-tool.html)

### 平台差异

- **macOS**：产物是 `.app` bundle；Qt 提供 CMake deployment API 和 `macdeployqt` 收集依赖，但发布仍需处理签名、公证及通用二进制策略。[Qt for macOS - Deployment](https://doc.qt.io/qt-6/macos-deployment.html)
- **Windows**：使用独立 Windows 工具链产出并收集 Qt/插件运行时依赖；x86-64 的 MSVC 与 MinGW 构建是不同配置，Windows ARM64 还需要 MSVC，不能假设桌面二进制通用。[Supported Platforms](https://doc.qt.io/qt-6/supported-platforms.html) [Deployment](https://doc.qt.io/qt-6/cmake-deployment.html)
- **Android**：CMake/Qt Creator 最终调用 `androiddeployqt` 生成 APK、AAB 或 AAR；Google Play 分发应使用可含多 ABI 的 AAB。自定义 Manifest、Java/Kotlin、资源和第三方库通过 Android package source directory 加入。[Deploying an Application on Android](https://doc.qt.io/qt-6/deployment-android.html)
- **iOS**：只能通过 macOS/Xcode 工具链构建；CMake 可生成 Xcode project，但 Qt Creator 不覆盖所有 iOS 设置，必要时要在 Xcode 管理。真机/App Store 需要 Apple Developer Program、证书和 provisioning profile，应用是自包含 bundle。[Qt for iOS](https://doc.qt.io/qt-6/ios.html) [Platform Notes - iOS](https://doc.qt.io/qt-6/ios-platform-notes.html)

## 对后续决策的约束

以下事实已经足够支持后续架构票据采用这些约束：

1. **UI 不共享是允许且更符合 Qt 官方定位的。** Desktop Application 使用 Widgets；Mobile Application 优先使用 Qt Quick，原生 UI + Qt 内容仅作为需要逐平台原型验证的替代。共享的是领域模型、命令、工程格式和媒体能力，不是 Widget/QML 页面。
2. **视频渲染必须有后端边界。** 不再向新核心代码扩散 OpenGL 类型；核心输出逻辑帧/表面描述与同步语义，Qt/平台适配层负责 GPU 资源导入、色彩转换和呈现。
3. **QRhi 是实现工具，不是稳定领域 API。** 若采用它，锁定并升级测试 Qt 小版本，所有 `GuiPrivate` 与 `QNativeInterface` 用法集中在可替换适配器中。
4. **移动应用不是桌面 UI 的重新编译。** 生命周期、权限、素材访问、分享、签名和商店发布都必须有平台实现；后台导出与端侧 AI 尤其需要单独验证系统限制。
5. **CMake 是四端共同构建基线。** 每个平台/ABI 仍需独立 CI、真机或目标机 smoke test 和发布流水线。
6. **Qt 版本与许可证需要单独决定。** Qt 6.11 是当前支持版本；Qt 6.8 LTS 的持续补丁访问与商业许可绑定，不能在这张研究票据中默认选定。

## 尚需原型验证的风险

官方文档能证明能力存在，不能证明本项目的媒体管线达到性能与稳定性目标。实施前至少需要以下窄原型：

- macOS/Metal 与 Windows/D3D11：解码帧到预览表面的拷贝次数、YUV/RGB 转换、seek 后资源重建、设备丢失恢复；
- Android：Qt Quick 时间线触控、高 DPI、多厂商 OpenGL ES/Vulkan 驱动、Activity 暂停/恢复、长任务与后台限制；
- iOS：Qt Quick/原生素材选择、Metal 纹理互操作、内存压力、签名与真机包；
- 四端：同一 Editing Project 的序列化兼容性与同一 Baseline Export 的视觉/音画同步一致性。

这些是下一层可精确表述的研究/原型问题，不改变本报告已经明确的“共享核心 + 分平台 UI/渲染/部署适配”方向。
