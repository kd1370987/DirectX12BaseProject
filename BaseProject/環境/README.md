# 依存関係図

BaseProject のクラス依存を Excalidraw 形式で書き出したもの。
`.excalidraw` を [excalidraw.com](https://excalidraw.com) にドラッグ&ドロップすると開ける。

## 図の一覧

| ファイル | 中身 |
|---|---|
| `01_Overview.excalidraw` | Application → 13個のシングルトン → MainEngine が抱える実体 |
| `02_Graphics.excalidraw` | GraphicsEngine / RenderContext / GraphicsPipeline → RenderGraph → Pass |
| `03_Scene_ECS_GameObject.excalidraw` | SceneManager → BaseScene → World / GameObjectManager、EngineServices の配り方 |
| `04_Resource.excalidraw` | ResourceManager / AssetDatabase / ResourceBuildContext と各アセットの IO |
| `05_Editor.excalidraw` | MainEditor → ImGuiContext / PanelManager / Profiler と各パネル |
| `06_D3D12_Raytracing.excalidraw` | D3D12Wrapper / DescriptorHeapManager / RayEngine 配下 |
| `07_Input_Audio_Option.excalidraw` | InputManager / AudioManager / OptionManager / Particle / JobSystem |

`Preview/*.svg` は同じ配置の確認用。ブラウザや VS でそのまま開ける。

## 表記

| 見た目 | 意味 |
|---|---|
| 緑の枠 | シングルトン(`static X& Instance()` を持つクラス) |
| 白の枠 | 通常のクラス |
| 白の破線枠 | 引数で配るコンテキスト構造体(EngineServices / SystemContext / ObjectContext / PassContext / ResourceBuildContext) |
| 緑の矢印 | シングルトン経由の依存。`X::Instance()` を直接呼んでいる |
| 白の実線矢印 | 所有(`unique_ptr` / 値メンバ) |
| 白の破線矢印 | 参照(引数・ポインタで受け取る) |
| 白の点線矢印 | 継承 |
| 橙の枠(右下) | 改善点。そのシートで気になった依存と、直し方の当たり |

色は Excalidraw の**ライトモードの値**(黒/緑)で保存している。
Excalidraw のダークモードは描画時に色を反転するので、ダークテーマで開くと
「黒背景・白枠・緑枠」に見える。

## シングルトン(13個)

`MainEngine` / `SceneManager` / `GameManager` / `MainEditor` / `ResourceManager` /
`AssetDatabase` / `InputManager` / `AudioManager` / `OptionManager` / `D3D12Wrapper` /
`DescriptorHeapManager` / `RayEngine` / `ObjectMetaRegistry`

`OptionManager` だけ `GetInstance()`、他は `Instance()`。

## 作り直し方

```
python gen_excalidraw.py
```

`.excalidraw` がこのフォルダに、`.svg` が `Preview/` に出る。
ノードと矢印の定義はスクリプト下部の `s1` 〜 `s7` にべた書きしてあるので、
クラスを足したらそこへ追記する。
右下の改善点は同じくスクリプト下部の `sX.imp(...)` にまとめてある。

緑矢印の元データは次のコマンドで洗い出せる。

```
rg -o "(MainEngine|SceneManager|ResourceManager|AssetDatabase|InputManager|AudioManager|MainEditor|D3D12Wrapper|DescriptorHeapManager|RayEngine|GameManager|ObjectMetaRegistry|OptionManager)::(Instance|GetInstance)\(\)" Source
```

## メモ

- `D3D12Wrapper` ⇄ `DescriptorHeapManager`、`OptionManager` ⇄ `MainEngine` / `InputManager` / `AudioManager` は相互に直接引き合っている。
- `EngineServices` が9個のシングルトンをまとめて配っているので、System / GameObject 側は `Instance()` を呼んでいない(03 が該当)。
- 逆に直接引きが多いのは `BaseScene`(9個)、`GraphicsEngine`(8個)、`SceneViewPanel`(7個)、`EffectEditor`(7個)とリソースの IO 群。
- パスは30種類以上あるが、02 に載せたのはシングルトンを直接引いているものだけ。
