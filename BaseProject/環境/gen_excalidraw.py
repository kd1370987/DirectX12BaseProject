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
s1 = Sheet("① 全体俯瞰 — 持ち主の木とシングルトン",
           "2026-10-09 時点。シングルトンは MainEngine と、ResourceRef 用の入口として残した ResourceManager の2つだけ。ほかは持ち主の unique_ptr。")
s1.n("WinMain", "WinMain (main.cpp)\n組み立ての場所", 3.4, 0)
s1.n("App", "Application", 1.0, 1.1)
s1.n("MainEngine", "MainEngine", 3.4, 1.1, "sing")
s1.n("IDevTool", "IDevTool\n(開発ツールの窓口)", 5.4, 1.1)
s1.n("MainEditor", "MainEditor", 7.2, 1.1)
# MainEngine が所有(1段目 : アプリ寿命のマネージャー)
s1.n("ResourceManager", "ResourceManager\n(Instance() は ResourceRef 用の入口)", -0.7, 2.3, "sing")
s1.n("OptionManager", "OptionManager", 0.85, 2.3)
s1.n("InputManager", "InputManager", 1.95, 2.3)
s1.n("AudioManager", "AudioManager", 3.05, 2.3)
s1.n("SceneManager", "SceneManager", 4.15, 2.3)
s1.n("ObjectMetaRegistry", "ObjectMetaRegistry", 5.4, 2.3)
s1.n("ComponentMetaRegistry", "ComponentMetaRegistry", 6.85, 2.3)
# MainEngine が所有(2段目 : 土台)
s1.n("NativeWindow", "NativeWindow", 0.85, 3.5)
s1.n("TimeManager", "TimeManager", 1.95, 3.5)
s1.n("GraphicsEngine", "GraphicsEngine", 3.05, 3.5)
s1.n("JobSystem", "JobSystem", 4.15, 3.5)
s1.n("PhysicsEngine", "PhysicsEngine", 5.2, 3.5)
s1.n("EngineServices", "EngineServices\n(アプリ寿命の置き場・正本)", 6.75, 3.5, "struct")
# 所有の下の段
s1.n("AssetDatabase", "AssetDatabase", -0.7, 3.5)
s1.n("RenderDevice", "RenderDevice", 1.5, 4.7)
s1.n("DescriptorHeapManager", "DescriptorHeapManager", 2.75, 4.7)
s1.n("RayEngine", "RayEngine", 3.95, 4.7)
s1.n("ParticleBufferManager", "ParticleBufferManager", 5.2, 4.7)
s1.n("DebugDraw", "DebugDraw", 6.4, 4.7)
# ゲーム / シーン
s1.n("GameManager", "GameManager", 0.2, 5.9)
s1.n("BaseScene", "BaseScene", 4.15, 5.9)
s1.n("UserData", "UserData", -0.7, 7.0)
s1.n("InputActionManager", "InputActionManager", 0.45, 7.0)
s1.n("MouseCursor", "MouseCursor", 1.6, 7.0)
s1.n("World", "World", 3.5, 7.0)
s1.n("GameObjectManager", "GameObjectManager", 4.85, 7.0)
s1.n("GameDataResource", "GameDataResource\n(シーンをまたぐ記録への入口)", 2.5, 8.1)

# 所有
s1.e("WinMain", "App", "own")
s1.e("WinMain", "MainEditor", "own")
s1.e("App", "GameManager", "own")
for t in ["ResourceManager", "OptionManager", "InputManager", "AudioManager", "SceneManager",
          "ObjectMetaRegistry", "ComponentMetaRegistry", "NativeWindow", "TimeManager",
          "GraphicsEngine", "JobSystem", "PhysicsEngine", "EngineServices"]:
    s1.e("MainEngine", t, "own")
s1.e("ResourceManager", "AssetDatabase", "own")
for t in ["RenderDevice", "DescriptorHeapManager", "RayEngine", "ParticleBufferManager", "DebugDraw"]:
    s1.e("GraphicsEngine", t, "own")
s1.e("SceneManager", "BaseScene", "own")
s1.e("BaseScene", "World", "own")
s1.e("BaseScene", "GameObjectManager", "own")
for t in ["UserData", "InputActionManager", "MouseCursor"]:
    s1.e("GameManager", t, "own")
s1.e("World", "GameDataResource", "own")
# 継承・参照
s1.e("MainEditor", "IDevTool", "inherit")
s1.e("MainEngine", "IDevTool", "ref")
s1.e("SceneManager", "MainEngine", "ref")
s1.e("GameManager", "MainEngine", "ref")
s1.e("GameManager", "SceneManager", "ref")
s1.e("GameDataResource", "GameManager", "ref")
s1.e("InputActionManager", "InputManager", "ref")
s1.e("InputManager", "OptionManager", "ref")
s1.e("AudioManager", "ResourceManager", "ref")
s1.e("GraphicsEngine", "ResourceManager", "ref")
s1.e("GameObjectManager", "ObjectMetaRegistry", "ref")
s1.e("World", "ComponentMetaRegistry", "ref")
# シングルトン経由(残っているもの)
for t in ["WinMain", "App", "MainEditor", "InputManager", "InputActionManager"]:
    s1.e(t, "MainEngine")
s1.note("シングルトンは 13 個(2026-09-10)→ 10 個(09-15)→ 2 個(10-09)。\n"
        "Instance() の呼び出しは 629 回 / 142 ファイル → 205 回 / 62 ファイル → 77 回 / 27 ファイル。\n"
        "GraphicsEngine の持ち物は ② と ⑥、EngineServices が配る先は ③ に載せた。", -0.9, 9.0, 16)

# =====================================================================
# 2. Graphics
# =====================================================================
s2 = Sheet("② Graphics — 描画エンジンとレンダーグラフ",
           "GraphicsEngine がデバイスからパイプライン・パーティクル・レイトレまで全部持つ。パスは PassContext から引く。")
s2.n("MainEngine", "MainEngine", 1.5, 0, "sing")
s2.n("ResourceManager", "ResourceManager", -0.6, 1.1, "sing")
s2.n("GraphicsEngine", "GraphicsEngine", 2.8, 1.1)
# GraphicsEngine が所有(器と実行の道具)
s2.n("RenderDevice", "RenderDevice\n(詳細は ⑥)", -0.7, 2.3)
s2.n("DescriptorHeapManager", "DescriptorHeapManager", 0.55, 2.3)
s2.n("BackBuffer", "BackBuffer", 1.7, 2.3)
s2.n("PipelineStateManager", "PipelineStateManager", 2.85, 2.3)
s2.n("SceneView", "SceneView\n(カメラ・環境の CB)", 4.0, 2.3)
s2.n("DrawSubmitter", "DrawSubmitter\n(描画要求の受け口)", 5.2, 2.3)
s2.n("ParticleBufferManager", "ParticleBufferManager", 6.5, 2.3)
s2.n("RayEngine", "RayEngine\n(詳細は ⑥)", 7.75, 2.3)
s2.n("FrameCompute", "SkinningCompute /\nParticleSimulation", 8.95, 2.3)
# GraphicsEngine が所有(描画データ)
s2.n("RenderContext", "RenderContext\n(フレーム数だけ)", -0.6, 3.4)
s2.n("MeshBufferAllocator", "MeshBufferAllocator", 0.6, 3.4)
s2.n("PassMetaRegistry", "PassMetaRegistry", 1.75, 3.4)
s2.n("LightManager", "LightManager", 2.85, 3.4)
s2.n("QuadPolygon", "QuadPolygon", 3.85, 3.4)
s2.n("DebugDraw", "DebugDraw", 4.85, 3.4)
s2.n("DrawLists", "DrawLists", 5.8, 3.4)
s2.n("GPUParticlePool", "GPUParticlePool /\nEmitterSlotPool", 6.9, 3.4)
s2.n("CBAllocator", "CBAllocator", -0.6, 4.5)
s2.n("CameraPipelineManager", "CameraPipelineManager", 3.0, 4.5)
s2.n("RenderingPipelineAsset", "RenderingPipelineAsset\n(設計図)", 5.6, 4.5)
s2.n("CameraPipelineData", "CameraPipelineData\n(カメラ1台ぶん)", 3.0, 5.5)
s2.n("GraphicsPipeline", "GraphicsPipeline\n(実行インスタンス)", 3.0, 6.5)
s2.n("RenderGraphCompiler", "RenderGraphCompiler", 1.2, 7.5)
s2.n("AliasingReport", "AliasingReport", 1.2, 6.6)
s2.n("RenderGraph", "RenderGraph", 3.0, 7.5)
s2.n("ResourceRegistry", "ResourceRegistry", 4.7, 7.0)
s2.n("VirtualResource", "VirtualResource", 6.3, 7.0)
s2.n("ResourceAllocator", "ResourceAllocator", 5.0, 7.9)
s2.n("GraphHeap", "GraphHeap", 4.2, 8.6)
s2.n("Pass", "Pass (基底)", 3.0, 8.6)
s2.n("PassContext", "PassContext\n(組むのは RenderGraph::MakeContext だけ)", 0.3, 8.7, "struct")
s2.n("ShadingPipelineBuilder", "ShadingPipelineBuilder", 1.6, 9.5)
s2.n("GBufferPass", "GBufferPass", -0.4, 10.6)
s2.n("ZPrePass", "ZPrePass", 0.6, 10.6)
s2.n("DeferredLightingPass", "DeferredLightingPass", 1.75, 10.6)
s2.n("RaytracingGIPass", "RaytracingGIPass", 3.0, 10.6)
s2.n("TAAPass", "TAAPass", 4.0, 10.6)
s2.n("ToneMapPass", "ToneMapPass", 4.9, 10.6)
s2.n("UIPass", "UIPass", 5.8, 10.6)
s2.n("FinalOutputPass", "FinalOutputPass", 6.8, 10.6)
s2.n("GroundFieldPass", "GroundFieldPass", 7.95, 10.6)
s2.n("SceneVolumetricFogPass", "SceneVolumetricFogPass", 9.3, 10.6)

# シングルトン直引き(残っているもの)
s2.e("RenderGraph", "MainEngine")
s2.e("FrameCompute", "MainEngine")
s2.e("ParticleBufferManager", "MainEngine")
s2.e("GPUParticlePool", "MainEngine")
s2.e("GroundFieldPass", "MainEngine")
s2.e("SceneVolumetricFogPass", "MainEngine")
# 所有
s2.e("MainEngine", "GraphicsEngine", "own")
for t in ["RenderDevice", "DescriptorHeapManager", "BackBuffer", "PipelineStateManager",
          "SceneView", "DrawSubmitter", "ParticleBufferManager", "RayEngine", "FrameCompute",
          "RenderContext", "MeshBufferAllocator", "PassMetaRegistry", "LightManager",
          "QuadPolygon", "DebugDraw", "DrawLists", "CameraPipelineManager"]:
    s2.e("GraphicsEngine", t, "own")
s2.e("ParticleBufferManager", "GPUParticlePool", "own")
s2.e("RenderContext", "CBAllocator", "own")
s2.e("CameraPipelineManager", "CameraPipelineData", "own")
s2.e("CameraPipelineData", "GraphicsPipeline", "own")
s2.e("GraphicsPipeline", "RenderGraph", "own")
for t in ["Pass", "ResourceRegistry", "ResourceAllocator", "GraphHeap"]:
    s2.e("RenderGraph", t, "own")
s2.e("ResourceRegistry", "VirtualResource", "own")
s2.e("Pass", "ShadingPipelineBuilder", "own")
s2.e("ResourceManager", "RenderingPipelineAsset", "own")
s2.e("RenderingPipelineAsset", "RenderGraph", "own")
# 参照
s2.e("GraphicsEngine", "ResourceManager", "ref")
s2.e("DrawSubmitter", "DrawLists", "ref")
s2.e("DrawSubmitter", "CameraPipelineManager", "ref")
s2.e("DrawSubmitter", "MeshBufferAllocator", "ref")
s2.e("RenderGraphCompiler", "RenderGraph", "ref")
s2.e("AliasingReport", "RenderGraph", "ref")
s2.e("RenderContext", "DescriptorHeapManager", "ref")
s2.e("RenderContext", "PipelineStateManager", "ref")
s2.e("ParticleBufferManager", "DescriptorHeapManager", "ref")
s2.e("Pass", "PassContext", "ref")
for t in ["RenderGraph", "GraphicsEngine", "RenderContext", "ResourceManager",
          "DescriptorHeapManager", "ParticleBufferManager", "RayEngine", "MainEngine"]:
    s2.e("PassContext", t, "ref")
for p in ["GBufferPass", "ZPrePass", "DeferredLightingPass", "RaytracingGIPass", "TAAPass",
          "ToneMapPass", "UIPass", "FinalOutputPass", "GroundFieldPass", "SceneVolumetricFogPass"]:
    s2.e(p, "Pass", "inherit")
s2.note("パスは 37 種類。ここに並べたのは代表と、MainEngine を直に引いている2つ(デルタタイム)。\n"
        "描画層からシーン・ゲーム・オプションへの直引きは 0 になった(オプションは MainEngine が流し込む)。", -0.9, 11.6, 16)

# =====================================================================
# 3. Scene / ECS / GameObject
# =====================================================================
s3 = Sheet("③ Scene / ECS / GameObject — シーンの中身",
           "EngineServices は MainEngine が正本を持ち、World が写しを持つ。System / GameObject はそこから引き、シングルトンは見ない。")
s3.n("SceneManager", "SceneManager", 3.0, 0)
s3.n("GameManager", "GameManager", 8.6, 0)
s3.n("BaseScene", "BaseScene", 3.0, 1.1)
s3.n("World", "World", 1.6, 2.2)
s3.n("APPWorld", "APPWorld\n(ゲーム側の決めごと)", 0.0, 1.2)
s3.n("GameObjectManager", "GameObjectManager", 5.2, 2.2)
s3.n("EntityStorage", "EntityStorage", -0.2, 3.3)
s3.n("SystemManager", "SystemManager", 1.0, 3.3)
s3.n("CommandBuffer", "CommandBuffer", 2.25, 3.3)
s3.n("ResourceStore", "ResourceStore\n(ワールド寿命のリソース)", 3.5, 3.3)
s3.n("BaseObject", "BaseObject", 5.6, 3.3)
s3.n("ISystem", "ISystem", 0.2, 4.4)
s3.n("PhysicsWorld", "PhysicsWorld", 2.9, 4.4)
s3.n("GameDataResource", "GameDataResource", 4.6, 4.4)
s3.n("GlobalGameContext", "GlobalGameContext\n(シーンをまたぐ記録)", 8.6, 1.2)
s3.n("SystemContext", "SystemContext\n(pWorld / pServices / dt)", 1.0, 5.5, "struct")
s3.n("ObjectContext", "ObjectContext\n(pWorld / pServices / pObjectManager)", 5.4, 5.5, "struct")
s3.n("CompEditContext", "CompEditContext\n(pData / pWorld)", 8.9, 4.4, "struct")
s3.n("EngineServices", "EngineServices\n(アプリ寿命の置き場)", 3.4, 6.5, "struct")
# EngineServices が配る先
s3.n("ResourceManager", "ResourceManager", -0.4, 7.7, "sing")
s3.n("AssetDatabase", "AssetDatabase", 0.75, 7.7)
s3.n("InputManager", "InputManager", 1.8, 7.7)
s3.n("AudioManager", "AudioManager", 2.85, 7.7)
s3.n("OptionManager", "OptionManager", 3.95, 7.7)
s3.n("RayEngine", "RayEngine", 4.95, 7.7)
s3.n("JobSystem", "JobSystem", 5.85, 7.7)
s3.n("PhysicsEngine", "PhysicsEngine", 6.85, 7.7)
s3.n("DebugDraw", "DebugDraw", 7.85, 7.7)
s3.n("MainEngine", "MainEngine", 9.0, 7.7, "sing")
s3.n("SceneManagerSvc", "SceneManager", 3.4, 8.7)
s3.n("ObjectMetaRegistry", "ObjectMetaRegistry", 1.4, 8.7)
s3.n("ComponentMetaRegistry", "ComponentMetaRegistry", -0.2, 2.2)
# アプリ側の使い手(今はコンテキスト経由)
s3.n("AppSequence", "Sequence 群\n(Title / Pause / Result / Scene / MissionSelect / Loading)", 10.4, 3.3)
s3.n("AppScore", "ScoreSystem / ScoreHUD", 10.4, 2.2)
s3.n("AppCamera", "CameraStartSystem", 10.4, 6.5)
s3.n("AppFollow", "FollowTargetComponent /\nAttachmentSlotsComponent", 10.4, 4.4)

s3.e("MainEngine", "EngineServices", "own")
s3.e("SceneManager", "BaseScene", "own")
s3.e("BaseScene", "World", "own")
s3.e("BaseScene", "GameObjectManager", "own")
s3.e("APPWorld", "World", "inherit")
for t in ["EntityStorage", "SystemManager", "CommandBuffer", "ResourceStore"]:
    s3.e("World", t, "own")
s3.e("World", "EngineServices", "own")
s3.e("World", "ComponentMetaRegistry", "ref")
s3.e("SystemManager", "ISystem", "own")
s3.e("ResourceStore", "PhysicsWorld", "own")
s3.e("ResourceStore", "GameDataResource", "own")
s3.e("GameDataResource", "GlobalGameContext", "ref")
s3.e("GameManager", "GlobalGameContext", "own")
s3.e("GameManager", "SceneManager", "ref")
s3.e("GameObjectManager", "BaseObject", "own")
s3.e("GameObjectManager", "ObjectContext", "own")
s3.e("ISystem", "SystemContext", "ref")
s3.e("SystemContext", "World", "ref")
s3.e("SystemContext", "EngineServices", "ref")
s3.e("BaseObject", "ObjectContext", "ref")
s3.e("ObjectContext", "World", "ref")
s3.e("ObjectContext", "EngineServices", "ref")
s3.e("ObjectContext", "GameObjectManager", "ref")
for t in ["MainEngine", "ResourceManager", "AssetDatabase", "InputManager", "AudioManager",
          "OptionManager", "RayEngine", "JobSystem", "PhysicsEngine", "DebugDraw",
          "ObjectMetaRegistry", "SceneManagerSvc"]:
    s3.e("EngineServices", t, "ref")
s3.e("AppSequence", "ObjectContext", "ref")
s3.e("AppScore", "GameDataResource", "ref")
s3.e("AppCamera", "SystemContext", "ref")
s3.e("AppFollow", "CompEditContext", "ref")
s3.note("シーン・ECS・GameObject・システムの Instance() は 0。アプリ側に残るのは入口の App.cpp と InputActionManager(RefDevTool)だけ。\n"
        "SceneManager / ObjectMetaRegistry を EngineServices に載せ、ゲームの記録はワールドの GameDataResource から引く形にした。\n"
        "右端はかつて直引きしていた使い手。今はすべてコンテキスト経由。", -0.9, 9.8, 16)

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
s4.n("AudioEngineDX", "DirectX::AudioEngine\n(持ち主は AudioManager)", 11.4, 0)
s4.n("SceneManager", "SceneManager", 13.9, 0)
s4.n("ScopedResourceBuild", "ScopedResourceBuild\n(モデル1体分をまとめて submit)", 0.0, -1.0)
s4.n("ResourceBuildContext", "ResourceBuildContext\n(Device / Heap / CmdList / RM / AD / AudioEngine / SceneManager ...)", 4.2, -1.0, "struct")
s4.n("AssetDatabase", "AssetDatabase", 7.9, -1.0)
s4.n("ResourceRef", "ResourceRef<T>\n(値で埋まる参照カウント)", 9.6, -1.0)
# 実体
s4.n("Model", "Model", 0.0, 2.8)
s4.n("Mesh", "Mesh", 1.1, 2.8)
s4.n("Material", "Material", 2.1, 2.8)
s4.n("Texture", "Texture", 3.1, 2.8)
s4.n("Shader", "Shader", 4.1, 2.8)
s4.n("Animation", "Animation", 5.1, 2.8)
s4.n("AnimatorAsset", "AnimatorAsset", 6.2, 2.8)
s4.n("EffectAsset", "EffectAsset", 7.4, 2.8)
s4.n("ParticlesAsset", "ParticlesAsset", 8.6, 2.8)
s4.n("Sound", "Sound", 9.7, 2.8)
s4.n("AudioBehavior", "AudioBehavior", 10.8, 2.8)
s4.n("Prefab", "Prefab", 11.9, 2.8)
s4.n("EffectPrefab", "EffectPrefab", 12.9, 2.8)
s4.n("Font", "Font", 13.9, 2.8)
s4.n("RenderingPipelineAsset", "RenderingPipelineAsset", 15.3, 2.8)
# IO
s4.n("ModelIO", "ModelIO", 0.0, 4.2)
s4.n("ModelConverter", "ModelConverter", 0.0, 5.2)
s4.n("ModelProcessor", "ModelProcessor", 0.0, 6.2)
s4.n("MeshIO", "MeshIO", 1.1, 4.2)
s4.n("MaterialIO", "MaterialIO", 2.1, 4.2)
s4.n("TextureIO", "TextureIO", 3.1, 4.2)
s4.n("ShaderIO", "ShaderIO", 4.1, 4.2)
s4.n("DXCCompiler", "DXCCompiler", 4.1, 5.2)
s4.n("AnimationIO", "AnimationIO", 5.1, 4.2)
s4.n("AnimatorAssetIO", "AnimatorAssetIO", 6.2, 4.2)
s4.n("EffectAssetIO", "EffectAssetIO", 7.4, 4.2)
s4.n("ParticlesIO", "ParticlesIO", 8.6, 4.2)
s4.n("SoundIO", "SoundIO", 9.7, 4.2)
s4.n("AudioBehaviorIO", "AudioBehaviorIO", 10.9, 4.2)
s4.n("FontIO", "FontIO", 13.9, 4.2)
s4.n("RenderingPipelineAssetIO", "RenderingPipelineAssetIO", 15.3, 4.2)

s4.e("MainEngine", "ResourceManager", "own")
s4.e("ResourceManager", "AssetDatabase", "own")
s4.e("ScopedResourceBuild", "ResourceBuildContext", "own")
for t in ["ResourceManager", "AssetDatabase", "GraphicsEngine", "DescriptorHeapManager",
          "MeshBufferAllocator", "PassMetaRegistry"]:
    s4.e("ResourceBuildContext", t, "ref")
# シングルトン直引き(残っているもの)
s4.e("ScopedResourceBuild", "MainEngine")
s4.e("Mesh", "MainEngine")
s4.e("ResourceRef", "ResourceManager")
# 読み込みでコンテキストから受け取るもの(ResourceManager が起動時に預かった行き先を載せる)
s4.e("SoundIO", "AudioEngineDX", "ref")
s4.e("Prefab", "SceneManager", "ref")
s4.e("EffectPrefab", "SceneManager", "ref")
for a, b in [("Model", "ModelIO"), ("ModelIO", "ModelConverter"), ("ModelConverter", "ModelProcessor"),
             ("Mesh", "MeshIO"), ("Material", "MaterialIO"), ("Texture", "TextureIO"),
             ("Shader", "ShaderIO"), ("ShaderIO", "DXCCompiler"),
             ("Animation", "AnimationIO"), ("AnimatorAsset", "AnimatorAssetIO"),
             ("EffectAsset", "EffectAssetIO"), ("ParticlesAsset", "ParticlesIO"),
             ("Sound", "SoundIO"), ("AudioBehavior", "AudioBehaviorIO"), ("Font", "FontIO"),
             ("RenderingPipelineAsset", "RenderingPipelineAssetIO")]:
    s4.e(a, b, "ref")
for t in ["Model", "Mesh", "Material", "Texture", "Shader", "Animation", "AnimatorAsset",
          "EffectAsset", "ParticlesAsset", "Sound", "AudioBehavior", "Prefab", "EffectPrefab",
          "Font", "RenderingPipelineAsset"]:
    s4.e("ResourceManager", t, "own")
s4.note("ResourceManager.Instance() は 12 回(すべて ResourceRef<T> の中)。\n"
        "SoundIO → AudioManager と Prefab / EffectPrefab → SceneManager の直引きは消え、\n"
        "ResourceManager が起動時に受け取った行き先(オーディオエンジン・SceneManager)をコンテキストに載せて渡す。", -0.9, 7.0, 16)

# =====================================================================
# 5. Editor
# =====================================================================
s5 = Sheet("⑤ Editor — MainEditor とパネル",
           "MainEditor は WinMain が持ち、エンジンへは IDevTool として差し込む。パネルは EditorContext から引く。")
s5.n("WinMain", "WinMain (main.cpp)", 3.4, -1.1)
s5.n("MainEngine", "MainEngine", 0.2, 0, "sing")
s5.n("IDevTool", "IDevTool\n(開発ツールの窓口)", 1.9, 0)
s5.n("SceneManager", "SceneManager", 5.1, 0)
s5.n("App", "Application", 6.6, 0)
s5.n("GameManager", "GameManager", 7.9, 0)
s5.n("InputActionManager", "InputActionManager", 9.3, 0)
s5.n("MainEditor", "MainEditor", 3.4, 1.2)
s5.n("DebugCallback", "Engine::Debug\n(ログ・計測のコールバック)", 6.6, 1.3)
s5.n("ImGuiContext", "ImGuiContext", 0.0, 2.4)
s5.n("PanelManager", "PanelManager", 1.6, 2.4)
s5.n("Profiler", "Profiler", 3.2, 2.4)
s5.n("EditorCamera", "EditorCamera", 4.4, 2.4)
s5.n("EffectEditor", "EffectEditor", 5.7, 2.4)
s5.n("EditorContext", "EditorContext\n(pServices / pProfiler / pEditorCamera / pEffectEditor)", 0.4, 3.6, "struct")
s5.n("IPanel", "IPanel", 2.9, 3.4)
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

s5.e("WinMain", "MainEditor", "own")
s5.e("MainEditor", "IDevTool", "inherit")
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
for t in ["EngineServices", "Profiler", "EditorCamera", "EffectEditor"]:
    s5.e("EditorContext", t, "ref")
s5.e("MainEditor", "DebugCallback", "ref")
# エディターの外からは IDevTool 越し(エディターを名指ししない)
for t in ["MainEngine", "SceneManager", "App", "GameManager", "InputActionManager"]:
    s5.e(t, "IDevTool", "ref")
# エディター側の直引き(残っているもの。すべて MainEngine)
for a in ["MainEditor", "EffectEditor", "EditorHelper", "ImGuiContext", "AssetDataBasePanel",
          "RenderingPipelineEdit", "ProfilerPanel", "RGResourceViewPanel", "SceneViewPanel"]:
    s5.e(a, "MainEngine")
s5.note("MainEditor::Instance() は 0。パネルからの SceneManager / OptionManager / AudioManager の直引きも 0 になり、\n"
        "EditorContext.pServices と pEffectEditor / pEditorCamera から引く。\n"
        "残る直引きは MainEngine だけ(SceneViewPanel / ProfilerPanel / EffectEditor が各 5 回)。", -0.9, 9.0, 16)

# =====================================================================
# 6. D3D12 / Raytracing
# =====================================================================
s6 = Sheet("⑥ D3D12 / Raytracing — GPU まわりの下層",
           "デバイスもヒープもレイトレも GraphicsEngine の持ち物。D3D12 層は引数で受け取るだけで上を見ない。")
s6.n("MainEngine", "MainEngine", 3.0, 0, "sing")
s6.n("GraphicsEngine", "GraphicsEngine", 3.0, 1.1)
s6.n("RenderDevice", "RenderDevice", 0.9, 1.1)
s6.n("GraphicsDevice", "GraphicsDevice\n(Device / Factory / Adapter)", -0.7, 1.1)
s6.n("CommandContext", "CommandContext", 0.6, 2.3)
s6.n("FrameManager", "FrameManager", 1.7, 2.3)
s6.n("AsyncGPUManager", "AsyncGPUManager", 2.8, 2.3)
s6.n("BackBuffer", "BackBuffer", 3.9, 2.3)
s6.n("DescriptorHeapManager", "DescriptorHeapManager", 5.2, 2.3)
s6.n("PipelineStateManager", "PipelineStateManager\n(PSO / ルートシグネチャをハンドルで配る)", 7.6, 2.3)
s6.n("CommandPool", "CommandPool\n(Direct / Copy / Compute)", 0.6, 3.4)
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
s6.n("RayEngine", "RayEngine", 1.5, 7.0)
s6.n("RenderGraph", "RenderGraph", 4.0, 6.6)
s6.n("PassContext", "PassContext", 4.0, 7.6, "struct")
s6.n("ResourceManager", "ResourceManager", 6.4, 7.0, "sing")
s6.n("RayWorld", "RayWorld", 1.5, 8.0)
s6.n("ShaderTable", "ShaderTable", 3.0, 8.6)
s6.n("RayPSO", "RayPSO", 4.1, 8.6)
s6.n("BLAS", "BLAS", 5.2, 8.6)
s6.n("TLAS", "TLAS", 1.5, 9.0)

s6.e("MainEngine", "GraphicsEngine", "own")
for t in ["RenderDevice", "BackBuffer", "DescriptorHeapManager", "PipelineStateManager"]:
    s6.e("GraphicsEngine", t, "own")
for t in ["GraphicsDevice", "CommandContext", "FrameManager", "AsyncGPUManager"]:
    s6.e("RenderDevice", t, "own")
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
s6.e("RenderGraph", "PassContext", "own")
s6.e("PassContext", "RayEngine", "ref")
s6.e("BLAS", "MainEngine")
s6.e("RenderGraph", "MainEngine")
s6.e("RayEngine", "RayWorld", "own")
s6.e("RayWorld", "TLAS", "own")
s6.e("RayEngine", "ResourceManager", "ref")
s6.e("RayWorld", "ResourceManager", "ref")
s6.e("RayWorld", "DescriptorHeapManager", "ref")
s6.e("TLAS", "DescriptorHeapManager", "ref")
s6.note("RayEngine はシングルトンを外れ、GraphicsEngine の持ち物になった(所有の矢印は ② に載せた)。パスへは PassContext.pRayEngine で配る。\n"
        "このシートで残る緑矢印は BLAS の遅延解放と、RenderGraph::MakeContext の入口だけ。", -0.9, 9.9, 16)

# =====================================================================
# 7. Input / Audio / Option / Particle / Job
# =====================================================================
s7 = Sheet("⑦ Input / Audio / Option / Particle / JobSystem",
           "アプリ寿命のサービス群。どれも MainEngine の持ち物になり、オプションは MainEngine とエディターが受け手へ流し込む。")
s7.n("MainEngine", "MainEngine", 3.0, 0, "sing")
s7.n("EngineServices", "EngineServices", 5.4, 0, "struct")
s7.n("ResourceManager", "ResourceManager", 7.2, 0, "sing")
s7.n("GraphicsEngine", "GraphicsEngine", 8.8, 0)
s7.n("NativeWindow", "NativeWindow\n(フォーカスの通知先を持つ)", -0.2, 1.2)
s7.n("InputManager", "InputManager", 1.4, 1.2)
s7.n("AudioManager", "AudioManager", 3.0, 1.2)
s7.n("OptionManager", "OptionManager", 4.6, 1.2)
s7.n("ParticleBufferManager", "ParticleBufferManager", 7.6, 1.2)
s7.n("JobSystem", "JobSystem", 9.4, 1.2)
s7.n("DebugDraw", "DebugDraw", 6.2, 1.2)
s7.n("InputCollector", "InputCollector", 1.0, 2.4)
s7.n("AudioEngineDX", "DirectX::AudioEngine", 2.6, 2.4)
s7.n("SoundInstancePool", "SoundInstance プール\n(発行元の AudioManager を持つ)", 4.0, 2.4)
s7.n("IOption", "IOption\n(DrawEdit に EngineServices)", 5.6, 2.4)
s7.n("GPUParticlePool", "GPUParticlePool /\nEmitterSlotPool", 7.6, 2.4)
s7.n("JobContext", "JobContext", 8.9, 2.4)
s7.n("JobWorker", "JobWorker", 9.9, 2.4)
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
s7.n("GameManager", "GameManager", 5.2, 5.7)
s7.n("InputActionManager", "InputActionManager", -0.6, 6.8)
s7.n("UserData", "UserData", 3.8, 7.6)
s7.n("MouseCursor", "MouseCursor", 6.6, 6.8)

for t in ["NativeWindow", "InputManager", "AudioManager", "OptionManager", "JobSystem", "GraphicsEngine",
          "EngineServices"]:
    s7.e("MainEngine", t, "own")
s7.e("GraphicsEngine", "ParticleBufferManager", "own")
s7.e("GraphicsEngine", "DebugDraw", "own")
s7.e("InputManager", "MainEngine")
s7.e("InputManager", "InputOption", "ref")
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
s7.e("SoundInstancePool", "AudioManager", "ref")
for t in ["WindowOption", "AudioOption", "InputOption", "CursorOption", "OtherOption"]:
    s7.e("OptionManager", t, "own")
    s7.e(t, "IOption", "inherit")
s7.e("IOption", "EngineServices", "ref")
s7.e("AudioOption", "AudioManager", "ref")
s7.e("MainEngine", "DebugDraw", "ref")
s7.e("ParticleBufferManager", "GPUParticlePool", "own")
s7.e("ParticleBufferManager", "MainEngine")
s7.e("GPUParticlePool", "MainEngine")
s7.e("JobSystem", "JobContext", "own")
s7.e("JobSystem", "JobWorker", "own")
s7.e("GameManager", "InputActionManager", "own")
s7.e("GameManager", "UserData", "own")
s7.e("GameManager", "MouseCursor", "own")
s7.e("InputActionManager", "InputManager", "ref")
s7.e("InputActionManager", "UserData", "ref")
s7.e("InputActionManager", "MainEngine")
s7.e("MouseCursor", "EngineServices", "ref")
s7.note("Option から各マネージャーを Instance() で押し込む形は消えた。WindowOption / AudioOption / InputOption は DrawEdit の EngineServices から、\n"
        "起動時の値は MainEngine が Init の引数(InputOption)・Apply(AudioOption)・毎フレームの流し込み(DebugDrawOption)で渡す。\n"
        "NativeWindow は入力を知らず、MainEngine がつないだフォーカスの通知先を呼ぶだけ。", -0.9, 8.5, 16)

# =====================================================================
# 改善点(各シートの右下)
# =====================================================================
s1.imp(
    ("MainEngine::Instance() が 65 回残っている", [
        "多いのは App.cpp(13)・InputManager(6)・エディター(29)。",
        "どれも EngineServices / EditorContext / PassContext に経路があるので、",
        "そちらへ寄せればさらに減らせる。"]),
    ("シーンの解放順はアプリ側が持っている", [
        "SceneManager の持ち主は MainEngine だが、Release はゲームより先に Application が呼ぶ。",
        "シーンのオブジェクトがゲームの記録(GameDataResource)を指しているため。"]),
    ("ResourceManager::Instance() は残す判断", [
        "ResourceRef<T> が値として資産に埋まり、コピー・破棄のたびに参照カウントを触る。",
        "引数で渡す口が無いので、ここだけは入口として残している。"]),
)

s2.imp(
    ("描画層の残りは MainEngine の4種類", [
        "RenderGraph::MakeContext(PassContext を組む入口)、",
        "ParticleBufferManager / GPUParticlePool / ParticleSimulation(遅延解放・デルタタイム)、",
        "GroundFieldPass / SceneVolumetricFogPass(デルタタイム)。"]),
    ("パスがデルタタイムを MainEngine から引いている", [
        "PassContext にデルタタイムを載せれば、パスの Instance() は再び 0 にできる。"]),
)

s3.imp(
    ("コンテキストへの寄せは完了", [
        "Sequence 群は ObjectContext、ScoreSystem / CameraStartSystem は SystemContext、",
        "コンポーネントの編集 UI は CompEditContext から引く。"]),
    ("World のリソースにアプリ寿命の記録の入口がある", [
        "GameDataResource はワールド寿命の箱に、GameManager が持つ記録のポインタを入れている。",
        "ワールドを作る作り手(GameManager)が詰めるので、作り手を通らないワールドには無い。"]),
)

s4.imp(
    ("方針(ResourceBuildContext 経由)は徹底できた", [
        "IO 群・Sound・Prefab の緑矢印は消えた。",
        "ResourceManager::Instance() が残るのは ResourceRef<T> の中だけ。"]),
    ("GraphicsEngine が欲しくて MainEngine を引いている", [
        "ScopedResourceBuild はバッチを開くため、Mesh::Release は",
        "MeshBufferAllocator へ返すため(Release には Context が来ない)。"]),
)

s5.imp(
    ("エンジン・アプリ → エディターは IDevTool 越しだけ", [
        "MainEngine / SceneManager / Application / GameManager / InputActionManager は",
        "IDevTool を見るだけで、MainEditor を名指ししない。"]),
    ("パネルの MainEngine 直引きが残る", [
        "SceneViewPanel / ProfilerPanel / EffectEditor が各 5 回、ImGuiContext が 4 回。",
        "欲しいのは GraphicsEngine とモードなので、EditorContext に載せれば消せる。"]),
)

s6.imp(
    ("RayEngine は所有ツリーに収まった", [
        "持ち主は GraphicsEngine。RenderGraph と MainEngine の Instance() 経由の依存は消えた。"]),
    ("BLAS → MainEngine は GraphicsEngine と遅延解放が欲しいだけ", [
        "生成時の ResourceBuildContext から取れば依存が消える。"]),
)

s7.imp(
    ("InputManager → MainEngine はモードとウィンドウ", [
        "Init でウィンドウを受け取り、モードは切り替え時に知らせれば片方向になる。"]),
    ("パーティクルの遅延解放が MainEngine を引いている", [
        "ParticleBufferManager / GPUParticlePool / EmitterSlotPool。",
        "解放の口を Init で受け取れば依存が消える。"]),
    ("InputActionManager → MainEngine は編集関数の登録", [
        "RefDevTool() を引くためだけなので、Init で IDevTool を受け取れば消える。"]),
)

# =====================================================================
# 出力先 : 環境/DependenceView(.excalidraw と確認用の .svg を同じ場所へ)
# =====================================================================
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "DependenceView")
OUT = os.environ.get("OUTDIR", OUT)
os.makedirs(OUT, exist_ok=True)
for fn, sh, lg in [
    ("01_Overview.excalidraw", s1, (-XP * 0.9, YP * 10.4)),
    ("02_Graphics.excalidraw", s2, (-XP * 0.9, YP * 12.5)),
    ("03_Scene_ECS_GameObject.excalidraw", s3, (-XP * 0.9, YP * 11.0)),
    ("04_Resource.excalidraw", s4, (-XP * 0.9, YP * 7.9)),
    ("05_Editor.excalidraw", s5, (-XP * 0.9, YP * 10.0)),
    ("06_D3D12_Raytracing.excalidraw", s6, (-XP * 0.9, YP * 10.6)),
    ("07_Input_Audio_Option.excalidraw", s7, (-XP * 0.9, YP * 9.6)),
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
    svg(sh, os.path.join(OUT, fn + ".svg"))
    print("svg", fn)
