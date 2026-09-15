# 依存関係図

BaseProject のクラス依存を Excalidraw 形式で書き出したもの。
`.excalidraw` を [excalidraw.com](https://excalidraw.com) にドラッグ&ドロップすると開ける。

- このフォルダ直下 … **現在(2026-09-15 時点)** の図
- `Before/` … リファクタリング前(2026-09-10 時点)の図。レイアウトの考え方は同じなので並べて見比べられる

## 図の一覧

| ファイル | 中身 |
|---|---|
| `01_Overview.excalidraw` | Application → 10個のシングルトン → MainEngine が抱える実体(ResourceManager / GraphicsEngine ほか) |
| `02_Graphics.excalidraw` | GraphicsEngine の持ち物 → CameraPipelineData → GraphicsPipeline → RenderGraph → Pass、PassContext の配り方 |
| `03_Scene_ECS_GameObject.excalidraw` | SceneManager → BaseScene → World / GameObjectManager、EngineServices の配り方、アプリ側に残る直引き |
| `04_Resource.excalidraw` | ResourceManager(MainEngine 所有)/ AssetDatabase / ResourceBuildContext と各アセットの IO |
| `05_Editor.excalidraw` | MainEditor → ImGuiContext / PanelManager / Profiler と各パネル、エディターへの逆向き依存 |
| `06_D3D12_Raytracing.excalidraw` | GraphicsEngine が持つ D3D12 まわり(GraphicsDevice / DescriptorHeapManager ほか)と RayEngine 配下 |
| `07_Input_Audio_Option.excalidraw` | InputManager / AudioManager / OptionManager / Particle / JobSystem |

`Preview/*.svg` は同じ配置の確認用。ブラウザや VS でそのまま開ける。

図から出した改善点を、どの順番で直すかは [REFACTORING_PLAN.md](REFACTORING_PLAN.md) にまとめてある(進み具合もそこに書いている)。

## 表記

| 見た目 | 意味 |
|---|---|
| 緑の枠 | シングルトン(`static X& Instance()` を持つクラス) |
| 白の枠 | 通常のクラス |
| 白の破線枠 | 引数で配るコンテキスト構造体(EngineServices / SystemContext / ObjectContext / PassContext / ResourceBuildContext / EditorContext) |
| 緑の矢印 | シングルトン経由の依存。`X::Instance()` を直接呼んでいる |
| 白の実線矢印 | 所有(`unique_ptr` / 値メンバ) |
| 白の破線矢印 | 参照(引数・ポインタで受け取る) |
| 白の点線矢印 | 継承 |
| 橙の枠(右下) | 改善点。そのシートで気になった依存と、直し方の当たり |

色は Excalidraw の**ライトモードの値**(黒/緑)で保存している。
Excalidraw のダークモードは描画時に色を反転するので、ダークテーマで開くと
「黒背景・白枠・緑枠」に見える。

## シングルトン(13個 → 10個)

`MainEngine` / `SceneManager` / `GameManager` / `MainEditor` / `ResourceManager` /
`InputManager` / `AudioManager` / `OptionManager` / `RayEngine` / `ObjectMetaRegistry`

`OptionManager` だけ `GetInstance()`、他は `Instance()`。

外れたもの:

| クラス | 今の持ち主 |
|---|---|
| `D3D12Wrapper` | 削除。`GraphicsDevice` / `CommandContext` / `FrameManager` / `BackBuffer` などに分けて `GraphicsEngine` が所有 |
| `DescriptorHeapManager` | `GraphicsEngine` が `unique_ptr` で所有。Device は `Init` の引数で受け取る |
| `AssetDatabase` | `ResourceManager` が `unique_ptr` で所有 |

`ResourceManager` は `MainEngine` の `unique_ptr` になったが、`Instance()` は残している。
`ResourceRef<T>` が値として資産に埋まり、コピー・破棄のたびに参照カウントを触るので引数で渡せないため。
図では緑枠のまま描き、ラベルにその旨を書いた。

## 数値(before → after)

`Instance()` / `GetInstance()` の呼び出し(下のコマンドの件数。コメント内の4件を含む)。

| シングルトン | 2026-09-10 | 2026-09-15 |
|---|---:|---:|
| ResourceManager | 158 | 12(すべて `ResourceRef<T>` の中) |
| AssetDatabase | 95 | 0(シングルトンではなくなった) |
| D3D12Wrapper | 68 | 0(削除) |
| MainEditor | 64 | 17 |
| MainEngine | 62 | 64 |
| DescriptorHeapManager | 62 | 0(シングルトンではなくなった) |
| SceneManager | 35 | 35 |
| OptionManager | 30 | 28 |
| AudioManager | 15 | 15 |
| InputManager | 13 | 13 |
| GameManager | 12 | 12 |
| RayEngine | 11 | 5 |
| ObjectMetaRegistry | 4 | 4 |
| **合計** | **629 回 / 142 ファイル** | **205 回 / 62 ファイル** |

層をまたぐ向き(呼び出し元のフォルダ → 呼び出すシングルトンの層):

| 向き | 2026-09-10 | 2026-09-15 | |
|---|---:|---:|---|
| Engine → Editor | 38 | 11 | 逆流。MainEngine の Init/Update/Draw/Release と SceneManager の通知だけ残る |
| App → Editor | 21 | 4 | 逆流。App.cpp / RegisterEditFunc |
| Engine → Game | 1 | 1 | 逆流。GraphicsEngine → GameManager::Draw() |
| 循環 | 1組 | 0組(D3D12層) | OptionManager ⇄ 各マネージャーの押し込みは残る |

## 作り直し方

```
python gen_excalidraw.py
```

`.excalidraw` がこのフォルダに、`.svg` が `Preview/` に出る(`Before/` は触らない)。
Windows のコンソールで文字化けする場合は `PYTHONUTF8=1` を付ける。
ノードと矢印の定義はスクリプト下部の `s1` 〜 `s7` にべた書きしてあるので、
クラスを足したらそこへ追記する。
右下の改善点は同じくスクリプト下部の `sX.imp(...)` にまとめてある。

緑矢印の元データは次のコマンドで洗い出せる(`Source` フォルダで実行)。

```
rg -o "(MainEngine|SceneManager|ResourceManager|AssetDatabase|InputManager|AudioManager|MainEditor|D3D12Wrapper|DescriptorHeapManager|RayEngine|GameManager|ObjectMetaRegistry|OptionManager)::(Instance|GetInstance)\(\)" .
```

ファイルごと・クラスごとに数えるなら:

```
rg -o "(MainEngine|SceneManager|ResourceManager|AssetDatabase|InputManager|AudioManager|MainEditor|D3D12Wrapper|DescriptorHeapManager|RayEngine|GameManager|ObjectMetaRegistry|OptionManager)::(Instance|GetInstance)\(\)" . | sed -E 's/::(Instance|GetInstance)\(\)//' | sort | uniq -c
```

## メモ

- `D3D12Wrapper` ⇄ `DescriptorHeapManager` の相互参照は消えた。D3D12 層は `Device*` / `DescriptorHeapManager*` を引数で受け取り、上の層を見ない。
- 相互に引き合っているのは `OptionManager` ⇄ `MainEngine` / `InputManager` / `AudioManager` だけになった(WindowOption / AudioOption / InputOption の押し込み)。
- `EngineServices` は `MainEngine` が正本を持ち、`World` が写しを持つ。中身は MainEngine / ResourceManager / AssetDatabase / InputManager / RayEngine / AudioManager / JobSystem / OptionManager / DebugDraw の9つ(MainEditor は外した)。
- レンダーパス(30種類)の `Instance()` は 0。`PassContext` を組むのは `RenderGraph::MakeContext` だけ。
- 直接引きが多いのは `SceneViewPanel`(17回)、`MainEngine.cpp`(28回・起動と終了の駆動)、`EffectEditor`(9回)、`GraphicsEngine`(9回)。
- `MainEngine::Instance()` は減っていない。`D3D12Wrapper` を引いていた箇所の一部が `MainEngine::Instance().RefGraphicsEngine()` に付け替わったため。
