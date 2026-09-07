const variants = [
  { key: "A", name: "经典三栏 · 全局可见" },
  { key: "B", name: "故事优先 · 多轨时间线" },
  { key: "C", name: "引导聚焦 · 分步工作流" },
];

const state = {
  variant: getVariantFromUrl(),
  project: "周末散步",
  dirty: true,
  recoveryAvailable: true,
  playing: false,
  selectedClip: "街角 · 片段 02",
  selectedMedia: "street.mov",
  activeSection: "素材",
  activeTool: "选择",
  playhead: "00:18:12",
  lastAction: "等待评审操作",
};

const root = document.querySelector("#prototype-root");
const variantLabel = document.querySelector("#variant-label");
const stateBody = document.querySelector("#prototype-state-body");
const toast = document.querySelector("#toast");
let toastTimer;

function getVariantFromUrl() {
  const candidate = new URLSearchParams(window.location.search).get("variant")?.toUpperCase();
  return variants.some((item) => item.key === candidate) ? candidate : "A";
}

function mediaCards() {
  const items = [
    ["street.mov", "街角  ·  00:14"],
    ["coffee.mov", "咖啡店  ·  00:22"],
    ["river.jpg", "河岸  ·  图片"],
    ["theme.m4a", "主题音乐  ·  01:48"],
  ];
  return items.map(([id, label]) => `
    <button class="media-card ${state.selectedMedia === id ? "is-selected" : ""}" data-action="select-media" data-media="${id}">
      <span class="media-card__thumb"></span><span>${label}</span>
    </button>`).join("");
}

function recoveryBanner(className = "") {
  if (!state.recoveryAvailable) return "";
  return `<div class="recovery-banner ${className}">
    <strong>发现 2 分钟前的恢复快照</strong><span>正式保存版本：今天 14:28</span>
    <button data-action="restore">恢复为副本</button><button data-action="dismiss-recovery">丢弃</button>
  </div>`;
}

function preview() {
  return `<section class="preview" aria-label="视频预览">
    <span class="preview__title">预览 · 1920 × 1080 · 30 fps</span>
    <span class="preview__caption">城市很大，生活要慢一点。</span>
    <div class="transport"><span>${state.playhead}</span><button data-action="play">${state.playing ? "Ⅱ" : "▶"}</button><span>01:24:08</span></div>
  </section>`;
}

function timeline() {
  return `<section class="timeline" aria-label="编辑时间线">
    <div class="timeline__ruler"><span>00:00</span><span>00:15</span><span>00:30</span><span>00:45</span><span>01:00</span><span>01:15</span></div>
    <div class="playhead"></div>
    <div class="timeline__lane"><span class="timeline__label">主故事线</span><div class="timeline__content">
      <button class="clip" data-action="select-clip" data-clip="开场 · 片段 01"><i class="clip__trim"></i>开场 · 12s<i class="clip__trim"></i></button>
      <button class="clip ${state.selectedClip.includes("街角") ? "is-selected" : ""}" data-action="select-clip" data-clip="街角 · 片段 02"><i class="clip__trim"></i>街角 · 26s<i class="clip__trim"></i></button>
      <button class="clip" style="flex:1" data-action="select-clip" data-clip="河岸 · 片段 03"><i class="clip__trim"></i>河岸 · 18s<i class="clip__trim"></i></button>
    </div></div>
    <div class="timeline__lane"><span class="timeline__label">背景音乐</span><div class="timeline__content"><button class="clip music" data-action="select-clip" data-clip="主题音乐 · 音乐 01">主题音乐 · -12 dB</button></div></div>
    <div class="timeline__lane"><span class="timeline__label">字幕</span><div class="timeline__content"><button class="clip caption" data-action="select-clip" data-clip="字幕段 08">城市很大</button><button class="clip caption" data-action="select-clip" data-clip="字幕段 09">生活要慢一点</button></div></div>
  </section>`;
}

function inspector(extraClass = "") {
  return `<aside class="inspector ${extraClass}" aria-label="属性区">
    <div class="inspector__header"><h2>属性</h2><span class="muted">${state.selectedClip}</span></div>
    <div class="property-group"><h3>源区间</h3>
      <div class="property-row"><span>入点</span><span class="field">00:03:14</span></div>
      <div class="property-row"><span>出点</span><span class="field">00:29:02</span></div>
      <div class="property-row"><span>时长</span><span>00:25:18</span></div>
    </div>
    <div class="property-group"><h3>画面</h3>
      <div class="property-row"><span>缩放</span><span class="field">100%</span></div>
      <div class="property-row"><span>旋转</span><span class="field">0°</span></div>
    </div>
    <div class="property-group"><h3>音频</h3>
      <div class="property-row"><span>音量</span><span class="field">0 dB</span></div>
      <div class="property-row"><span>静音</span><button class="quiet-button" data-action="generic" data-message="切换片段静音">关闭</button></div>
    </div>
  </aside>`;
}

function editButtons() {
  return `<button class="tool-button ${state.activeTool === "选择" ? "is-active" : ""}" data-action="tool" data-tool="选择">↖ 选择</button>
    <button class="tool-button" data-action="edit" data-message="在播放头分割为一个 Editing Command">✂ 分割</button>
    <button class="tool-button danger-button" data-action="edit" data-message="删除所选并让后续片段磁性贴合">⌫ 删除</button>
    <button class="tool-button" data-action="generic" data-message="撤销上一条 Editing Command">↶</button>
    <button class="tool-button" data-action="generic" data-message="重做下一条 Editing Command">↷</button>`;
}

function renderVariantA() {
  return `<div class="app-shell variant-a">
    <header class="a-topbar">
      <div class="brand"><span class="brand-mark">P</span> PlayerLab</div>
      <div class="project-title">${state.project}<small>${state.dirty ? "● 未保存" : "已保存"}</small></div>
      <div class="a-actions"><button class="quiet-button" data-action="save">保存 ⌘S</button><button class="primary-button" data-action="export">导出</button></div>
    </header>
    ${recoveryBanner()}
    <div class="a-main">
      <aside class="a-library">
        <div class="panel-heading"><h2>素材区</h2><button class="primary-button" data-action="import">＋ 导入</button></div>
        <div class="a-library__tabs"><button class="tab-button is-active">全部</button><button class="tab-button">视频</button><button class="tab-button">音频</button></div>
        <div class="media-grid">${mediaCards()}</div>
      </aside>
      <div class="a-preview-column">${preview()}<div class="a-preview-tools"><button class="quiet-button" data-action="generic" data-message="适合窗口">适合</button><button class="quiet-button" data-action="generic" data-message="切换全屏预览">全屏</button><button class="quiet-button" data-action="generic" data-message="临时关闭磁性吸附">磁性：开</button></div></div>
      ${inspector()}
    </div>
    <div><div class="a-timeline-header"><div class="a-timeline-header__group">${editButtons()}</div><div class="a-timeline-header__group"><span class="muted">缩放</span><button class="quiet-button" data-action="generic" data-message="缩小时间线">−</button><button class="quiet-button" data-action="generic" data-message="放大时间线">＋</button></div></div>${timeline()}</div>
  </div>`;
}

function drawerForSection() {
  if (state.activeSection === "字幕") return `<div class="b-drawer__heading"><p class="eyebrow">AI + MANUAL</p><h2>字幕</h2><p>字幕段随主故事线内容移动；词级时间保留在段内。</p></div>
    <button class="primary-button" data-action="generic" data-message="从语音生成可编辑字幕段">生成字幕</button>
    <div class="property-group"><h3>当前字幕段</h3><div class="field">城市很大，生活要慢一点。</div><div class="property-row"><span>时间</span><span>00:16:08 — 00:20:11</span></div></div>`;
  if (state.activeSection === "音乐") return `<div class="b-drawer__heading"><p class="eyebrow">ONE INDEPENDENT LANE</p><h2>背景音乐</h2><p>顺序放置、不可重叠；主故事线改变时位置不自动变化。</p></div>
    <div class="media-grid">${mediaCards()}</div>`;
  if (state.activeSection === "检查") return `<div class="b-drawer__heading"><p class="eyebrow">SELECTION</p><h2>片段属性</h2><p>${state.selectedClip}</p></div>${inspector()}`;
  return `<div class="b-drawer__heading"><p class="eyebrow">SOURCE MEDIA</p><h2>把故事放进来</h2><p>导入只建立引用，不更改也不复制源文件。</p></div><button class="b-import-zone" data-action="import">＋ 拖入或选择视频、图片、音频</button><div class="media-grid">${mediaCards()}</div>`;
}

function renderVariantB() {
  const sections = [["素材", "▦"], ["音乐", "♫"], ["字幕", "字"], ["检查", "⌁"]];
  return `<div class="app-shell variant-b">
    <header class="b-topbar"><div class="brand"><span class="brand-mark">P</span> PlayerLab</div><div class="b-topbar__project"><strong>${state.project}</strong> <span>／${state.dirty ? "有未保存更改" : "已保存"}</span></div><button class="quiet-button" data-action="save">保存</button><button class="primary-button" data-action="export">完成并导出</button></header>
    ${recoveryBanner("b-recovery")}
    <nav class="b-rail">${sections.map(([name, icon]) => `<button class="${state.activeSection === name ? "is-active" : ""}" data-action="section" data-section="${name}"><b>${icon}</b><span>${name}</span></button>`).join("")}</nav>
    <aside class="b-drawer">${drawerForSection()}</aside>
    <div class="b-workspace"><div style="position:relative">${preview()}<aside class="b-context-popover"><p class="eyebrow">当前片段</p><h3>${state.selectedClip}</h3><div class="b-context-actions">${editButtons()}</div></aside></div>
      <section class="b-multitrack"><div class="b-multitrack__header"><div><h2>多轨道时间线</h2><span>主故事线磁性贴合 · 背景音乐与字幕同时可见</span></div><div class="b-multitrack__zoom"><span>缩放</span><button class="quiet-button" data-action="generic" data-message="缩小时间线">−</button><button class="quiet-button" data-action="generic" data-message="放大时间线">＋</button></div></div>${timeline()}</section>
    </div>
    <footer class="b-statusbar"><span><span class="status-dot"></span>素材引用均在线 · 恢复快照刚刚更新</span><span>磁性吸附 开 · 按住 ⌥ 临时关闭</span></footer>
  </div>`;
}

function renderVariantC() {
  const steps = ["导入", "剪辑", "字幕", "导出"];
  return `<div class="app-shell variant-c">
    <header class="c-header"><div class="brand"><span class="brand-mark">P</span> PlayerLab</div><div class="c-header__middle"><nav class="stepper">${steps.map((step, index) => `<button class="step ${step === "剪辑" ? "is-active" : ""}" data-action="step" data-step="${step}"><span class="step__number">${index + 1}</span>${step}</button>`).join("")}</nav></div><div class="c-header__actions"><button class="quiet-button" data-action="save">${state.dirty ? "保存更改" : "已保存 ✓"}</button><button class="primary-button" data-action="export">导出</button></div></header>
    ${recoveryBanner("c-recovery")}
    <div class="c-body">
      <aside class="c-tasks"><p class="eyebrow">当前步骤 · 剪辑</p><h2>完成你的故事</h2><div class="task-list">
        <button class="task-item" data-action="import"><span>＋</span><span><strong>添加素材</strong><small>视频、图片、音频</small></span></button>
        <button class="task-item is-active" data-action="tool" data-tool="选择"><span>↔</span><span><strong>整理顺序</strong><small>拖动卡片，自动贴合</small></span></button>
        <button class="task-item" data-action="edit" data-message="拖动两端裁剪，松手记为一条 Editing Command"><span>◫</span><span><strong>裁剪片段</strong><small>拖动片段两端</small></span></button>
        <button class="task-item" data-action="edit" data-message="在播放头分割为一个 Editing Command"><span>✂</span><span><strong>分割片段</strong><small>从当前位置切开</small></span></button>
        <button class="task-item" data-action="section" data-section="音乐"><span>♫</span><span><strong>背景音乐</strong><small>1 条独立音乐轨</small></span></button>
        <button class="task-item" data-action="section" data-section="字幕"><span>字</span><span><strong>字幕</strong><small>9 段，1 段待校对</small></span></button>
      </div></aside>
      <section class="c-canvas">${preview()}<div class="c-canvas__bottom"><div class="c-canvas__toolbar"><strong>主故事线 · 3 个片段</strong><div>${editButtons()}</div></div><div class="c-storyline"><button class="c-story-card" data-action="select-clip" data-clip="开场 · 片段 01"><strong>01 · 开场</strong><small>00:00—00:12</small></button><button class="c-story-card is-selected" data-action="select-clip" data-clip="街角 · 片段 02"><strong>02 · 街角</strong><small>00:12—00:38</small></button><button class="c-story-card" data-action="select-clip" data-clip="河岸 · 片段 03"><strong>03 · 河岸</strong><small>00:38—00:56</small></button></div><div class="c-track-summary"><div class="track-pill music">♫ 背景音乐已添加</div><div class="track-pill caption">字 字幕跟随内容</div></div></div></section>
      ${inspector("c-inspector")}
    </div>
  </div>`;
}

function render() {
  const renderers = { A: renderVariantA, B: renderVariantB, C: renderVariantC };
  root.innerHTML = renderers[state.variant]();
  const meta = variants.find((item) => item.key === state.variant);
  variantLabel.textContent = `${meta.key} · ${meta.name}`;
  renderState();
}

function renderState() {
  const rows = [
    ["方案", state.variant],
    ["工程", state.project],
    ["保存", state.dirty ? "有未保存更改" : "已保存"],
    ["恢复快照", state.recoveryAvailable ? "待处理" : "已处理"],
    ["当前区", state.activeSection],
    ["当前工具", state.activeTool],
    ["所选片段", state.selectedClip],
    ["最近动作", state.lastAction],
  ];
  stateBody.innerHTML = rows.map(([key, value]) => `<span>${key}</span><span title="${value}">${value}</span>`).join("");
}

function setVariant(direction) {
  const index = variants.findIndex((item) => item.key === state.variant);
  state.variant = variants[(index + direction + variants.length) % variants.length].key;
  const url = new URL(window.location.href);
  url.searchParams.set("variant", state.variant);
  window.history.replaceState({}, "", url);
  state.lastAction = `切换到方案 ${state.variant}`;
  render();
}

function showToast(message) {
  window.clearTimeout(toastTimer);
  toast.textContent = message;
  toast.classList.add("is-visible");
  toastTimer = window.setTimeout(() => toast.classList.remove("is-visible"), 1800);
}

function markEdited(message) {
  state.dirty = true;
  state.lastAction = message;
  showToast(message);
  render();
}

function openExportDialog() {
  const backdrop = document.createElement("div");
  backdrop.className = "dialog-backdrop";
  backdrop.innerHTML = `<section class="dialog" role="dialog" aria-modal="true" aria-label="导出视频"><p class="eyebrow">BASELINE EXPORT</p><h2>导出 MP4</h2><p>使用首版可靠交付配置。所有使用中的素材当前均在线。</p><div class="dialog__options"><div class="dialog__option"><span>格式</span><strong>MP4 · H.264 + AAC</strong></div><div class="dialog__option"><span>分辨率</span><strong>1080p</strong></div><div class="dialog__option"><span>帧率</span><strong>跟随主素材 · 30 fps</strong></div></div><div class="dialog__actions"><button class="quiet-button" data-action="close-dialog">取消</button><button class="primary-button" data-action="confirm-export">选择位置并导出</button></div></section>`;
  document.body.append(backdrop);
}

document.addEventListener("click", (event) => {
  const target = event.target.closest("[data-action]");
  if (!target) return;
  const action = target.dataset.action;

  if (action === "previous-variant") return setVariant(-1);
  if (action === "next-variant") return setVariant(1);
  if (action === "toggle-state") {
    stateBody.hidden = !stateBody.hidden;
    target.setAttribute("aria-expanded", String(!stateBody.hidden));
    return;
  }
  if (action === "play") {
    state.playing = !state.playing;
    state.lastAction = state.playing ? "开始预览" : "暂停预览";
    return render();
  }
  if (action === "save") {
    state.dirty = false;
    state.lastAction = "正式保存 Project File";
    showToast("工程已原子保存；Undo/Redo 历史不写入工程");
    return render();
  }
  if (action === "restore") {
    state.project = "周末散步（已恢复）";
    state.recoveryAvailable = false;
    state.dirty = true;
    state.lastAction = "从 Recovery Snapshot 恢复为副本";
    showToast("已恢复为副本，没有覆盖正式保存版本");
    return render();
  }
  if (action === "dismiss-recovery") {
    state.recoveryAvailable = false;
    state.lastAction = "丢弃 Recovery Snapshot";
    return render();
  }
  if (action === "select-media") {
    state.selectedMedia = target.dataset.media;
    state.lastAction = `选择 Source Media：${target.dataset.media}`;
    return render();
  }
  if (action === "select-clip") {
    state.selectedClip = target.dataset.clip;
    state.lastAction = `选择 Clip：${target.dataset.clip}`;
    return render();
  }
  if (action === "section") {
    state.activeSection = target.dataset.section;
    state.lastAction = `打开${state.activeSection}工具`;
    return render();
  }
  if (action === "tool") {
    state.activeTool = target.dataset.tool;
    state.lastAction = `切换到${state.activeTool}工具`;
    return render();
  }
  if (action === "step") {
    state.lastAction = `检查${target.dataset.step}步骤的信息架构`;
    return showToast(`${target.dataset.step}步骤在此原型中只展示入口`);
  }
  if (action === "import") return markEdited("导入 Source Media 引用（原型未读取文件）");
  if (action === "edit") return markEdited(target.dataset.message);
  if (action === "generic") {
    state.lastAction = target.dataset.message;
    showToast(target.dataset.message);
    return renderState();
  }
  if (action === "export") return openExportDialog();
  if (action === "close-dialog") return target.closest(".dialog-backdrop").remove();
  if (action === "confirm-export") {
    target.closest(".dialog-backdrop").remove();
    state.lastAction = "开始 Baseline Export";
    showToast("将导出 1080p H.264/AAC MP4");
    return renderState();
  }
});

document.addEventListener("keydown", (event) => {
  const editable = event.target.closest("input, textarea, [contenteditable]");
  if (editable) return;
  if (event.key === "ArrowLeft") setVariant(-1);
  if (event.key === "ArrowRight") setVariant(1);
});

window.addEventListener("popstate", () => {
  state.variant = getVariantFromUrl();
  render();
});

render();
