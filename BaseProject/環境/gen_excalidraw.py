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
           "App が Instance() を叩いて回し、MainEngine が実体を unique_ptr で抱える。")
s1.n("App", "Application", 3, 0)
s1.n("MainEngine", "MainEngine", 3, 1, "sing")
# MainEngine が所有
s1.n("NativeWindow", "NativeWindow", 0.0, 2)
s1.n("TimeManager", "TimeManager", 1.1, 2)
s1.n("GraphicsEngine", "GraphicsEngine", 2.3, 2)
s1.n("PipelineStateManager", "PipelineStateManager", 3.6, 2)
s1.n("ParticleBufferManager", "ParticleBufferManager", 5.0, 2)
s1.n("JobSystem", "JobSystem", 6.3, 2)
s1.n("MouseCursor", "MouseCursor", 7.3, 2)
# 下層シングルトン
s1.n("InputManager", "InputManager", 0.0, 3.2, "sing")
s1.n("OptionManager", "OptionManager", 1.2, 3.2, "sing")
s1.n("D3D12Wrapper", "D3D12Wrapper", 2.5, 3.2, "sing")
s1.n("DescriptorHeapManager", "DescriptorHeapManager", 3.9, 3.2, "sing")
s1.n("RayEngine", "RayEngine", 5.3, 3.2, "sing")
s1.n("AudioManager", "AudioManager", 6.5, 3.2, "sing")
s1.n("MainEditor", "MainEditor", 7.6, 3.2, "sing")
# リソース系
s1.n("ResourceManager", "ResourceManager", 2.5, 4.3, "sing")
s1.n("AssetDatabase", "AssetDatabase", 4.0, 4.3, "sing")
s1.n("ObjectMetaRegistry", "ObjectMetaRegistry", 6.5, 4.3, "sing")
# ゲーム/シーン
s1.n("GameManager", "GameManager", 0.6, 5.4, "sing")
s1.n("SceneManager", "SceneManager", 3.2, 5.4, "sing")
s1.n("UserData", "UserData", 0.0, 6.4)
s1.n("InputActionManager", "InputActionManager", 1.2, 6.4)
s1.n("BaseScene", "BaseScene", 3.2, 6.4)
s1.n("World", "World", 2.4, 7.4)
s1.n("GameObjectManager", "GameObjectManager", 4.2, 7.4)

for t in ["MainEngine", "GameManager", "SceneManager", "MainEditor", "InputManager"]:
    s1.e("App", t)
for t in ["AssetDatabase", "AudioManager", "D3D12Wrapper", "DescriptorHeapManager",
          "InputManager", "MainEditor", "OptionManager", "RayEngine", "ResourceManager"]:
    s1.e("MainEngine", t)
for t in ["MainEngine", "OptionManager"]:
    s1.e("MainEditor", t)
for t in ["AssetDatabase", "AudioManager", "D3D12Wrapper", "MainEditor", "ResourceManager"]:
    s1.e("SceneManager", t)
for t in ["MainEditor", "SceneManager", "ObjectMetaRegistry"]:
    s1.e("GameManager", t)
for t in ["MainEngine", "OptionManager"]:
    s1.e("InputManager", t)
for t in ["AssetDatabase", "ResourceManager"]:
    s1.e("AudioManager", t)
s1.e("ResourceManager", "AssetDatabase")
s1.e("AssetDatabase", "MainEditor")
s1.e("D3D12Wrapper", "DescriptorHeapManager")
s1.e("DescriptorHeapManager", "D3D12Wrapper")
s1.e("NativeWindow", "InputManager")
for t in ["DescriptorHeapManager", "InputManager", "MainEngine", "OptionManager", "ResourceManager"]:
    s1.e("MouseCursor", t)
for t in ["AssetDatabase", "D3D12Wrapper"]:
    s1.e("ParticleBufferManager", t)
for t in ["D3D12Wrapper", "DescriptorHeapManager", "GameManager", "MainEditor",
          "MainEngine", "OptionManager", "ResourceManager", "SceneManager"]:
    s1.e("GraphicsEngine", t)
for t in ["AssetDatabase", "AudioManager", "InputManager", "MainEditor", "MainEngine",
          "OptionManager", "RayEngine", "ResourceManager", "SceneManager"]:
    s1.e("BaseScene", t)
for t in ["MainEngine", "AudioManager", "InputManager"]:
    s1.e("OptionManager", t)
s1.e("GameObjectManager", "ObjectMetaRegistry")
for t in ["NativeWindow", "TimeManager", "GraphicsEngine", "PipelineStateManager",
          "ParticleBufferManager", "JobSystem", "MouseCursor"]:
    s1.e("MainEngine", t, "own")
s1.e("SceneManager", "BaseScene", "own")
s1.e("BaseScene", "World", "own")
s1.e("BaseScene", "GameObjectManager", "own")
s1.e("GameManager", "UserData", "own")
s1.e("GameManager", "InputActionManager", "own")
s1.note("シングルトンは 13 個。実体を持つのは MainEngine / SceneManager / GameManager で、\n"
        "残りは「どこからでも引ける置き場」として使われている。", -0.9, 8.6, 16)

# =====================================================================
# 2. Graphics
# =====================================================================
s2 = Sheet("② Graphics — 描画エンジンとレンダーグラフ",
           "カメラごとに GraphicsPipeline(実行インスタンス)を持ち、その中身が RenderGraph。")
s2.n("MainEngine", "MainEngine", 3.0, 0, "sing")
s2.n("GraphicsEngine", "GraphicsEngine", 3.0, 1)
s2.n("PipelineStateManager", "PipelineStateManager", 6.6, 1)
s2.n("MouseCursor", "MouseCursor", 8.0, 1)
s2.n("RenderContext", "RenderContext\n(フレーム数だけ)", 0.6, 2)
s2.n("CBAllocator", "CBAllocator", 0.6, 3)
s2.n("MeshBufferAllocator", "MeshBufferAllocator", 1.9, 2)
s2.n("PassMetaRegistry", "PassMetaRegistry", 4.6, 2.0)
s2.n("LightManager", "LightManager", 5.9, 2.0)
s2.n("QuadPolygon", "QuadPolygon", 7.1, 2.0)
s2.n("CameraPipelineData", "CameraPipelineData", 3.1, 2.6)
s2.n("GraphicsPipeline", "GraphicsPipeline", 3.1, 3.5)
s2.n("RenderGraph", "RenderGraph", 3.1, 4.4)
s2.n("RenderGraphCompiler", "RenderGraphCompiler", 1.5, 4.4)
s2.n("ResourceRegistry", "ResourceRegistry", 4.7, 4.0)
s2.n("ResourceAllocator", "ResourceAllocator", 6.0, 4.0)
s2.n("GraphHeap", "GraphHeap", 7.2, 4.0)
s2.n("VirtualResource", "VirtualResource", 5.4, 4.9)
s2.n("RenderingPipelineAsset", "RenderingPipelineAsset\n(設計図)", 1.3, 3.5)
s2.n("Pass", "Pass (基底)", 3.1, 5.4)
s2.n("PassContext", "PassContext", 1.4, 5.9, "struct")
s2.n("GBufferPass", "GBufferPass", 0.4, 7.0)
s2.n("ZPrePass", "ZPrePass", 1.6, 7.0)
s2.n("DeferredLightingPass", "DeferredLightingPass", 3.0, 7.0)
s2.n("RaytracingGIPass", "RaytracingGIPass", 4.5, 7.0)
s2.n("RaytracingShadowPass", "RaytracingShadowPass", 6.0, 7.0)
s2.n("SkyPass", "SkyPass", 7.3, 7.0)
s2.n("ParticlePass", "ParticlePass", 8.4, 7.0)
s2.n("MonitorPass", "MonitorPass", 9.5, 7.0)
s2.n("ParticleSimulation", "ParticleSimulation", 8.6, 2.6)
s2.n("SkinningPass", "SkinningPass", 8.6, 3.4)
s2.n("ShadingPipelineBuilder", "ShadingPipelineBuilder", 8.6, 4.2)
# 参照されるシングルトン
s2.n("D3D12Wrapper", "D3D12Wrapper", 0.2, 0, "sing")
s2.n("DescriptorHeapManager", "DescriptorHeapManager", 1.5, 0, "sing")
s2.n("ResourceManager", "ResourceManager", 4.5, 0, "sing")
s2.n("AssetDatabase", "AssetDatabase", 5.7, 0, "sing")
s2.n("OptionManager", "OptionManager", 6.8, 0, "sing")
s2.n("MainEditor", "MainEditor", 7.9, 0, "sing")
s2.n("SceneManager", "SceneManager", 9.0, 0, "sing")
s2.n("GameManager", "GameManager", 10.0, 0, "sing")
s2.n("RayEngine", "RayEngine", 9.6, 5.9, "sing")
s2.n("InputManager", "InputManager", 9.6, 1.0, "sing")

for t in ["D3D12Wrapper", "DescriptorHeapManager", "GameManager", "MainEditor",
          "MainEngine", "OptionManager", "ResourceManager", "SceneManager"]:
    s2.e("GraphicsEngine", t)
for t in ["D3D12Wrapper", "DescriptorHeapManager", "MainEditor", "MainEngine", "ResourceManager"]:
    s2.e("RenderContext", t)
s2.e("Pass", "ResourceManager")
s2.e("RenderGraph", "DescriptorHeapManager")
s2.e("VirtualResource", "D3D12Wrapper")
for t in ["AssetDatabase", "ResourceManager"]:
    s2.e("GBufferPass", t); s2.e("ZPrePass", t)
s2.e("DeferredLightingPass", "DescriptorHeapManager")
for t in ["D3D12Wrapper", "RayEngine"]:
    s2.e("RaytracingGIPass", t); s2.e("RaytracingShadowPass", t)
s2.e("SkyPass", "ResourceManager")
s2.e("ParticlePass", "MainEngine"); s2.e("ParticlePass", "ResourceManager")
s2.e("MonitorPass", "MainEngine")
s2.e("ParticleSimulation", "MainEngine"); s2.e("ParticleSimulation", "ResourceManager")
s2.e("SkinningPass", "ResourceManager")
s2.e("ShadingPipelineBuilder", "ResourceManager")
for t in ["DescriptorHeapManager", "InputManager", "MainEngine", "OptionManager", "ResourceManager"]:
    s2.e("MouseCursor", t)
s2.e("MainEngine", "GraphicsEngine", "own")
s2.e("MainEngine", "PipelineStateManager", "own")
s2.e("MainEngine", "MouseCursor", "own")
for t in ["RenderContext", "MeshBufferAllocator", "PassMetaRegistry", "LightManager",
          "QuadPolygon", "CameraPipelineData"]:
    s2.e("GraphicsEngine", t, "own")
s2.e("RenderContext", "CBAllocator", "own")
s2.e("CameraPipelineData", "GraphicsPipeline", "own")
s2.e("GraphicsPipeline", "RenderGraph", "own")
for t in ["Pass", "ResourceRegistry", "ResourceAllocator", "GraphHeap"]:
    s2.e("RenderGraph", t, "own")
s2.e("ResourceRegistry", "VirtualResource", "own")
s2.e("RenderGraphCompiler", "RenderGraph", "ref")
s2.e("RenderingPipelineAsset", "RenderGraph", "ref")
s2.e("Pass", "PassContext", "ref")
for t in ["RenderGraph", "GraphicsEngine", "RenderContext"]:
    s2.e("PassContext", t, "ref")
for p in ["GBufferPass", "ZPrePass", "DeferredLightingPass", "RaytracingGIPass",
          "RaytracingShadowPass", "SkyPass", "ParticlePass", "MonitorPass"]:
    s2.e(p, "Pass", "inherit")
s2.note("パスは 30 種類以上ある。ここに出しているのは\nシングルトンを直接引いているものだけ。", 0.0, 8.4, 16)

# =====================================================================
# 3. Scene / ECS / GameObject
# =====================================================================
s3 = Sheet("③ Scene / ECS / GameObject — シーンの中身",
           "EngineServices と各種 Context が『シングルトンを名指ししない』ための経路。")
s3.n("SceneManager", "SceneManager", 3.0, 0, "sing")
s3.n("BaseScene", "BaseScene", 3.0, 1)
s3.n("World", "World", 1.6, 2.2)
s3.n("GameObjectManager", "GameObjectManager", 5.2, 2.2)
s3.n("EntityManager", "EntityManager", 0.0, 3.3)
s3.n("SystemManager", "SystemManager", 1.2, 3.3)
s3.n("ArchetypeChunkManager", "ArchetypeChunkManager", 2.6, 3.3)
s3.n("ComponentMetaRegistry", "ComponentMetaRegistry", 4.0, 3.3)
s3.n("ResourceWrapper", "IResourceWrapper\n(ワールド寿命のリソース)", 0.5, 4.4)
s3.n("CollisionWorld", "CollisionWorld", 2.0, 4.4)
s3.n("ISystem", "ISystem", 1.2, 4.4)
s3.n("SystemContext", "SystemContext\n(pWorld / pServices / dt)", 1.2, 5.4, "struct")
s3.n("EngineServices", "EngineServices\n(アプリ寿命の置き場)", 3.4, 6.4, "struct")
s3.n("BaseObject", "BaseObject", 5.8, 3.3)
s3.n("ObjectContext", "ObjectContext\n(pWorld / pServices / pObjectManager)", 5.4, 5.4, "struct")
s3.n("ObjectMetaRegistry", "ObjectMetaRegistry", 7.2, 2.2, "sing")
s3.n("Prefab", "Prefab", 7.2, 3.6)
# EngineServices が配る先(すべてシングルトン)
s3.n("MainEngine", "MainEngine", 0.4, 7.6, "sing")
s3.n("ResourceManager", "ResourceManager", 1.6, 7.6, "sing")
s3.n("AssetDatabase", "AssetDatabase", 2.8, 7.6, "sing")
s3.n("InputManager", "InputManager", 3.9, 7.6, "sing")
s3.n("MainEditor", "MainEditor", 5.0, 7.6, "sing")
s3.n("RayEngine", "RayEngine", 6.0, 7.6, "sing")
s3.n("AudioManager", "AudioManager", 7.0, 7.6, "sing")
s3.n("JobSystem", "JobSystem", 8.1, 7.6)
s3.n("OptionManager", "OptionManager", 9.2, 7.6, "sing")

s3.e("SceneManager", "BaseScene", "own")
s3.e("BaseScene", "World", "own")
s3.e("BaseScene", "GameObjectManager", "own")
for t in ["EntityManager", "SystemManager", "ArchetypeChunkManager",
          "ComponentMetaRegistry", "ResourceWrapper"]:
    s3.e("World", t, "own")
s3.e("World", "EngineServices", "own")
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
for t in ["MainEngine", "ResourceManager", "AssetDatabase", "InputManager", "MainEditor",
          "RayEngine", "AudioManager", "JobSystem", "OptionManager"]:
    s3.e("EngineServices", t, "ref")
for t in ["AssetDatabase", "AudioManager", "InputManager", "MainEditor", "MainEngine",
          "OptionManager", "RayEngine", "ResourceManager", "SceneManager"]:
    s3.e("BaseScene", t)
s3.e("World", "MainEditor")
s3.e("ComponentMetaRegistry", "MainEditor")
s3.e("GameObjectManager", "ObjectMetaRegistry")
for t in ["AssetDatabase", "ResourceManager", "SceneManager"]:
    s3.e("Prefab", t)
s3.e("CollisionWorld", "MainEditor")
s3.e("CollisionWorld", "ResourceManager")
s3.note("EngineServices はシングルトンの実体を集めて配る箱。\n"
        "System / GameObject はここから引くので、自分では Instance() を呼ばない。", -0.9, 9.0, 16)

# =====================================================================
# 4. Resource
# =====================================================================
s4 = Sheet("④ Resource — アセットとリソースの実体化",
           "AssetDatabase = ファイルの台帳 / ResourceManager = 実体の置き場。生成は ResourceBuildContext 経由。")
s4.n("ResourceManager", "ResourceManager", 2.0, 0, "sing")
s4.n("AssetDatabase", "AssetDatabase", 5.0, 0, "sing")
s4.n("MainEditor", "MainEditor", 7.6, 0, "sing")
s4.n("D3D12Wrapper", "D3D12Wrapper", 8.9, 0, "sing")
s4.n("MainEngine", "MainEngine", 0.0, 0, "sing")
s4.n("AudioManager", "AudioManager", 10.0, 0, "sing")
s4.n("SceneManager", "SceneManager", 6.3, 0, "sing")
s4.n("OptionManager", "OptionManager", 11.1, 0, "sing")
s4.n("ScopedResourceBuild", "ScopedResourceBuild\n(モデル1体分をまとめて submit)", 1.2, 1.4)
s4.n("ResourceBuildContext", "ResourceBuildContext", 4.2, 1.4, "struct")
s4.n("MeshBufferAllocator", "MeshBufferAllocator", 6.6, 1.4)
s4.n("PassMetaRegistry", "PassMetaRegistry", 8.2, 1.4)
# 実体
s4.n("Model", "Model", 0.4, 2.8)
s4.n("Mesh", "Mesh", 1.5, 2.8)
s4.n("Material", "Material", 2.6, 2.8)
s4.n("Texture", "Texture", 3.7, 2.8)
s4.n("Shader", "Shader", 4.8, 2.8)
s4.n("AnimatorAsset", "AnimatorAsset", 6.0, 2.8)
s4.n("ActionStateMachineAsset", "ActionStateMachineAsset", 7.6, 2.8)
s4.n("EffectAsset", "EffectAsset", 9.2, 2.8)
s4.n("ParticlesAsset", "ParticlesAsset", 10.5, 2.8)
s4.n("Sound", "Sound", 11.6, 2.8)
s4.n("Prefab", "Prefab", 12.6, 2.8)
s4.n("Font", "Font", 13.5, 2.8)
s4.n("ShadingModelTable", "ShadingModelTable", 14.7, 2.8)
# IO
s4.n("ModelIO", "ModelIO", 0.0, 4.2)
s4.n("ModelConverter", "ModelConverter", 1.1, 4.2)
s4.n("ModelProcessor", "ModelProcessor", 1.1, 5.2)
s4.n("MeshIO", "MeshIO", 2.3, 4.2)
s4.n("MaterialIO", "MaterialIO", 3.3, 4.2)
s4.n("TextureIO", "TextureIO", 4.4, 4.2)
s4.n("TextureImporter", "TextureImporter", 4.4, 5.2)
s4.n("ShaderIO", "ShaderIO", 5.6, 4.2)
s4.n("DXCCompiler", "DXCCompiler", 5.6, 5.2)
s4.n("AnimatorAssetIO", "AnimatorAssetIO", 6.9, 4.2)
s4.n("ActionStateMachineAssetIO", "ActionStateMachineAssetIO", 8.6, 4.2)
s4.n("EffectAssetIO", "EffectAssetIO", 10.1, 4.2)
s4.n("ParticlesIO", "ParticlesIO", 11.3, 4.2)
s4.n("SoundIO", "SoundIO", 12.3, 4.2)
s4.n("AudioBehaviorIO", "AudioBehaviorIO", 13.4, 4.2)
s4.n("ShadingModelTableIO", "ShadingModelTableIO", 14.9, 4.2)
s4.n("RenderingPipelineAssetIO", "RenderingPipelineAssetIO", 16.6, 4.2)
s4.n("RenderingPipelineAsset", "RenderingPipelineAsset", 16.6, 2.8)

s4.e("ResourceManager", "AssetDatabase")
s4.e("AssetDatabase", "MainEditor")
for t in ["AssetDatabase", "D3D12Wrapper", "MainEngine", "ResourceManager"]:
    s4.e("ScopedResourceBuild", t)
for t in ["ResourceManager", "AssetDatabase", "MeshBufferAllocator", "PassMetaRegistry"]:
    s4.e("ResourceBuildContext", t, "ref")
s4.e("ScopedResourceBuild", "ResourceBuildContext", "own")
for a, ts in [
    ("Model", ["AssetDatabase", "ResourceManager"]),
    ("Mesh", ["MainEngine"]),
    ("Material", ["AssetDatabase", "OptionManager", "ResourceManager"]),
    ("Texture", ["D3D12Wrapper", "ResourceManager"]),
    ("AnimatorAsset", ["AssetDatabase", "MainEditor", "ResourceManager"]),
    ("ActionStateMachineAsset", ["AssetDatabase", "MainEditor", "ResourceManager"]),
    ("EffectAsset", ["ResourceManager"]),
    ("ParticlesAsset", ["ResourceManager"]),
    ("Sound", ["AudioManager", "ResourceManager"]),
    ("Prefab", ["AssetDatabase", "ResourceManager", "SceneManager"]),
    ("Font", ["ResourceManager"]),
    ("ShadingModelTable", ["ResourceManager"]),
    ("ModelIO", ["AssetDatabase", "ResourceManager"]),
    ("ModelConverter", ["AssetDatabase", "ResourceManager"]),
    ("MaterialIO", ["ResourceManager"]),
    ("TextureIO", ["AssetDatabase", "ResourceManager"]),
    ("TextureImporter", ["D3D12Wrapper"]),
    ("ShaderIO", ["AssetDatabase", "ResourceManager"]),
    ("AnimatorAssetIO", ["AssetDatabase", "ResourceManager"]),
    ("ActionStateMachineAssetIO", ["AssetDatabase", "ResourceManager"]),
    ("EffectAssetIO", ["AssetDatabase", "ResourceManager"]),
    ("ParticlesIO", ["AssetDatabase", "ResourceManager"]),
    ("SoundIO", ["AudioManager"]),
    ("AudioBehaviorIO", ["AssetDatabase", "ResourceManager"]),
    ("ShadingModelTableIO", ["AssetDatabase", "ResourceManager"]),
    ("RenderingPipelineAssetIO", ["AssetDatabase", "ResourceManager"]),
]:
    for t in ts:
        s4.e(a, t)
for a, b in [("Model", "ModelIO"), ("ModelIO", "ModelConverter"), ("ModelConverter", "ModelProcessor"),
             ("Mesh", "MeshIO"), ("Material", "MaterialIO"), ("Texture", "TextureIO"),
             ("TextureIO", "TextureImporter"), ("Shader", "ShaderIO"), ("ShaderIO", "DXCCompiler"),
             ("AnimatorAsset", "AnimatorAssetIO"), ("ActionStateMachineAsset", "ActionStateMachineAssetIO"),
             ("EffectAsset", "EffectAssetIO"), ("ParticlesAsset", "ParticlesIO"),
             ("Sound", "SoundIO"), ("ShadingModelTable", "ShadingModelTableIO"),
             ("RenderingPipelineAsset", "RenderingPipelineAssetIO")]:
    s4.e(a, b, "ref")
for t in ["Model", "Mesh", "Material", "Texture", "Shader", "AnimatorAsset",
          "ActionStateMachineAsset", "EffectAsset", "ParticlesAsset", "Sound",
          "Prefab", "Font", "ShadingModelTable", "RenderingPipelineAsset"]:
    s4.e("ResourceManager", t, "own")
s4.note("ResourceManager が ResourceData<T> で実体を持つ(所有)。\n"
        "IO は実体を作る側で、生成に必要なものは ResourceBuildContext から受け取るのが方針。\n"
        "※ 図の緑矢印は、その方針から外れて今もシングルトンを直接引いている箇所。", -0.9, 6.4, 16)

# =====================================================================
# 5. Editor
# =====================================================================
s5 = Sheet("⑤ Editor — MainEditor とパネル",
           "MainEditor が ImGui とパネルを抱え、パネル側は必要なシングルトンを直接引いている。")
s5.n("MainEditor", "MainEditor", 3.0, 0, "sing")
s5.n("ImGuiContext", "ImGuiContext", 0.4, 1.2)
s5.n("PanelManager", "PanelManager", 2.0, 1.2)
s5.n("Profiler", "Profiler", 4.0, 1.2)
s5.n("EditorCamera", "EditorCamera", 5.4, 1.2)
s5.n("EffectEditor", "EffectEditor", 6.8, 1.2)
s5.n("CPUProfiler", "CPUProfiler", 3.6, 2.2)
s5.n("GPUProfiler", "GPUProfiler", 4.8, 2.2)
s5.n("EditorContext", "EditorContext", 1.0, 2.2, "struct")
s5.n("IPanel", "IPanel", 2.2, 2.2)
s5.n("HierarchyPanel", "HierarchyPanel", 0.2, 3.4)
s5.n("GameObjectHierarchyPanel", "GameObjectHierarchyPanel", 1.9, 3.4)
s5.n("InspectorPanel", "InspectorPanel", 3.5, 3.4)
s5.n("SceneViewPanel", "SceneViewPanel", 4.8, 3.4)
s5.n("AssetDataBasePanel", "AssetDataBasePanel", 6.2, 3.4)
s5.n("ProfilerPanel", "ProfilerPanel", 7.6, 3.4)
s5.n("OptionPanel", "OptionPanel", 8.8, 3.4)
s5.n("LogPanel", "LogPanel", 9.9, 3.4)
s5.n("RGResourceViewPanel", "RenderGraphResourceViewPanel", 11.4, 3.4)
s5.n("EntityInspector", "EntityInspector", 2.6, 4.5)
s5.n("AssetInspector", "AssetInspector", 4.0, 4.5)
s5.n("ResourceDraw", "ResourceDraw\n(アセット種別ごとの編集)", 4.0, 5.5)
s5.n("AssetLink", "AssetLink", 5.6, 5.5)
s5.n("EditorHelper", "EditorHelper", 7.0, 5.5)
# 参照先
s5.n("MainEngine", "MainEngine", 0.0, 6.8, "sing")
s5.n("SceneManager", "SceneManager", 1.2, 6.8, "sing")
s5.n("ResourceManager", "ResourceManager", 2.5, 6.8, "sing")
s5.n("AssetDatabase", "AssetDatabase", 3.8, 6.8, "sing")
s5.n("DescriptorHeapManager", "DescriptorHeapManager", 5.3, 6.8, "sing")
s5.n("D3D12Wrapper", "D3D12Wrapper", 6.8, 6.8, "sing")
s5.n("OptionManager", "OptionManager", 8.0, 6.8, "sing")
s5.n("AudioManager", "AudioManager", 9.2, 6.8, "sing")
s5.n("ObjectMetaRegistry", "ObjectMetaRegistry", 10.5, 6.8, "sing")

for t in ["ImGuiContext", "PanelManager", "Profiler", "EditorCamera", "EffectEditor"]:
    s5.e("MainEditor", t, "own")
s5.e("Profiler", "CPUProfiler", "own")
s5.e("Profiler", "GPUProfiler", "own")
s5.e("PanelManager", "IPanel", "own")
s5.e("PanelManager", "EditorContext", "own")
for p in ["HierarchyPanel", "GameObjectHierarchyPanel", "InspectorPanel", "SceneViewPanel",
          "AssetDataBasePanel", "ProfilerPanel", "OptionPanel", "LogPanel", "RGResourceViewPanel"]:
    s5.e(p, "IPanel", "inherit")
s5.e("InspectorPanel", "EntityInspector", "own")
s5.e("InspectorPanel", "AssetInspector", "own")
s5.e("AssetInspector", "ResourceDraw", "ref")
s5.e("AssetInspector", "AssetLink", "ref")
s5.e("MainEditor", "MainEngine")
s5.e("MainEditor", "OptionManager")
for a, ts in [
    ("EditorCamera", ["OptionManager"]),
    ("EffectEditor", ["AssetDatabase", "AudioManager", "DescriptorHeapManager", "MainEditor",
                      "MainEngine", "OptionManager", "ResourceManager"]),
    ("EditorHelper", ["AssetDatabase", "DescriptorHeapManager", "ResourceManager"]),
    ("ImGuiContext", ["D3D12Wrapper", "DescriptorHeapManager"]),
    ("AssetDataBasePanel", ["AssetDatabase", "MainEngine", "SceneManager"]),
    ("GameObjectHierarchyPanel", ["ObjectMetaRegistry", "SceneManager"]),
    ("HierarchyPanel", ["AssetDatabase", "ResourceManager", "SceneManager"]),
    ("AssetLink", ["AssetDatabase"]),
    ("ResourceDraw", ["AssetDatabase", "ResourceManager", "SceneManager"]),
    ("EntityInspector", ["AssetDatabase", "ResourceManager", "SceneManager"]),
    ("InspectorPanel", ["SceneManager"]),
    ("OptionPanel", ["OptionManager"]),
    ("ProfilerPanel", ["D3D12Wrapper", "MainEngine"]),
    ("RGResourceViewPanel", ["DescriptorHeapManager", "MainEngine"]),
    ("SceneViewPanel", ["AssetDatabase", "DescriptorHeapManager", "MainEditor", "MainEngine",
                        "OptionManager", "ResourceManager", "SceneManager"]),
    ("Profiler", ["D3D12Wrapper"]),
    ("GPUProfiler", ["D3D12Wrapper"]),
]:
    for t in ts:
        s5.e(a, t)
s5.note("SceneViewPanel と EffectEditor がいちばん多くのシングルトンを直接引いている。", -0.9, 8.0, 16)

# =====================================================================
# 6. D3D12 / Raytracing
# =====================================================================
s6 = Sheet("⑥ D3D12 / Raytracing — GPU まわりの下層",
           "D3D12Wrapper と DescriptorHeapManager が互いを直接引き合っている。")
s6.n("MainEngine", "MainEngine", 3.0, 0, "sing")
s6.n("D3D12Wrapper", "D3D12Wrapper", 1.2, 1.2, "sing")
s6.n("DescriptorHeapManager", "DescriptorHeapManager", 4.4, 1.2, "sing")
s6.n("CommandContext", "CommandContext", 0.0, 2.4)
s6.n("FrameManager", "FrameManager", 1.2, 2.4)
s6.n("AsyncGPUManager", "AsyncGPUManager", 2.4, 2.4)
s6.n("DescriptorHeap", "DescriptorHeap\n(CBV_SRV_UAV / RTV / DSV / Sampler / ImGui)", 4.6, 2.4)
s6.n("HeapAllocator", "HeapAllocator<T>", 6.6, 2.4)
s6.n("SamplerAllocator", "SamplerAllocator", 8.0, 2.4)
s6.n("GPUResource", "GPUResource", 1.0, 3.6)
s6.n("GPUBuffer", "GPUBuffer", 2.2, 3.6)
s6.n("StaticBuffer", "StaticBuffer", 3.3, 3.6)
s6.n("MegaBuffer", "MegaBuffer", 4.4, 3.6)
s6.n("Texture", "Texture", 5.5, 3.6)
s6.n("RootSignatureBuilder", "RootSignatureBuilder", 6.9, 3.6)
s6.n("RootSignature", "RootSignature", 8.3, 3.6)
s6.n("PipelineState", "PipelineState", 9.5, 3.6)
s6.n("PipelineStateManager", "PipelineStateManager", 8.3, 1.2)
s6.n("MainEditor", "MainEditor", 9.7, 1.2, "sing")
s6.n("ResourceManager", "ResourceManager", 6.6, 0.2, "sing")
# レイトレ
s6.n("RayEngine", "RayEngine", 1.5, 5.0, "sing")
s6.n("RayWorld", "RayWorld", 1.5, 6.0)
s6.n("TLAS", "TLAS", 0.6, 7.0)
s6.n("BLAS", "BLAS", 1.8, 7.0)
s6.n("ShaderTable", "ShaderTable", 3.0, 7.0)
s6.n("RayPSO", "RayPSO", 4.2, 7.0)

s6.e("MainEngine", "D3D12Wrapper")
s6.e("MainEngine", "DescriptorHeapManager")
s6.e("MainEngine", "RayEngine")
s6.e("D3D12Wrapper", "DescriptorHeapManager")
s6.e("DescriptorHeapManager", "D3D12Wrapper")
for t in ["CommandContext", "FrameManager", "AsyncGPUManager"]:
    s6.e("D3D12Wrapper", t, "own")
for t in ["DescriptorHeap", "HeapAllocator", "SamplerAllocator"]:
    s6.e("DescriptorHeapManager", t, "own")
s6.e("MainEngine", "PipelineStateManager", "own")
s6.e("GPUResource", "DescriptorHeapManager")
s6.e("GPUBuffer", "DescriptorHeapManager")
s6.e("StaticBuffer", "DescriptorHeapManager")
s6.e("MegaBuffer", "D3D12Wrapper")
s6.e("Texture", "D3D12Wrapper")
s6.e("Texture", "DescriptorHeapManager")
s6.e("RootSignatureBuilder", "D3D12Wrapper")
s6.e("RootSignature", "D3D12Wrapper")
s6.e("PipelineState", "MainEditor")
s6.e("PipelineStateManager", "RootSignature", "own")
s6.e("PipelineStateManager", "PipelineState", "own")
s6.e("RayEngine", "RayWorld", "own")
s6.e("RayWorld", "TLAS", "own")
s6.e("RayWorld", "DescriptorHeapManager")
s6.e("RayWorld", "ResourceManager")
s6.e("BLAS", "MainEngine")
s6.e("ShaderTable", "DescriptorHeapManager")
s6.e("ShaderTable", "ResourceManager")
s6.e("TLAS", "DescriptorHeapManager")
s6.e("GPUBuffer", "GPUResource", "inherit")
s6.e("StaticBuffer", "GPUBuffer", "inherit")
s6.e("MegaBuffer", "GPUBuffer", "inherit")
s6.e("Texture", "GPUResource", "inherit")
s6.note("GPU リソース側(GPUResource / Texture / Buffer)は、\nディスクリプタを取るために DescriptorHeapManager を直接引いている。", -0.9, 8.2, 16)

# =====================================================================
# 7. Input / Audio / Option / Particle / Job
# =====================================================================
s7 = Sheet("⑦ Input / Audio / Option / Particle / JobSystem",
           "アプリ寿命のサービス群。Option は各シングルトンへ設定を流し込む側でもある。")
s7.n("MainEngine", "MainEngine", 3.0, 0, "sing")
s7.n("NativeWindow", "NativeWindow", 0.0, 1.2)
s7.n("InputManager", "InputManager", 1.3, 1.2, "sing")
s7.n("AudioManager", "AudioManager", 3.0, 1.2, "sing")
s7.n("OptionManager", "OptionManager", 4.6, 1.2, "sing")
s7.n("ParticleBufferManager", "ParticleBufferManager", 6.4, 1.2)
s7.n("JobSystem", "JobSystem", 8.2, 1.2)
s7.n("InputCollector", "InputCollector", 1.0, 2.4)
s7.n("InputAxisBase", "InputAxisBase", 0.3, 3.4)
s7.n("InputButtonBase", "InputButtonBase", 1.6, 3.4)
s7.n("InputAxisForWindows", "InputAxisForWindows", 0.0, 4.4)
s7.n("InputAxisForXInput", "InputAxisForXInput", 1.3, 4.4)
s7.n("InputButtonForWindows", "InputButtonForWindows", 2.7, 4.4)
s7.n("InputButtonForWindowsChord", "InputButtonForWindowsChord", 4.3, 4.4)
s7.n("InputButtonForXInput", "InputButtonForXInput", 5.8, 4.4)
s7.n("AudioEngineDX", "DirectX::AudioEngine", 2.6, 2.4)
s7.n("SoundInstancePool", "SoundInstance プール", 3.9, 2.4)
s7.n("IOption", "IOption", 5.2, 2.4)
s7.n("WindowOption", "WindowOption", 5.0, 3.4)
s7.n("AudioOption", "AudioOption", 6.2, 3.4)
s7.n("InputOption", "InputOption", 7.3, 3.4)
s7.n("OtherOption", "RenderingOption / GIOption /\nBloomOption / CursorOption ...", 9.0, 3.4)
s7.n("GPUParticlePool", "GPUParticlePool", 6.4, 2.4)
s7.n("JobContext", "JobContext", 7.7, 2.4)
s7.n("JobWorker", "JobWorker", 8.9, 2.4)
s7.n("AssetDatabase", "AssetDatabase", 8.6, 0.2, "sing")
s7.n("ResourceManager", "ResourceManager", 7.2, 0.2, "sing")
s7.n("D3D12Wrapper", "D3D12Wrapper", 10.0, 0.2, "sing")
s7.n("GameManager", "GameManager", 0.0, 5.6, "sing")
s7.n("InputActionManager", "InputActionManager", 1.6, 5.6)
s7.n("UserData", "UserData", 3.0, 5.6)
s7.n("MainEditor", "MainEditor", 4.4, 5.6, "sing")
s7.n("SceneManager", "SceneManager", 5.7, 5.6, "sing")
s7.n("ObjectMetaRegistry", "ObjectMetaRegistry", 7.2, 5.6, "sing")

for t in ["InputManager", "AudioManager", "OptionManager"]:
    s7.e("MainEngine", t)
s7.e("MainEngine", "ParticleBufferManager", "own")
s7.e("MainEngine", "JobSystem", "own")
s7.e("MainEngine", "NativeWindow", "own")
s7.e("NativeWindow", "InputManager")
s7.e("InputManager", "MainEngine")
s7.e("InputManager", "OptionManager")
s7.e("InputManager", "InputCollector", "own")
s7.e("InputCollector", "InputAxisBase", "own")
s7.e("InputCollector", "InputButtonBase", "own")
for a, b in [("InputAxisForWindows", "InputAxisBase"), ("InputAxisForXInput", "InputAxisBase"),
             ("InputButtonForWindows", "InputButtonBase"),
             ("InputButtonForWindowsChord", "InputButtonBase"),
             ("InputButtonForXInput", "InputButtonBase")]:
    s7.e(a, b, "inherit")
s7.e("AudioManager", "AudioEngineDX", "own")
s7.e("AudioManager", "SoundInstancePool", "own")
s7.e("AudioManager", "AssetDatabase")
s7.e("AudioManager", "ResourceManager")
for t in ["WindowOption", "AudioOption", "InputOption", "OtherOption"]:
    s7.e("OptionManager", t, "own")
for a, b in [("WindowOption", "IOption"), ("AudioOption", "IOption"),
             ("InputOption", "IOption"), ("OtherOption", "IOption")]:
    s7.e(a, b, "inherit")
s7.e("WindowOption", "MainEngine")
s7.e("AudioOption", "AudioManager")
s7.e("InputOption", "InputManager")
s7.e("ParticleBufferManager", "GPUParticlePool", "own")
s7.e("ParticleBufferManager", "AssetDatabase")
s7.e("ParticleBufferManager", "D3D12Wrapper")
s7.e("GPUParticlePool", "MainEngine")
s7.e("GPUParticlePool", "ResourceManager")
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
        "InputActionManager が InputManager へ流し込む。", -0.9, 6.8, 16)

# =====================================================================
# 改善点(各シートの右下)
# =====================================================================
s1.imp(
    ("シングルトンが13個。実体を持っているのは3つだけ", [
        "MainEngine / SceneManager / GameManager 以外の10個は",
        "「どこからでも引ける置き場」になっていて、依存の向きが追えない。",
        "MainEngine が起動時に組み立てて配れば緑矢印はかなり減らせる。"]),
    ("相互に引き合っている組がある", [
        "D3D12Wrapper ⇄ DescriptorHeapManager",
        "OptionManager ⇄ MainEngine / InputManager / AudioManager",
        "初期化順と解放順が暗黙の了解になっている。片方向に倒したい。"]),
    ("BaseScene が9個のシングルトンを直接引いている", [
        "EngineServices を組み立てているのは BaseScene 自身なので、",
        "作ったあとは自分もそこ経由で引けば直引きを消せる。"]),
    ("Application が MainEditor を15箇所で叩いている", [
        "中身はプロファイラのタイマー。スコープガード(RAII)かマクロに包めば、",
        "アプリの入口からエディター依存が消える。"]),
)

s2.imp(
    ("PassContext にリソースへの経路が無い", [
        "30種以上あるパスが ResourceManager / AssetDatabase / RayEngine を直引き。",
        "PassContext は既に RenderGraph / GraphicsEngine / RenderContext を配って",
        "いるので、ここへ足せばパス側の緑矢印をまとめて消せる。"]),
    ("GraphicsEngine → SceneManager / GameManager は層の逆流", [
        "描画層がシーンとゲームを見に行っている。",
        "必要なデータはシーン更新側から積む形にしたい。"]),
    ("MainEngine 直引きは「持ち物が欲しいだけ」", [
        "ParticlePass / ParticleSimulation / MonitorPass が欲しいのは",
        "ParticleBufferManager や PipelineStateManager。",
        "PassContext か初期化時の引数で渡せば済む。"]),
    ("計測のためだけに描画層がエディターを見ている", [
        "RenderContext / GraphicsEngine → MainEditor はプロファイル用。",
        "計測インターフェースを切って挟みたい。"]),
)

s3.imp(
    ("経路は用意できているが、使い切れていない", [
        "EngineServices / SystemContext / ObjectContext の3経路は機能している。",
        "ただしアプリ側にはまだ直引きが残っている(この図には未掲載):",
        "GunShootSystem → ResourceManager / ScoreSystem → GameManager /",
        "CameraStartSystem → OptionManager / 各 Sequence → SceneManager"]),
    ("コンポーネントのヘルパーがヘッダーで Instance() を呼んでいる", [
        "ModelComponent / ParticlesComponent / SoundComponent など。",
        "コンポーネントは POD なので、取得と返却はシステム側か",
        "ComponentTraits::Release 経由に寄せたい。"]),
    ("BaseScene だけが例外で9個を直引き", [
        "EngineServices を作る当人なので、作ったあとは自分も経由する。"]),
    ("ECS がエディターを見ている", [
        "World.h / ComponentMetaRegistry → MainEditor はログ目的。",
        "ログ用の細いインターフェースを挟めば ECS からエディターが消える。"]),
    ("Prefab / CollisionWorld の直引き", [
        "Prefab は生成に SceneManager、CollisionWorld は ResourceManager が要る。",
        "どちらも引数で World / Context を受け取れば片付く。"]),
)

s4.imp(
    ("決めた方針(ResourceBuildContext 経由)がまだ徹底されていない", [
        "ResourceManager は157箇所、AssetDatabase は95箇所から直接呼ばれている。",
        "この図の緑矢印はほぼ全部それ。IO の入口の引数を Context に",
        "統一するのが一番効く。"]),
    ("Context に足りないものがある", [
        "Material は OptionManager、Mesh は MainEngine(遅延解放)を直引き。",
        "足りない分を Context に載せれば、直引きする理由が無くなる。"]),
    ("ResourceManager.h が自分自身の Instance() を呼んでいる", [
        "ヘッダーのテンプレート実装の中で12箇所。",
        "呼び出し側が「どの ResourceManager か」を選べない。"]),
    ("実体と IO が相互に絡んでいる", [
        "Model / AnimatorAsset などが IO から呼ばれつつ、自分でも Instance() を引く。",
        "読み込みの入口を IO 側に寄せて、実体は受け取るだけにしたい。"]),
)

s5.imp(
    ("EditorContext があるのに使い切れていない", [
        "PanelManager が EditorContext を持っているのに、パネルは各自 Instance()。",
        "よく使うもの(MainEngine / SceneManager / ResourceManager / AssetDatabase)",
        "を載せれば、パネル側は Context だけ見ればよくなる。"]),
    ("SceneViewPanel(7個) と EffectEditor(7個) が突出している", [
        "EffectEditor は専用ワールドを持つほぼミニアプリ。",
        "パネルではなく別レイヤーとして切り出したほうが見通しがよい。"]),
    ("エンジン側からエディターへの逆向き依存", [
        "AssetDatabase / Archive / PipelineState / World / ComponentMetaRegistry /",
        "CollisionWorld / BVHTraverser が MainEditor を直引き。",
        "用途はログ・計測・デバッグ描画なので、そこだけ細いインターフェースを",
        "切って挟めば、エンジンがエディターを知らなくて済む。"]),
    ("エディター自身の直引きは許容範囲", [
        "アプリ寿命のツール層なので、上の逆向き依存だけ直せば十分。"]),
)

s6.imp(
    ("D3D12Wrapper ⇄ DescriptorHeapManager の相互参照", [
        "DescriptorHeapManager が欲しいのは Device だけ。",
        "初期化時に受け取って保持すれば片方向になる。"]),
    ("GPU リソースがディスクリプタ確保のため直引きしている", [
        "GPUResource / Texture / Buffer 群 → DescriptorHeapManager。",
        "生成時にアロケーターの参照を渡す(ResourceBuildContext と同じ考え方)。"]),
    ("BLAS → MainEngine は遅延解放の登録だけ", [
        "「解放を後回しにする」インターフェースを切って渡せば依存が消える。"]),
    ("PipelineState → MainEditor はログ", [
        "シェーダー再コンパイルの結果表示。ログ経路を分ければよい。"]),
)

s7.imp(
    ("OptionManager と各マネージャーが相互に引き合っている", [
        "WindowOption → MainEngine / AudioOption → AudioManager /",
        "InputOption → InputManager。Option 側から押し込むのをやめ、",
        "受け手が起動時と変更通知で読む形にすると片方向になる。"]),
    ("所有されている側が Instance() で親を引いている", [
        "ParticleBufferManager / GPUParticlePool / MouseCursor → MainEngine。",
        "所有者が初期化時に必要なものを渡せばよい。"]),
    ("InputActionManager → MainEditor はモード判定", [
        "アプリのモードを持っているのは MainEngine。",
        "そちらから取ればゲーム側がエディターを見なくて済む。"]),
    ("GameManager がゲーム側の各所から引かれている", [
        "Sequence / ScoreHUD / ScoreSystem に加えて GraphicsEngine からも。",
        "描画層からゲーム層への逆流は特に切り離したい。"]),
)

# =====================================================================
OUT = os.path.dirname(os.path.abspath(__file__))
OUT = os.environ.get("OUTDIR", OUT)
os.makedirs(OUT, exist_ok=True)
for fn, sh, lg in [
    ("01_Overview.excalidraw", s1, (-XP * 0.9, YP * 9.6)),
    ("02_Graphics.excalidraw", s2, (-XP * 0.9, YP * 9.2)),
    ("03_Scene_ECS_GameObject.excalidraw", s3, (-XP * 0.9, YP * 10.0)),
    ("04_Resource.excalidraw", s4, (-XP * 0.9, YP * 7.6)),
    ("05_Editor.excalidraw", s5, (-XP * 0.9, YP * 9.0)),
    ("06_D3D12_Raytracing.excalidraw", s6, (-XP * 0.9, YP * 9.2)),
    ("07_Input_Audio_Option.excalidraw", s7, (-XP * 0.9, YP * 7.8)),
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
