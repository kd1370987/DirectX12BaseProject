# -*- coding: utf-8 -*-
"""
DirectX12BaseProject の依存関係図を .excalidraw で書き出す。

色は Excalidraw の「ライトモードの値」で書く。
Excalidraw のダークモードは描画時に色を反転させるので、
ライトモードの黒/緑で持たせておくと、ダークモードで開いたときに
見本どおり「黒背景・白枠/緑枠」に見える。
"""
import json, os, math

XP, YP = 300.0, 150.0
FS = 16          # ノードの文字サイズ
LH = 1.25

C_SING   = "#2f9e44"   # シングルトン(緑)
C_CLS    = "#1e1e1e"   # 通常クラス(白枠として見える)
C_TXT    = "#1e1e1e"
C_STRUCT = "#1e1e1e"
C_IMP    = "#e8590c"   # 改善点(注記)

_seed = [1000]
def nid(p):
    _seed[0] += 1
    return "%s%04d" % (p, _seed[0])

def tw(s):
    w = 0.0
    for ch in s:
        w += 16.0 if ord(ch) > 0x2E80 else 8.6
    return w

def text_size(label, fs=FS):
    lines = label.split("\n")
    return max(tw(l) for l in lines) * (fs / FS), len(lines) * fs * LH

class Sheet:
    def __init__(self, title, subtitle=""):
        self.title = title
        self.subtitle = subtitle
        self.nodes = {}
        self.edges = []
        self.notes = []
        self.improve = []   # 右下に出す改善点

    def n(self, key, label, col, row, kind="cls", w=None):
        twd, thd = text_size(label)
        width = w if w else max(150.0, twd + 34.0)
        height = thd + 30.0
        self.nodes[key] = dict(key=key, label=label, kind=kind,
                               cx=col * XP, cy=row * YP, w=width, h=height,
                               id=nid("r"), tid=nid("t"), bound=[])
        return key

    def e(self, a, b, kind="sing"):
        # kind: sing(緑) / own(白実線・所有) / ref(白破線・引数参照) / inherit(白点線・継承)
        self.edges.append((a, b, kind))

    def note(self, text, col, row, fs=20):
        self.notes.append((text, col * XP, row * YP, fs))

    def imp(self, *items):
        """改善点。1項目 = 見出し + 本文の行リスト"""
        self.improve.extend(items)

    def bbox(self):
        xs, ys = [], []
        for n in self.nodes.values():
            xs += [n["cx"] - n["w"] / 2, n["cx"] + n["w"] / 2]
            ys += [n["cy"] - n["h"] / 2, n["cy"] + n["h"] / 2]
        for text, x, y, fs in self.notes:
            xs.append(x + text_size(text, fs)[0])
            ys.append(y + text_size(text, fs)[1])
        return min(xs), min(ys), max(xs), max(ys)

    def improve_lines(self):
        """改善点パネルの表示行を組み立てる"""
        out = []
        for i, (head, body) in enumerate(self.improve):
            if i:
                out.append(("", False))
            out.append(("%d. %s" % (i + 1, head), True))
            for ln in body:
                out.append(("    " + ln, False))
        return out

    def improve_box(self):
        """(x, y, w, h, 行) を返す。無ければ None"""
        if not self.improve:
            return None
        lines = self.improve_lines()
        w = max(text_size(t)[0] for t, _ in lines) + 84
        h = len(lines) * FS * LH + 76
        _, _, maxx, maxy = self.bbox()
        return maxx - w, maxy + 130, w, h, lines


def base(el, **kw):
    d = dict(isDeleted=False, fillStyle="solid", strokeWidth=2, strokeStyle="solid",
             roughness=1, opacity=100, angle=0, backgroundColor="transparent",
             seed=_seed[0] * 7 + 13, version=1, versionNonce=_seed[0] * 31 + 7,
             updated=1, link=None, locked=False, frameId=None, groupIds=[],
             boundElements=None, roundness=None)
    d.update(el); d.update(kw)
    return d


def clip(cx, cy, w, h, tx, ty, gap=6.0):
    """(cx,cy) の箱の枠のうち (tx,ty) 方向の点を返す"""
    dx, dy = tx - cx, ty - cy
    if dx == 0 and dy == 0:
        return cx, cy
    hw, hh = w / 2.0 + gap, h / 2.0 + gap
    sx = hw / abs(dx) if dx else 1e9
    sy = hh / abs(dy) if dy else 1e9
    s = min(sx, sy)
    return cx + dx * s, cy + dy * s


def build(sheet):
    els = []
    # --- タイトル ---
    els.append(base(dict(
        type="text", id=nid("T"), x=-XP * 0.9, y=-YP * 1.9, width=text_size(sheet.title, 36)[0],
        height=36 * LH, strokeColor=C_TXT, text=sheet.title, fontSize=36, fontFamily=1,
        textAlign="left", verticalAlign="top", containerId=None,
        originalText=sheet.title, lineHeight=LH, autoResize=True)))
    if sheet.subtitle:
        els.append(base(dict(
            type="text", id=nid("T"), x=-XP * 0.9, y=-YP * 1.9 + 50, width=text_size(sheet.subtitle, 16)[0],
            height=16 * LH, strokeColor=C_TXT, text=sheet.subtitle, fontSize=16, fontFamily=1,
            textAlign="left", verticalAlign="top", containerId=None,
            originalText=sheet.subtitle, lineHeight=LH, autoResize=True)))

    # --- 矢印(先に作って boundElements を集める) ---
    arrows = []
    for a, b, kind in sheet.edges:
        na, nb = sheet.nodes[a], sheet.nodes[b]
        sx, sy = clip(na["cx"], na["cy"], na["w"], na["h"], nb["cx"], nb["cy"])
        ex, ey = clip(nb["cx"], nb["cy"], nb["w"], nb["h"], na["cx"], na["cy"])
        aid = nid("a")
        col = C_SING if kind == "sing" else C_CLS
        style = {"sing": "solid", "own": "solid", "ref": "dashed", "inherit": "dotted"}[kind]
        arrows.append(base(dict(
            type="arrow", id=aid, x=sx, y=sy, width=abs(ex - sx), height=abs(ey - sy),
            points=[[0, 0], [ex - sx, ey - sy]], strokeColor=col, strokeStyle=style,
            strokeWidth=1.5 if kind != "own" else 2,
            startArrowhead=None, endArrowhead="arrow", elbowed=False,
            startBinding=dict(elementId=na["id"], focus=0.0, gap=6),
            endBinding=dict(elementId=nb["id"], focus=0.0, gap=6),
            roundness=dict(type=2))))
        na["bound"].append(dict(id=aid, type="arrow"))
        nb["bound"].append(dict(id=aid, type="arrow"))

    # --- 箱 ---
    for n in sheet.nodes.values():
        col = C_SING if n["kind"] == "sing" else C_CLS
        style = "dashed" if n["kind"] == "struct" else "solid"
        bg = "transparent"
        els.append(base(dict(
            type="rectangle", id=n["id"], x=n["cx"] - n["w"] / 2, y=n["cy"] - n["h"] / 2,
            width=n["w"], height=n["h"], strokeColor=col, backgroundColor=bg,
            strokeStyle=style, roundness=dict(type=3),
            boundElements=n["bound"] + [dict(id=n["tid"], type="text")])))
        twd, thd = text_size(n["label"])
        els.append(base(dict(
            type="text", id=n["tid"], x=n["cx"] - twd / 2, y=n["cy"] - thd / 2,
            width=twd, height=thd, strokeColor=col, text=n["label"], fontSize=FS,
            fontFamily=1, textAlign="center", verticalAlign="middle",
            containerId=n["id"], originalText=n["label"], lineHeight=LH, autoResize=True)))

    els += arrows

    # --- メモ ---
    for text, x, y, fs in sheet.notes:
        twd, thd = text_size(text, fs)
        els.append(base(dict(
            type="text", id=nid("m"), x=x, y=y, width=twd, height=thd,
            strokeColor=C_TXT, text=text, fontSize=fs, fontFamily=1,
            textAlign="left", verticalAlign="top", containerId=None,
            originalText=text, lineHeight=LH, autoResize=True)))

    # --- 改善点(右下) ---
    box = sheet.improve_box()
    if box:
        bx, by, bw, bh, lines = box
        els.append(base(dict(
            type="rectangle", id=nid("i"), x=bx, y=by, width=bw, height=bh,
            strokeColor=C_IMP, backgroundColor="transparent", strokeStyle="solid",
            strokeWidth=2, roundness=dict(type=3))))
        title = "改善点"
        els.append(base(dict(
            type="text", id=nid("i"), x=bx + 26, y=by + 20, width=text_size(title, 24)[0],
            height=24 * LH, strokeColor=C_IMP, text=title, fontSize=24, fontFamily=1,
            textAlign="left", verticalAlign="top", containerId=None,
            originalText=title, lineHeight=LH, autoResize=True)))
        # 見出し行と本文行で色を変えるので、続く同種の行をまとめて1要素にする
        y0 = by + 62
        i = 0
        while i < len(lines):
            head = lines[i][1]
            run = []
            while i < len(lines) and lines[i][1] == head:
                run.append(lines[i][0]); i += 1
            body = "\n".join(run)
            twd, thd = text_size(body)
            els.append(base(dict(
                type="text", id=nid("i"), x=bx + 26, y=y0, width=twd, height=thd,
                strokeColor=C_IMP if head else C_TXT, text=body, fontSize=FS,
                fontFamily=1, textAlign="left", verticalAlign="top", containerId=None,
                originalText=body, lineHeight=LH, autoResize=True)))
            y0 += thd
    return els


def legend(x, y):
    """凡例。各シート共通"""
    els = []
    rows = [
        (C_SING, "solid", "rect", "シングルトン (Instance() を持つクラス)"),
        (C_CLS,  "solid", "rect", "通常クラス"),
        (C_CLS,  "dashed", "rect", "構造体(引数で配るコンテキスト)"),
        (C_SING, "solid", "arrow", "シングルトン経由の依存 (X::Instance() を呼ぶ)"),
        (C_CLS,  "solid", "arrow", "所有 (unique_ptr / 値メンバ)"),
        (C_CLS,  "dashed", "arrow", "参照 (引数・ポインタで受け取る)"),
        (C_CLS,  "dotted", "arrow", "継承"),
    ]
    els.append(base(dict(
        type="text", id=nid("L"), x=x, y=y - 34, width=60, height=20,
        strokeColor=C_TXT, text="凡例", fontSize=20, fontFamily=1,
        textAlign="left", verticalAlign="top", containerId=None,
        originalText="凡例", lineHeight=LH, autoResize=True)))
    for i, (col, style, shape, label) in enumerate(rows):
        yy = y + i * 40
        if shape == "rect":
            els.append(base(dict(
                type="rectangle", id=nid("l"), x=x, y=yy, width=70, height=26,
                strokeColor=col, strokeStyle=style, roundness=dict(type=3))))
        else:
            els.append(base(dict(
                type="arrow", id=nid("l"), x=x, y=yy + 13, width=70, height=0,
                points=[[0, 0], [70, 0]], strokeColor=col, strokeStyle=style,
                strokeWidth=1.5, startArrowhead=None, endArrowhead="arrow",
                elbowed=False, startBinding=None, endBinding=None)))
        els.append(base(dict(
            type="text", id=nid("l"), x=x + 86, y=yy + 4, width=text_size(label)[0],
            height=20, strokeColor=C_TXT, text=label, fontSize=FS, fontFamily=1,
            textAlign="left", verticalAlign="top", containerId=None,
            originalText=label, lineHeight=LH, autoResize=True)))
    return els


def dump(sheet, path, legend_pos):
    els = build(sheet) + legend(*legend_pos)
    doc = dict(type="excalidraw", version=2, source="https://excalidraw.com",
               elements=els, appState=dict(gridSize=None, viewBackgroundColor="#ffffff"),
               files={})
    with open(path, "w", encoding="utf-8") as f:
        json.dump(doc, f, ensure_ascii=False, indent=1)
    print(path, len(sheet.nodes), "nodes /", len(sheet.edges), "edges")


# =====================================================================
# 1. 全体俯瞰
# =====================================================================
s1 = Sheet("① 全体俯瞰 — アプリケーションとシングルトン",
           "2026-09-15 時点。D3D12Wrapper / DescriptorHeapManager / AssetDatabase はシングルトンを外れ、所有ツリーの中に入った。")
s1.n("App", "Application", 3, 0)
s1.n("MainEngine", "MainEngine", 3, 1, "sing")
# MainEngine が所有
s1.n("ResourceManager", "ResourceManager\n(Instance() は ResourceRef 用の入口)", -0.3, 2.1, "sing")
s1.n("NativeWindow", "NativeWindow", 1.2, 2.1)
s1.n("TimeManager", "TimeManager", 2.2, 2.1)
s1.n("GraphicsEngine", "GraphicsEngine", 3.6, 2.1)
s1.n("ParticleBufferManager", "ParticleBufferManager", 4.9, 2.1)
s1.n("JobSystem", "JobSystem", 6.1, 2.1)
s1.n("MouseCursor", "MouseCursor", 7.1, 2.1)
s1.n("EngineServices", "EngineServices\n(アプリ寿命の置き場・正本)", 8.5, 2.1, "struct")
# 所有の2段目
s1.n("AssetDatabase", "AssetDatabase", -0.3, 3.2)
s1.n("GraphicsDevice", "GraphicsDevice", 2.9, 3.2)
s1.n("DescriptorHeapManager", "DescriptorHeapManager", 4.2, 3.2)
s1.n("DebugDraw", "DebugDraw", 5.5, 3.2)
# 残っているシングルトン
s1.n("InputManager", "InputManager", 0.4, 4.4, "sing")
s1.n("OptionManager", "OptionManager", 1.6, 4.4, "sing")
s1.n("AudioManager", "AudioManager", 2.8, 4.4, "sing")
s1.n("RayEngine", "RayEngine", 4.0, 4.4, "sing")
s1.n("MainEditor", "MainEditor", 5.3, 4.4, "sing")
s1.n("ObjectMetaRegistry", "ObjectMetaRegistry", 6.8, 4.4, "sing")
# ゲーム/シーン
s1.n("GameManager", "GameManager", 0.8, 5.6, "sing")
s1.n("SceneManager", "SceneManager", 3.4, 5.6, "sing")
s1.n("UserData", "UserData", 0.0, 6.6)
s1.n("InputActionManager", "InputActionManager", 1.3, 6.6)
s1.n("BaseScene", "BaseScene", 3.9, 6.6)
s1.n("World", "World", 3.1, 7.6)
s1.n("GameObjectManager", "GameObjectManager", 4.8, 7.6)

for t in ["MainEngine", "GameManager", "SceneManager", "MainEditor", "InputManager"]:
    s1.e("App", t)
for t in ["AudioManager", "InputManager", "MainEditor", "OptionManager", "RayEngine"]:
    s1.e("MainEngine", t)
s1.e("MainEditor", "MainEngine")
for t in ["AudioManager", "MainEditor", "MainEngine"]:
    s1.e("SceneManager", t)
for t in ["MainEngine", "SceneManager"]:
    s1.e("BaseScene", t)
for t in ["MainEditor", "MainEngine", "ObjectMetaRegistry", "SceneManager"]:
    s1.e("GameManager", t)
for t in ["MainEngine", "OptionManager"]:
    s1.e("InputManager", t)
s1.e("NativeWindow", "InputManager")
for t in ["InputManager", "MainEngine", "OptionManager"]:
    s1.e("MouseCursor", t)
for t in ["GameManager", "MainEngine", "OptionManager", "SceneManager"]:
    s1.e("GraphicsEngine", t)
for t in ["MainEngine", "AudioManager", "InputManager"]:
    s1.e("OptionManager", t)
s1.e("GameObjectManager", "ObjectMetaRegistry")
s1.e("InputActionManager", "InputManager")
s1.e("InputActionManager", "MainEditor")
s1.e("DebugDraw", "OptionManager")
for t in ["ResourceManager", "NativeWindow", "TimeManager", "GraphicsEngine",
          "ParticleBufferManager", "JobSystem", "MouseCursor", "EngineServices"]:
    s1.e("MainEngine", t, "own")
s1.e("ResourceManager", "AssetDatabase", "own")
for t in ["GraphicsDevice", "DescriptorHeapManager", "DebugDraw"]:
    s1.e("GraphicsEngine", t, "own")
s1.e("SceneManager", "BaseScene", "own")
s1.e("BaseScene", "World", "own")
s1.e("BaseScene", "GameObjectManager", "own")
s1.e("GameManager", "UserData", "own")
s1.e("GameManager", "InputActionManager", "own")
s1.e("GraphicsEngine", "ResourceManager", "ref")
s1.e("AudioManager", "ResourceManager", "ref")
s1.e("RayEngine", "ResourceManager", "ref")
s1.note("シングルトンは 13 個 → 10 個。Instance() の呼び出しは 629 回 / 142 ファイル → 205 回 / 62 ファイル。\n"
        "GraphicsEngine の持ち物は ② と ⑥、EngineServices が配る先は ③ に載せた。", -0.9, 8.7, 16)

# =====================================================================
# 2. Graphics
# =====================================================================
s2 = Sheet("② Graphics — 描画エンジンとレンダーグラフ",
           "GraphicsEngine がデバイスからパイプラインまで全部持つ。パスは PassContext だけを見て、Instance() を呼ばない。")
s2.n("MainEngine", "MainEngine", 1.5, 0, "sing")
s2.n("OptionManager", "OptionManager", 4.4, 0, "sing")
s2.n("SceneManager", "SceneManager", 5.6, 0, "sing")
s2.n("GameManager", "GameManager", 6.8, 0, "sing")
s2.n("RayEngine", "RayEngine", 8.0, 0, "sing")
s2.n("InputManager", "InputManager", 9.2, 0, "sing")
s2.n("ResourceManager", "ResourceManager", -0.5, 1.1, "sing")
s2.n("GraphicsEngine", "GraphicsEngine", 2.6, 1.1)
s2.n("ParticleBufferManager", "ParticleBufferManager", 6.4, 1.1)
s2.n("MouseCursor", "MouseCursor", 8.7, 1.1)
# GraphicsEngine が所有(D3D12 まわり。詳細は ⑥)
s2.n("GraphicsDevice", "GraphicsDevice", -0.6, 2.3)
s2.n("CommandContext", "CommandContext", 0.5, 2.3)
s2.n("FrameManager", "FrameManager", 1.6, 2.3)
s2.n("AsyncGPUManager", "AsyncGPUManager", 2.7, 2.3)
s2.n("DescriptorHeapManager", "DescriptorHeapManager", 4.0, 2.3)
s2.n("BackBuffer", "BackBuffer", 5.2, 2.3)
s2.n("PipelineStateManager", "PipelineStateManager", 6.4, 2.3)
s2.n("GPUParticlePool", "GPUParticlePool", 7.9, 2.3)
s2.n("ParticleSimulation", "ParticleSimulation", 9.2, 2.3)
# GraphicsEngine が所有(描画データ)
s2.n("RenderContext", "RenderContext\n(フレーム数だけ)", -0.3, 3.4)
s2.n("MeshBufferAllocator", "MeshBufferAllocator", 1.0, 3.4)
s2.n("PassMetaRegistry", "PassMetaRegistry", 2.3, 3.4)
s2.n("LightManager", "LightManager", 3.4, 3.4)
s2.n("QuadPolygon", "QuadPolygon", 4.4, 3.4)
s2.n("DebugDraw", "DebugDraw", 5.4, 3.4)
s2.n("DrawLists", "DrawLists", 6.4, 3.4)
s2.n("FrameCompute", "SkinningPass /\nUpdateBLASPass", 9.2, 3.4)
s2.n("CBAllocator", "CBAllocator", -0.3, 4.5)
s2.n("CameraPipelineData", "CameraPipelineData\n(カメラ1台ぶん)", 3.0, 4.5)
s2.n("RenderingPipelineAsset", "RenderingPipelineAsset\n(設計図)", 5.6, 4.5)
s2.n("GraphicsPipeline", "GraphicsPipeline\n(実行インスタンス)", 3.0, 5.5)
s2.n("RenderGraphCompiler", "RenderGraphCompiler", 1.3, 6.5)
s2.n("RenderGraph", "RenderGraph", 3.0, 6.5)
s2.n("ResourceRegistry", "ResourceRegistry", 4.7, 6.0)
s2.n("ResourceAllocator", "ResourceAllocator", 5.0, 6.9)
s2.n("GraphHeap", "GraphHeap", 4.2, 7.6)
s2.n("AliasingReport", "AliasingReport", 1.2, 5.6)
s2.n("Pass", "Pass (基底)", 3.0, 7.5)
s2.n("VirtualResource", "VirtualResource", 6.3, 6.0)
s2.n("ShadingPipelineBuilder", "ShadingPipelineBuilder", 1.6, 8.1)
s2.n("PassContext", "PassContext\n(組むのは RenderGraph::MakeContext だけ)", 0.4, 7.7, "struct")
s2.n("GBufferPass", "GBufferPass", 0.0, 8.9)
s2.n("ZPrePass", "ZPrePass", 1.1, 8.9)
s2.n("DeferredLightingPass", "DeferredLightingPass", 2.3, 8.9)
s2.n("RaytracingGIPass", "RaytracingGIPass", 3.6, 8.9)
s2.n("TAAPass", "TAAPass", 4.6, 8.9)
s2.n("ToneMapPass", "ToneMapPass", 5.5, 8.9)
s2.n("UIPass", "UIPass", 6.4, 8.9)
s2.n("FinalOutputPass", "FinalOutputPass", 7.5, 8.9)
s2.n("MonitorPass", "MonitorPass", 8.6, 8.9)

# シングルトン直引き(残っているもの)
for t in ["GameManager", "MainEngine", "OptionManager", "SceneManager"]:
    s2.e("GraphicsEngine", t)
s2.e("RenderGraph", "MainEngine")
s2.e("RenderGraph", "RayEngine")
s2.e("ParticleSimulation", "MainEngine")
s2.e("GPUParticlePool", "MainEngine")
for t in ["InputManager", "MainEngine", "OptionManager"]:
    s2.e("MouseCursor", t)
s2.e("DebugDraw", "OptionManager")
# 所有
s2.e("MainEngine", "GraphicsEngine", "own")
s2.e("MainEngine", "ParticleBufferManager", "own")
s2.e("MainEngine", "MouseCursor", "own")
for t in ["GraphicsDevice", "CommandContext", "FrameManager", "AsyncGPUManager",
          "DescriptorHeapManager", "BackBuffer", "PipelineStateManager",
          "RenderContext", "MeshBufferAllocator", "PassMetaRegistry", "LightManager",
          "QuadPolygon", "DebugDraw", "DrawLists", "CameraPipelineData"]:
    s2.e("GraphicsEngine", t, "own")
s2.e("ParticleBufferManager", "GPUParticlePool", "own")
s2.e("RenderContext", "CBAllocator", "own")
s2.e("CameraPipelineData", "GraphicsPipeline", "own")
s2.e("GraphicsPipeline", "RenderGraph", "own")
for t in ["Pass", "ResourceRegistry", "ResourceAllocator", "GraphHeap"]:
    s2.e("RenderGraph", t, "own")
s2.e("ResourceRegistry", "VirtualResource", "own")
s2.e("Pass", "ShadingPipelineBuilder", "own")
s2.e("ResourceManager", "RenderingPipelineAsset", "own")
# 参照
s2.e("GraphicsEngine", "ResourceManager", "ref")
s2.e("GraphicsEngine", "ParticleSimulation", "ref")
s2.e("GraphicsEngine", "FrameCompute", "ref")
s2.e("RenderingPipelineAsset", "RenderGraph", "ref")
s2.e("RenderGraphCompiler", "RenderGraph", "ref")
s2.e("AliasingReport", "RenderGraph", "ref")
s2.e("RenderContext", "DescriptorHeapManager", "ref")
s2.e("RenderContext", "PipelineStateManager", "ref")
s2.e("ParticleBufferManager", "GraphicsEngine", "ref")
s2.e("ParticleBufferManager", "DescriptorHeapManager", "ref")
s2.e("MouseCursor", "DescriptorHeapManager", "ref")
s2.e("Pass", "PassContext", "ref")
for t in ["RenderGraph", "GraphicsEngine", "RenderContext", "ResourceManager",
          "DescriptorHeapManager", "ParticleBufferManager", "RayEngine", "MainEngine"]:
    s2.e("PassContext", t, "ref")
for p in ["GBufferPass", "ZPrePass", "DeferredLightingPass", "RaytracingGIPass",
          "TAAPass", "ToneMapPass", "UIPass", "FinalOutputPass", "MonitorPass"]:
    s2.e(p, "Pass", "inherit")
s2.note("パスは 30 種類。以前は GBufferPass など 8 種類がシングルトンを直接引いていたが、今は 0。\n"
        "ここに並べたのは代表だけ。", -0.9, 9.9, 16)

# =====================================================================
# 3. Scene / ECS / GameObject
# =====================================================================
s3 = Sheet("③ Scene / ECS / GameObject — シーンの中身",
           "EngineServices は MainEngine が正本を持ち、World が写しを持つ。System / GameObject はそこから引く。")
s3.n("SceneManager", "SceneManager", 3.0, 0, "sing")
s3.n("GameManager", "GameManager", 8.6, 0, "sing")
s3.n("BaseScene", "BaseScene", 3.0, 1)
s3.n("World", "World", 1.6, 2.2)
s3.n("GameObjectManager", "GameObjectManager", 5.2, 2.2)
s3.n("ObjectMetaRegistry", "ObjectMetaRegistry", 7.2, 2.2, "sing")
s3.n("EntityManager", "EntityManager", 0.0, 3.3)
s3.n("SystemManager", "SystemManager", 1.2, 3.3)
s3.n("ArchetypeChunkManager", "ArchetypeChunkManager", 2.5, 3.3)
s3.n("ComponentMetaRegistry", "ComponentMetaRegistry", 3.9, 3.3)
s3.n("BaseObject", "BaseObject", 5.6, 3.3)
s3.n("Prefab", "Prefab", 7.2, 3.4)
s3.n("ResourceWrapper", "IResourceWrapper\n(ワールド寿命のリソース)", -0.2, 4.3)
s3.n("ISystem", "ISystem", 0.8, 4.4)
s3.n("CollisionWorld", "CollisionWorld", -0.2, 5.4)
s3.n("SystemContext", "SystemContext\n(pWorld / pServices / dt)", 1.2, 5.4, "struct")
s3.n("ObjectContext", "ObjectContext\n(pWorld / pServices / pObjectManager)", 5.4, 5.4, "struct")
s3.n("EngineServices", "EngineServices\n(アプリ寿命の置き場)", 3.4, 6.4, "struct")
# EngineServices が配る先
s3.n("ResourceManager", "ResourceManager", 0.2, 7.6, "sing")
s3.n("AssetDatabase", "AssetDatabase", 1.4, 7.6)
s3.n("InputManager", "InputManager", 2.5, 7.6, "sing")
s3.n("RayEngine", "RayEngine", 3.5, 7.6, "sing")
s3.n("JobSystem", "JobSystem", 4.5, 7.6)
s3.n("DebugDraw", "DebugDraw", 5.5, 7.6)
s3.n("MainEngine", "MainEngine", 9.0, 7.6, "sing")
s3.n("OptionManager", "OptionManager", 6.6, 7.6, "sing")
s3.n("AudioManager", "AudioManager", 7.8, 7.6, "sing")
# アプリ側でまだ直引きしているもの
s3.n("AppFollow", "FollowTargetComponent /\nAttachmentSlotsComponent", 10.6, 0.9)
s3.n("AppSequence", "Sequence 群\n(Title / Pause / Result / Scene / MissionSelect)", 10.6, 3.1)
s3.n("AppScore", "ScoreSystem / ScoreHUD", 10.6, 2.0)
s3.n("AppCamera", "CameraStartSystem", 10.6, 5.2)
s3.n("AppSound", "SoundComponent / HitSoundComponent /\nFlyingSoundResource", 10.6, 6.4)

s3.e("SceneManager", "BaseScene", "own")
s3.e("BaseScene", "World", "own")
s3.e("BaseScene", "GameObjectManager", "own")
for t in ["EntityManager", "SystemManager", "ArchetypeChunkManager",
          "ComponentMetaRegistry", "ResourceWrapper"]:
    s3.e("World", t, "own")
s3.e("World", "EngineServices", "own")
s3.e("MainEngine", "EngineServices", "own")
s3.e("SystemManager", "ISystem", "own")
s3.e("ResourceWrapper", "CollisionWorld", "own")
s3.e("GameObjectManager", "BaseObject", "own")
s3.e("GameObjectManager", "ObjectContext", "own")
s3.e("ISystem", "SystemContext", "ref")
s3.e("SystemContext", "World", "ref")
s3.e("SystemContext", "EngineServices", "ref")
s3.e("BaseObject", "ObjectContext", "ref")
s3.e("ObjectContext", "World", "ref")
s3.e("ObjectContext", "EngineServices", "ref")
s3.e("ObjectContext", "GameObjectManager", "ref")
for t in ["MainEngine", "ResourceManager", "AssetDatabase", "InputManager",
          "RayEngine", "AudioManager", "JobSystem", "OptionManager", "DebugDraw"]:
    s3.e("EngineServices", t, "ref")
# シングルトン直引き
s3.e("BaseScene", "SceneManager")
s3.e("BaseScene", "MainEngine")
s3.e("GameManager", "SceneManager")
s3.e("GameManager", "ObjectMetaRegistry")
s3.e("GameObjectManager", "ObjectMetaRegistry")
s3.e("Prefab", "SceneManager")
s3.e("AppFollow", "SceneManager")
s3.e("AppSequence", "SceneManager")
s3.e("AppSequence", "GameManager")
s3.e("AppScore", "GameManager")
s3.e("AppCamera", "OptionManager")
s3.e("AppSound", "AudioManager")
s3.note("BaseScene の直引きは 9 個 → 2 個(ワールドの作成と EngineServices の写し)。\n"
        "ECS / CollisionWorld / ComponentMetaRegistry からエディターへの依存は無くなった。\n"
        "右端はアプリ側に残っている直引き。", -0.9, 9.0, 16)

# =====================================================================
# 4. Resource
# =====================================================================
s4 = Sheet("④ Resource — アセットとリソースの実体化",
           "ResourceManager は MainEngine の持ち物で、AssetDatabase はその中。生成に要るものは ResourceBuildContext で受け取る。")
s4.n("MainEngine", "MainEngine", 0.0, 0, "sing")
s4.n("ResourceManager", "ResourceManager", 2.4, 0, "sing")
s4.n("GraphicsEngine", "GraphicsEngine", 5.2, 0)
s4.n("DescriptorHeapManager", "DescriptorHeapManager", 6.6, 0)
s4.n("MeshBufferAllocator", "MeshBufferAllocator", 8.1, 0)
s4.n("PassMetaRegistry", "PassMetaRegistry", 9.5, 0)
s4.n("AudioManager", "AudioManager", 12.6, 0, "sing")
s4.n("SceneManager", "SceneManager", 13.9, 0, "sing")
s4.n("ScopedResourceBuild", "ScopedResourceBuild\n(モデル1体分をまとめて submit)", 0.0, -1.0)
s4.n("ResourceBuildContext", "ResourceBuildContext\n(Device / Heap / CmdList / RM / AD ...)", 2.6, -1.0, "struct")
s4.n("AssetDatabase", "AssetDatabase", 5.0, -1.0)
s4.n("ResourceRef", "ResourceRef<T>\n(値で埋まる参照カウント)", 6.6, -1.0)
# 実体
s4.n("Model", "Model", 0.0, 2.8)
s4.n("Mesh", "Mesh", 1.1, 2.8)
s4.n("Material", "Material", 2.1, 2.8)
s4.n("Texture", "Texture", 3.1, 2.8)
s4.n("Shader", "Shader", 4.1, 2.8)
s4.n("Animation", "Animation", 5.1, 2.8)
s4.n("AnimatorAsset", "AnimatorAsset", 6.2, 2.8)
s4.n("ActionStateMachineAsset", "ActionStateMachineAsset", 7.7, 2.8)
s4.n("EffectAsset", "EffectAsset", 9.1, 2.8)
s4.n("ParticlesAsset", "ParticlesAsset", 10.3, 2.8)
s4.n("Sound", "Sound", 11.4, 2.8)
s4.n("AudioBehavior", "AudioBehavior", 12.5, 2.8)
s4.n("Prefab", "Prefab", 13.5, 2.8)
s4.n("Font", "Font", 14.4, 2.8)
s4.n("RenderingPipelineAsset", "RenderingPipelineAsset", 15.8, 2.8)
# IO
s4.n("ModelIO", "ModelIO", 0.0, 4.2)
s4.n("ModelConverter", "ModelConverter", 0.0, 5.2)
s4.n("ModelProcessor", "ModelProcessor", 0.0, 6.2)
s4.n("MeshIO", "MeshIO", 1.1, 4.2)
s4.n("MaterialIO", "MaterialIO", 2.1, 4.2)
s4.n("TextureIO", "TextureIO", 3.1, 4.2)
s4.n("TextureImporter", "TextureImporter\n/ TextureCreater", 3.1, 5.2)
s4.n("ShaderIO", "ShaderIO", 4.1, 4.2)
s4.n("DXCCompiler", "DXCCompiler", 4.1, 5.2)
s4.n("AnimationIO", "AnimationIO", 5.1, 4.2)
s4.n("AnimatorAssetIO", "AnimatorAssetIO", 6.2, 4.2)
s4.n("ActionStateMachineAssetIO", "ActionStateMachineAssetIO", 7.7, 4.2)
s4.n("EffectAssetIO", "EffectAssetIO", 9.1, 4.2)
s4.n("ParticlesIO", "ParticlesIO", 10.3, 4.2)
s4.n("SoundIO", "SoundIO", 11.4, 4.2)
s4.n("AudioBehaviorIO", "AudioBehaviorIO", 12.6, 4.2)
s4.n("FontIO", "FontIO", 14.4, 4.2)
s4.n("RenderingPipelineAssetIO", "RenderingPipelineAssetIO", 15.8, 4.2)

s4.e("MainEngine", "ResourceManager", "own")
s4.e("ResourceManager", "AssetDatabase", "own")
s4.e("ScopedResourceBuild", "ResourceBuildContext", "own")
for t in ["ResourceManager", "AssetDatabase", "GraphicsEngine", "DescriptorHeapManager",
          "MeshBufferAllocator", "PassMetaRegistry"]:
    s4.e("ResourceBuildContext", t, "ref")
# シングルトン直引き(残っているもの)
s4.e("ScopedResourceBuild", "MainEngine")
s4.e("Mesh", "MainEngine")
s4.e("Sound", "AudioManager")
s4.e("SoundIO", "AudioManager")
s4.e("Prefab", "SceneManager")
s4.e("ResourceRef", "ResourceManager")
for a, b in [("Model", "ModelIO"), ("ModelIO", "ModelConverter"), ("ModelConverter", "ModelProcessor"),
             ("Mesh", "MeshIO"), ("Material", "MaterialIO"), ("Texture", "TextureIO"),
             ("TextureIO", "TextureImporter"), ("Shader", "ShaderIO"), ("ShaderIO", "DXCCompiler"),
             ("Animation", "AnimationIO"),
             ("AnimatorAsset", "AnimatorAssetIO"), ("ActionStateMachineAsset", "ActionStateMachineAssetIO"),
             ("EffectAsset", "EffectAssetIO"), ("ParticlesAsset", "ParticlesIO"),
             ("Sound", "SoundIO"), ("AudioBehavior", "AudioBehaviorIO"), ("Font", "FontIO"),
             ("RenderingPipelineAsset", "RenderingPipelineAssetIO")]:
    s4.e(a, b, "ref")
for t in ["Model", "Mesh", "Material", "Texture", "Shader", "Animation", "AnimatorAsset",
          "ActionStateMachineAsset", "EffectAsset", "ParticlesAsset", "Sound",
          "AudioBehavior", "Prefab", "Font", "RenderingPipelineAsset"]:
    s4.e("ResourceManager", t, "own")
s4.note("ResourceManager.Instance() 158 回 → 12 回(すべて ResourceRef<T> の中)、AssetDatabase 95 回 → 0 回。\n"
        "IO 群の緑矢印はほぼ消え、残りは SoundIO → AudioManager だけ。\n"
        "ShadingModelTable はシェーディングモデルごと削除した。", -0.9, 7.0, 16)

# =====================================================================
# 5. Editor
# =====================================================================
s5 = Sheet("⑤ Editor — MainEditor とパネル",
           "ログと計測は Engine::Debug のコールバックに一本化。パネルは EditorContext.pServices から引くのが基本になった。")
s5.n("MainEngine", "MainEngine", 0.2, 0, "sing")
s5.n("SceneManager", "SceneManager", 1.6, 0, "sing")
s5.n("OptionManager", "OptionManager", 2.9, 0, "sing")
s5.n("AudioManager", "AudioManager", 4.1, 0, "sing")
s5.n("ObjectMetaRegistry", "ObjectMetaRegistry", 5.4, 0, "sing")
s5.n("App", "Application", 7.0, 0)
s5.n("GameManager", "GameManager", 8.2, 0, "sing")
s5.n("InputActionManager", "InputActionManager", 9.6, 0)
s5.n("MainEditor", "MainEditor", 3.4, 1.2, "sing")
s5.n("DebugCallback", "Engine::Debug\n(ログ・計測のコールバック)", 7.2, 1.3)
s5.n("ImGuiContext", "ImGuiContext", 0.0, 2.4)
s5.n("PanelManager", "PanelManager", 1.6, 2.4)
s5.n("Profiler", "Profiler", 3.2, 2.4)
s5.n("EditorCamera", "EditorCamera", 4.4, 2.4)
s5.n("EffectEditor", "EffectEditor", 5.7, 2.4)
s5.n("EditorContext", "EditorContext\n(pServices / pProfiler / pEditorCamera)", 0.4, 3.6, "struct")
s5.n("IPanel", "IPanel", 2.6, 3.3)
s5.n("EngineServices", "EngineServices", 1.2, 1.2, "struct")
s5.n("HierarchyPanel", "HierarchyPanel", 0.0, 4.9)
s5.n("GameObjectHierarchyPanel", "GameObjectHierarchyPanel", 1.6, 4.9)
s5.n("InspectorPanel", "InspectorPanel", 3.2, 4.9)
s5.n("SceneViewPanel", "SceneViewPanel", 4.5, 4.9)
s5.n("AssetDataBasePanel", "AssetDataBasePanel", 5.9, 4.9)
s5.n("ProfilerPanel", "ProfilerPanel", 7.2, 4.9)
s5.n("OptionPanel", "OptionPanel", 8.3, 4.9)
s5.n("LogPanel", "LogPanel", 9.3, 4.9)
s5.n("RGResourceViewPanel", "RenderGraphResourceViewPanel", 10.8, 4.9)
s5.n("EntityInspector", "EntityInspector", 2.6, 6.0)
s5.n("AssetInspector", "AssetInspector", 4.0, 6.0)
s5.n("ResourceDraw", "ResourceDraw\n(アセット種別ごとの編集)", 4.0, 7.0)
s5.n("AssetLink", "AssetLink", 5.6, 7.0)
s5.n("EditorHelper", "EditorHelper", 7.0, 7.0)
s5.n("AudioBehaviorEdit", "AudioBehaviorEdit", 1.9, 8.1)
s5.n("EffectAssetEdit", "EffectAssetEdit", 3.3, 8.1)
s5.n("TextureEdit", "TextureEdit", 4.5, 8.1)
s5.n("RenderingPipelineEdit", "RenderingPipelineEdit\n/ BuiltinPassEditors", 6.0, 8.1)

for t in ["ImGuiContext", "PanelManager", "Profiler", "EditorCamera", "EffectEditor"]:
    s5.e("MainEditor", t, "own")
s5.e("PanelManager", "IPanel", "own")
s5.e("PanelManager", "EditorContext", "own")
for p in ["HierarchyPanel", "GameObjectHierarchyPanel", "InspectorPanel", "SceneViewPanel",
          "AssetDataBasePanel", "ProfilerPanel", "OptionPanel", "LogPanel", "RGResourceViewPanel"]:
    s5.e(p, "IPanel", "inherit")
s5.e("InspectorPanel", "EntityInspector", "own")
s5.e("InspectorPanel", "AssetInspector", "own")
s5.e("AssetInspector", "ResourceDraw", "ref")
s5.e("AssetInspector", "AssetLink", "ref")
for t in ["AudioBehaviorEdit", "EffectAssetEdit", "TextureEdit", "RenderingPipelineEdit"]:
    s5.e("ResourceDraw", t, "ref")
s5.e("EditorContext", "EngineServices", "ref")
s5.e("EditorContext", "Profiler", "ref")
s5.e("EditorContext", "EditorCamera", "ref")
s5.e("MainEditor", "DebugCallback", "ref")
# エディターの外からエディターへ(逆向き)
s5.e("MainEngine", "MainEditor")
s5.e("SceneManager", "MainEditor")
s5.e("App", "MainEditor")
s5.e("GameManager", "MainEditor")
s5.e("InputActionManager", "MainEditor")
# エディター側の直引き
for a, ts in [
    ("MainEditor", ["MainEngine"]),
    ("EditorCamera", ["OptionManager"]),
    ("EffectEditor", ["AudioManager", "MainEngine", "OptionManager"]),
    ("EditorHelper", ["MainEngine"]),
    ("ImGuiContext", ["MainEngine"]),
    ("PanelManager", ["SceneManager"]),
    ("AssetDataBasePanel", ["MainEngine", "SceneManager"]),
    ("GameObjectHierarchyPanel", ["ObjectMetaRegistry", "SceneManager"]),
    ("HierarchyPanel", ["SceneManager"]),
    ("InspectorPanel", ["SceneManager"]),
    ("EntityInspector", ["SceneManager"]),
    ("ResourceDraw", ["SceneManager"]),
    ("AudioBehaviorEdit", ["AudioManager"]),
    ("EffectAssetEdit", ["MainEditor"]),
    ("TextureEdit", ["OptionManager"]),
    ("RenderingPipelineEdit", ["MainEngine"]),
    ("OptionPanel", ["OptionManager"]),
    ("ProfilerPanel", ["MainEngine"]),
    ("RGResourceViewPanel", ["MainEngine"]),
    ("SceneViewPanel", ["MainEditor", "MainEngine", "OptionManager", "SceneManager"]),
]:
    for t in ts:
        s5.e(a, t)
s5.note("エンジン → エディターの直引きは 38 本 → 11 本(MainEngine の Init/Update/Draw/Release と SceneManager の通知)。\n"
        "AddLog / StartTimer / DrawBox 系は削除済み。SceneViewPanel は今も 17 回の直引きで最多。", -0.9, 9.0, 16)

# =====================================================================
# 6. D3D12 / Raytracing
# =====================================================================
s6 = Sheet("⑥ D3D12 / Raytracing — GPU まわりの下層",
           "D3D12Wrapper は削除。デバイスもヒープも GraphicsEngine が持ち、D3D12 層は引数で受け取るだけで上を見ない。")
s6.n("MainEngine", "MainEngine", 3.0, 0, "sing")
s6.n("GraphicsEngine", "GraphicsEngine", 3.0, 1.1)
s6.n("GraphicsDevice", "GraphicsDevice\n(Device / Factory / Adapter)", 0.2, 1.1)
s6.n("CommandContext", "CommandContext", 0.3, 2.3)
s6.n("FrameManager", "FrameManager", 1.5, 2.3)
s6.n("AsyncGPUManager", "AsyncGPUManager", 2.7, 2.3)
s6.n("BackBuffer", "BackBuffer", 3.8, 2.3)
s6.n("DescriptorHeapManager", "DescriptorHeapManager", 5.2, 2.3)
s6.n("PipelineStateManager", "PipelineStateManager\n(PSO / ルートシグネチャをハンドルで配る)", 7.6, 2.3)
s6.n("CommandPool", "CommandPool\n(Direct / Copy / Compute)", 0.3, 3.4)
s6.n("DescriptorHeap", "DescriptorHeap\n(CBV_SRV_UAV / RTV / DSV / ImGui)", 4.6, 3.4)
s6.n("HeapAllocator", "HeapAllocator<T>", 6.4, 3.4)
s6.n("SamplerAllocator", "SamplerAllocator", 7.7, 3.4)
# D3D12 層
s6.n("GPUResource", "GPUResource", 0.4, 4.8)
s6.n("GPUBuffer", "GPUBuffer", 1.6, 4.8)
s6.n("Texture", "Texture", 2.8, 4.8)
s6.n("RootSignatureBuilder", "RootSignatureBuilder", 4.2, 4.8)
s6.n("PipelineBuilder", "RenderPipelineBuilder", 5.6, 4.8)
s6.n("CBAllocator", "CBAllocator", 6.8, 4.8)
s6.n("StaticBuffer", "StaticBuffer", 0.6, 5.8)
s6.n("MegaBuffer", "MegaBuffer", 1.7, 5.8)
s6.n("OtherBuffer", "Structured / Dynamic /\nVertex / Index ...", 3.0, 5.8)
# レイトレ
s6.n("RayEngine", "RayEngine", 1.5, 7.0, "sing")
s6.n("RenderGraph", "RenderGraph", 4.0, 6.4)
s6.n("ResourceManager", "ResourceManager", 6.4, 7.0, "sing")
s6.n("RayWorld", "RayWorld", 1.5, 8.0)
s6.n("ShaderTable", "ShaderTable", 3.0, 8.0)
s6.n("RayPSO", "RayPSO", 4.1, 8.0)
s6.n("BLAS", "BLAS", 5.2, 8.0)
s6.n("TLAS", "TLAS", 1.5, 9.0)

s6.e("MainEngine", "GraphicsEngine", "own")
for t in ["GraphicsDevice", "CommandContext", "FrameManager", "AsyncGPUManager",
          "BackBuffer", "DescriptorHeapManager", "PipelineStateManager"]:
    s6.e("GraphicsEngine", t, "own")
s6.e("CommandContext", "CommandPool", "own")
for t in ["DescriptorHeap", "HeapAllocator", "SamplerAllocator"]:
    s6.e("DescriptorHeapManager", t, "own")
s6.e("GPUBuffer", "GPUResource", "inherit")
s6.e("StaticBuffer", "GPUBuffer", "inherit")
s6.e("MegaBuffer", "GPUBuffer", "inherit")
s6.e("OtherBuffer", "GPUBuffer", "inherit")
s6.e("GPUResource", "DescriptorHeapManager", "ref")
s6.e("Texture", "DescriptorHeapManager", "ref")
s6.e("DescriptorHeapManager", "GraphicsDevice", "ref")
s6.e("PipelineStateManager", "GraphicsDevice", "ref")
s6.e("PipelineStateManager", "RootSignatureBuilder", "ref")
s6.e("PipelineStateManager", "PipelineBuilder", "ref")
s6.e("MainEngine", "RayEngine")
s6.e("RenderGraph", "RayEngine")
s6.e("BLAS", "MainEngine")
s6.e("RayEngine", "RayWorld", "own")
s6.e("RayWorld", "TLAS", "own")
s6.e("RayEngine", "ResourceManager", "ref")
s6.e("RayWorld", "ResourceManager", "ref")
s6.e("RayWorld", "DescriptorHeapManager", "ref")
s6.e("TLAS", "DescriptorHeapManager", "ref")
s6.note("D3D12Wrapper(68 回)と DescriptorHeapManager::Instance()(62 回)は 0 回に。相互参照も消えた。\n"
        "レイトレ側で残る緑矢印は RayEngine 本体と、BLAS の遅延解放だけ。", -0.9, 9.9, 16)

# =====================================================================
# 7. Input / Audio / Option / Particle / Job
# =====================================================================
s7 = Sheet("⑦ Input / Audio / Option / Particle / JobSystem",
           "アプリ寿命のサービス群。Option が各マネージャーへ設定を押し込む形はまだ残っている。")
s7.n("MainEngine", "MainEngine", 3.0, 0, "sing")
s7.n("EngineServices", "EngineServices", 5.4, 0, "struct")
s7.n("ResourceManager", "ResourceManager", 7.2, 0, "sing")
s7.n("GraphicsEngine", "GraphicsEngine", 8.8, 0)
s7.n("NativeWindow", "NativeWindow", 0.0, 1.2)
s7.n("InputManager", "InputManager", 1.3, 1.2, "sing")
s7.n("AudioManager", "AudioManager", 3.0, 1.2, "sing")
s7.n("OptionManager", "OptionManager", 4.6, 0.7, "sing")
s7.n("ParticleBufferManager", "ParticleBufferManager", 6.4, 1.2)
s7.n("JobSystem", "JobSystem", 8.2, 1.2)
s7.n("InputCollector", "InputCollector", 1.0, 2.4)
s7.n("AudioEngineDX", "DirectX::AudioEngine", 2.6, 2.4)
s7.n("SoundInstancePool", "SoundInstance プール", 3.9, 2.4)
s7.n("IOption", "IOption\n(DrawEdit に EngineServices)", 5.4, 2.4)
s7.n("GPUParticlePool", "GPUParticlePool", 6.9, 2.4)
s7.n("JobContext", "JobContext", 8.1, 2.4)
s7.n("JobWorker", "JobWorker", 9.2, 2.4)
s7.n("InputAxisBase", "InputAxisBase", 0.3, 3.4)
s7.n("InputButtonBase", "InputButtonBase", 1.7, 3.4)
s7.n("WindowOption", "WindowOption", 4.0, 3.5)
s7.n("AudioOption", "AudioOption", 5.1, 3.5)
s7.n("InputOption", "InputOption", 6.2, 3.5)
s7.n("CursorOption", "CursorOption", 7.3, 3.5)
s7.n("OtherOption", "GIOption / RenderingOption / LightingOption /\nBloomOption / ToneMapOption / DebugDrawOption", 9.4, 3.5)
s7.n("InputAxisForWindows", "InputAxisForWindows", -0.2, 4.5)
s7.n("InputAxisForWindowsMouse", "InputAxisForWindowsMouse", 1.2, 4.5)
s7.n("InputAxisForXInput", "InputAxisForXInput", 2.6, 4.5)
s7.n("InputButtonForWindows", "InputButtonForWindows", 0.2, 5.4)
s7.n("InputButtonForWindowsChord", "InputButtonForWindowsChord", 1.7, 5.4)
s7.n("InputButtonForXInput", "InputButtonForXInput", 3.2, 5.4)
s7.n("GameManager", "GameManager", 5.2, 5.7, "sing")
s7.n("InputActionManager", "InputActionManager", -0.6, 6.8)
s7.n("UserData", "UserData", 3.8, 7.6)
s7.n("MainEditor", "MainEditor", 5.2, 6.8, "sing")
s7.n("SceneManager", "SceneManager", 6.5, 6.8, "sing")
s7.n("ObjectMetaRegistry", "ObjectMetaRegistry", 7.9, 6.8, "sing")

for t in ["InputManager", "AudioManager", "OptionManager"]:
    s7.e("MainEngine", t)
for t in ["NativeWindow", "ParticleBufferManager", "JobSystem"]:
    s7.e("MainEngine", t, "own")
s7.e("NativeWindow", "InputManager")
s7.e("InputManager", "MainEngine")
s7.e("InputManager", "OptionManager")
s7.e("InputManager", "InputCollector", "own")
s7.e("InputCollector", "InputAxisBase", "own")
s7.e("InputCollector", "InputButtonBase", "own")
for a, b in [("InputAxisForWindows", "InputAxisBase"), ("InputAxisForWindowsMouse", "InputAxisBase"),
             ("InputAxisForXInput", "InputAxisBase"),
             ("InputButtonForWindows", "InputButtonBase"),
             ("InputButtonForWindowsChord", "InputButtonBase"),
             ("InputButtonForXInput", "InputButtonBase")]:
    s7.e(a, b, "inherit")
s7.e("AudioManager", "AudioEngineDX", "own")
s7.e("AudioManager", "SoundInstancePool", "own")
s7.e("AudioManager", "ResourceManager", "ref")
for t in ["WindowOption", "AudioOption", "InputOption", "CursorOption", "OtherOption"]:
    s7.e("OptionManager", t, "own")
    s7.e(t, "IOption", "inherit")
s7.e("IOption", "EngineServices", "ref")
s7.e("WindowOption", "MainEngine")
s7.e("AudioOption", "AudioManager")
s7.e("InputOption", "InputManager")
s7.e("ParticleBufferManager", "GPUParticlePool", "own")
s7.e("ParticleBufferManager", "GraphicsEngine", "ref")
s7.e("GPUParticlePool", "MainEngine")
s7.e("JobSystem", "JobContext", "own")
s7.e("JobSystem", "JobWorker", "own")
s7.e("GameManager", "InputActionManager", "own")
s7.e("GameManager", "UserData", "own")
s7.e("GameManager", "MainEditor")
s7.e("GameManager", "SceneManager")
s7.e("GameManager", "ObjectMetaRegistry")
s7.e("InputActionManager", "InputManager")
s7.e("InputActionManager", "MainEditor")
s7.note("入力は必ず InputManager 経由。アプリ側の割り当ては UserData に入り、\n"
        "InputActionManager が InputManager へ流し込む。\n"
        "ParticleBufferManager / AudioManager は Init の引数で持ち物を受け取る形になった(MouseCursor は ② に載せた)。", -0.9, 8.5, 16)

# =====================================================================
# 改善点(各シートの右下)
# =====================================================================
s1.imp(
    ("MainEngine が新しい「置き場」になりつつある", [
        "Instance() の総数は 629 → 205 に減ったが、MainEngine だけは 62 → 64 回。",
        "D3D12Wrapper / DescriptorHeapManager を引いていた箇所の一部が",
        "MainEngine::Instance().RefGraphicsEngine() に付け替わっただけのものがある",
        "(BLAS / Mesh / ScopedResourceBuild / ImGuiContext / EditorHelper / SceneViewPanel)。"]),
    ("OptionManager ⇄ MainEngine / InputManager / AudioManager は相互のまま", [
        "D3D12Wrapper ⇄ DescriptorHeapManager の循環は消えた。",
        "残る循環はオプションの押し込み。受け手が起動時と変更通知で読む形にしたい。"]),
    ("描画層 → シーン / ゲームの逆流が残っている", [
        "GraphicsEngine → GameManager::Draw()(テスト呼び出し)と",
        "GraphicsEngine → SceneManager::RefWorld()(BLAS 初期化キュー)。"]),
    ("MainEngine に使われていないメンバーがある", [
        "RenderContext の配列は GraphicsEngine へ移ったが、",
        "MainEngine.h 側の m_upRenderContextVec が宣言だけ残っている。"]),
)

s2.imp(
    ("パスの直引きは 0 になった。残りは MakeContext の中の2本", [
        "RenderGraph::MakeContext が MainEngine と RayEngine を Instance() で引いて詰める。",
        "GraphicsEngine から RayEngine / ParticleBufferManager を受け取れば",
        "描画層の中でシングルトンを引くのは GraphicsEngine の入口だけになる。"]),
    ("GraphicsEngine → SceneManager / GameManager は層の逆流", [
        "GameManager::Draw() は「テスト」とコメントされた呼び出し。",
        "BLAS の初期化要求はワールドのリソースから読んでいるので、",
        "シーン更新側から積むか、Submit の引数で World を渡したい。"]),
    ("ParticleSimulation / GPUParticlePool → MainEngine", [
        "欲しいのは ParticleBufferManager・デルタタイム・遅延解放。",
        "ExecuteParticleSimulation の引数か PassContext で足りる。"]),
    ("OptionManager を描画層の3箇所が直接読む", [
        "GraphicsEngine / MouseCursor / DebugDraw。",
        "解像度などは GraphicsEngineDesc と変更通知で受け取る形にしたい。"]),
)

s3.imp(
    ("アプリ側の直引きが残っている(右端)", [
        "Sequence 群 → SceneManager / GameManager、ScoreSystem → GameManager、",
        "CameraStartSystem → OptionManager。",
        "SystemContext / ObjectContext に経路があるので、そちらへ寄せる。"]),
    ("コンポーネントのヘルパーがヘッダーで Instance() を呼んでいる", [
        "ModelComponent / ParticlesComponent は解消。",
        "SoundComponent / HitSoundComponent → AudioManager、",
        "FollowTargetComponent / AttachmentSlotsComponent → SceneManager が残り。"]),
    ("Prefab → SceneManager", [
        "生成先のワールドを SceneManager::RefWorld() で決めている。",
        "引数で World を受け取れば片付く。"]),
    ("EngineServices に SceneManager が無い", [
        "アプリ側の SceneManager 直引きが消えない理由がここ。",
        "載せるか、シーン遷移だけの細いインターフェースを足すか決めたい。"]),
)

s4.imp(
    ("方針(ResourceBuildContext 経由)はほぼ徹底できた", [
        "IO 群・Model / Texture / Material / Shader の緑矢印は消えた。",
        "ResourceManager::Instance() が残るのは ResourceRef<T> の中だけで、",
        "値として資産に埋まるので引数で渡せない、という理由が付いている。"]),
    ("GraphicsEngine が欲しくて MainEngine を引いている", [
        "ScopedResourceBuild はバッチを開くため、Mesh::Release は",
        "MeshBufferAllocator へ返すため(Release には Context が来ない)。",
        "ScopedResourceBuild は引数で受け取り、Mesh は解放の口を渡しておきたい。"]),
    ("Sound / SoundIO → AudioManager", [
        "サウンドの読み込みにオーディオエンジンが要る。",
        "AudioManager のポインタを Context に載せるのが素直。"]),
)

s5.imp(
    ("エンジン → エディターの逆流は「駆動」と「通知」だけになった", [
        "MainEngine が Init / Update / Draw / Release を呼ぶのは入口として許容。",
        "SceneManager → MainEditor::OnSceneChanged() は通知なので、",
        "コールバック登録の形にすればシーン層からエディターが消える。"]),
    ("アプリ側からエディターを直接呼んでいる", [
        "GameManager / InputActionManager → MainEditor::RegisterEditFunc。",
        "App.cpp も EndProfileFrame / IsModalActive を直接呼ぶ。",
        "エディター初期化の後に登録する、という順序の縛りも暗黙。"]),
    ("SceneViewPanel(17 回)と EffectEditor(9 回)が突出", [
        "EditorContext.pServices は既にある。",
        "SceneManager と GraphicsEngine への経路を足せば、パネルの直引きはほぼ消せる。"]),
    ("エディター自身の直引きは許容範囲", [
        "アプリ寿命のツール層なので、上の2点が直れば十分。"]),
)

s6.imp(
    ("循環と D3D12Wrapper は解消済み", [
        "GraphicsDevice / CommandContext / FrameManager / BackBuffer に分けて",
        "GraphicsEngine へ集約した。D3D12 層は Device* / HeapManager* を引数で受け取る。"]),
    ("RayEngine は描画層の外に置かれたシングルトンのまま", [
        "持ち主が居ないので MainEngine と RenderGraph が Instance() で引く。",
        "GraphicsEngine の持ち物にすると ② と同じ所有ツリーに収まる。"]),
    ("BLAS → MainEngine は GraphicsEngine と遅延解放が欲しいだけ", [
        "生成時の ResourceBuildContext から取れば依存が消える。"]),
)

s7.imp(
    ("OptionManager と各マネージャーが相互に引き合っている", [
        "WindowOption → MainEngine / AudioOption → AudioManager /",
        "InputOption → InputManager。Option 側から押し込むのをやめ、",
        "受け手が起動時と変更通知で読む形にすると片方向になる。"]),
    ("所有されている側が親を引いている箇所が残る", [
        "ParticleBufferManager は解消。GPUParticlePool → MainEngine(遅延解放)と",
        "MouseCursor → MainEngine(ウィンドウ)が残り。"]),
    ("InputManager → MainEngine はモードとウィンドウ", [
        "Init でウィンドウを受け取り、モードは切り替え時に知らせれば片方向になる。"]),
    ("InputActionManager → MainEditor は編集関数の登録", [
        "エディターを知らない形(登録口をエンジン側に置く)にしたい。"]),
)

# =====================================================================
OUT = os.path.dirname(os.path.abspath(__file__))
OUT = os.environ.get("OUTDIR", OUT)
os.makedirs(OUT, exist_ok=True)
for fn, sh, lg in [
    ("01_Overview.excalidraw", s1, (-XP * 0.9, YP * 9.6)),
    ("02_Graphics.excalidraw", s2, (-XP * 0.9, YP * 10.6)),
    ("03_Scene_ECS_GameObject.excalidraw", s3, (-XP * 0.9, YP * 10.0)),
    ("04_Resource.excalidraw", s4, (-XP * 0.9, YP * 7.9)),
    ("05_Editor.excalidraw", s5, (-XP * 0.9, YP * 10.0)),
    ("06_D3D12_Raytracing.excalidraw", s6, (-XP * 0.9, YP * 10.6)),
    ("07_Input_Audio_Option.excalidraw", s7, (-XP * 0.9, YP * 9.2)),
]:
    dump(sh, os.path.join(OUT, fn), lg)


# =====================================================================
# レイアウト確認用の SVG(見た目は excalidraw と同じ配置)
# =====================================================================
def svg(sheet, path):
    xs, ys = [], []
    for n in sheet.nodes.values():
        xs += [n["cx"] - n["w"] / 2, n["cx"] + n["w"] / 2]
        ys += [n["cy"] - n["h"] / 2, n["cy"] + n["h"] / 2]
    for text, x, y, fs in sheet.notes:
        xs.append(x)
        ys.append(y)
    ibox = sheet.improve_box()
    if ibox:
        xs += [ibox[0], ibox[0] + ibox[2]]
        ys += [ibox[1], ibox[1] + ibox[3]]
    minx, maxx = min(xs) - 120, max(xs) + 120
    miny, maxy = min(ys) - 220, max(ys) + 260
    o = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="%d %d %d %d" width="%d">'
         % (minx, miny, maxx - minx, maxy - miny, min(2400, maxx - minx))]
    o.append('<rect x="%d" y="%d" width="%d" height="%d" fill="#121212"/>'
             % (minx, miny, maxx - minx, maxy - miny))
    o.append('<defs><marker id="aw" markerWidth="9" markerHeight="9" refX="8" refY="3" orient="auto">'
             '<path d="M0,0 L8,3 L0,6" fill="none" stroke="#ffffff" stroke-width="1.2"/></marker>'
             '<marker id="ag" markerWidth="9" markerHeight="9" refX="8" refY="3" orient="auto">'
             '<path d="M0,0 L8,3 L0,6" fill="none" stroke="#51cf66" stroke-width="1.2"/></marker></defs>')
    o.append('<text x="%d" y="%d" fill="#fff" font-size="34" font-family="sans-serif">%s</text>'
             % (minx + 40, miny + 60, sheet.title))
    if sheet.subtitle:
        o.append('<text x="%d" y="%d" fill="#aaa" font-size="16" font-family="sans-serif">%s</text>'
                 % (minx + 40, miny + 92, sheet.subtitle))
    for a, b, kind in sheet.edges:
        na, nb = sheet.nodes[a], sheet.nodes[b]
        sx, sy = clip(na["cx"], na["cy"], na["w"], na["h"], nb["cx"], nb["cy"])
        ex, ey = clip(nb["cx"], nb["cy"], nb["w"], nb["h"], na["cx"], na["cy"])
        col = "#51cf66" if kind == "sing" else "#ffffff"
        dash = {"sing": "", "own": "", "ref": ' stroke-dasharray="7 5"', "inherit": ' stroke-dasharray="2 5"'}[kind]
        mk = "ag" if kind == "sing" else "aw"
        o.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="1.4" opacity="0.85"%s marker-end="url(#%s)"/>'
                 % (sx, sy, ex, ey, col, dash, mk))
    for n in sheet.nodes.values():
        col = "#51cf66" if n["kind"] == "sing" else "#ffffff"
        dash = ' stroke-dasharray="8 5"' if n["kind"] == "struct" else ""
        o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="10" fill="#121212" stroke="%s" stroke-width="2"%s/>'
                 % (n["cx"] - n["w"] / 2, n["cy"] - n["h"] / 2, n["w"], n["h"], col, dash))
        lines = n["label"].split("\n")
        for i, ln in enumerate(lines):
            yy = n["cy"] - (len(lines) - 1) * 10 + i * 20 + 5
            o.append('<text x="%.1f" y="%.1f" fill="%s" font-size="15" font-family="sans-serif" text-anchor="middle">%s</text>'
                     % (n["cx"], yy, col, ln.replace("&", "&amp;").replace("<", "&lt;")))
    for text, x, y, fs in sheet.notes:
        for i, ln in enumerate(text.split("\n")):
            o.append('<text x="%.1f" y="%.1f" fill="#bbb" font-size="%d" font-family="sans-serif">%s</text>'
                     % (x, y + i * fs * 1.3, fs, ln))
    if ibox:
        bx, by, bw, bh, lines = ibox
        o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="10" fill="#1a1410" stroke="#ff922b" stroke-width="2"/>'
                 % (bx, by, bw, bh))
        o.append('<text x="%.1f" y="%.1f" fill="#ff922b" font-size="23" font-family="sans-serif">改善点</text>'
                 % (bx + 26, by + 42))
        yy = by + 62 + 15
        for t, head in lines:
            if t:
                o.append('<text x="%.1f" y="%.1f" fill="%s" font-size="15" font-family="sans-serif" xml:space="preserve">%s</text>'
                         % (bx + 26, yy, "#ff922b" if head else "#e6e6e6",
                            t.replace("&", "&amp;").replace("<", "&lt;")))
            yy += FS * LH
    o.append('</svg>')
    open(path, "w", encoding="utf-8").write("\n".join(o))

for fn, sh in [("01_overview", s1), ("02_graphics", s2), ("03_scene_ecs", s3), ("04_resource", s4),
               ("05_editor", s5), ("06_d3d12_raytracing", s6), ("07_input_audio_option", s7)]:
    _pv = os.path.join(OUT, "Preview")
    os.makedirs(_pv, exist_ok=True)
    svg(sh, os.path.join(_pv, fn + ".svg"))
    print("svg", fn)
