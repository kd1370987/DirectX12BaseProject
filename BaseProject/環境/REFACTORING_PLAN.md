# リファクタリング手順

依存関係図（`01_Overview` 〜 `07_Input_Audio_Option`）で見えた汚さを、
どの順番で直していくかの計画。

---

## 0. 先にゴールを決める

**シングルトンをゼロにするのは目的ではない。**

`D3D12Wrapper` や `DescriptorHeapManager` のように「アプリに1つしか無く、
生存期間がアプリと同じ」ものは、シングルトンのままでも設計として説明がつく。
面接で強いのは「全部消しました」ではなく「ここは残す判断をしました、理由は〜」の方。

消すべきものは3つに絞る。

| # | 直すもの | なぜ |
|---|---|---|
| A | **層の逆流** | エンジンがエディターを見る / 描画層がゲームを見る。依存の向きが説明できなくなる |
| B | **循環** | `D3D12Wrapper` ⇄ `DescriptorHeapManager` など。初期化順と解放順が暗黙になる |
| C | **どこからでも書ける状態** | 「誰がこれを触るのか」がコードから読めない。バグの原因が追えない |

逆に、**残してよいもの**は最初に決めておく（後で迷わないため）。

- `D3D12Wrapper` / `DescriptorHeapManager` … デバイスとディスクリプタヒープはプロセスに1つ
- `MainEngine` … アプリの入口そのもの
- `MainEditor` … ツール層。ただし**エンジン側から見られる**のは無し（＝A）

---

## 1. 現状（ベースライン）

`Instance()` / `GetInstance()` の呼び出し **629回 / 142ファイル**。

| シングルトン | 呼び出し | ファイル |
|---|---:|---:|
| ResourceManager | 158 | 68 |
| AssetDatabase | 95 | 41 |
| D3D12Wrapper | 68 | 20 |
| MainEditor | 64 | 21 |
| MainEngine | 62 | 24 |
| DescriptorHeapManager | 62 | 23 |
| SceneManager | 35 | 20 |
| OptionManager | 30 | 13 |
| AudioManager | 15 | 11 |
| InputManager | 13 | 8 |
| GameManager | 12 | 7 |
| RayEngine | 11 | 4 |
| ObjectMetaRegistry | 4 | 3 |

層をまたぐ向き：

```
Engine  -> Engine   378      Engine -> Editor   38   ← A(逆流)
Editor  -> Engine   122      App    -> Editor   21   ← A(逆流)
App     -> Engine    53      Engine -> Game      1   ← A(逆流)
```

---

## 2. 進め方の原則

1. **1フェーズ = 1ブランチ = 1テーマ。** 途中で別のことを直したくなっても、メモに残して手は出さない。
2. **フェーズの終わりで必ず起動確認。** 起動 → シーン読み込み → ゲームモード往復 → エディター操作。自動テストが無いので、この手順書だけは最初に作る。
3. **ビルドが何日も通らない状態を作らない。** 特にフェーズ4は、アセット種別ごとに区切る。
4. **数を測りながら進める。** 各フェーズの前後で下のコマンドを叩き、README に記録する。

```bash
rg -o "(MainEngine|SceneManager|ResourceManager|AssetDatabase|InputManager|AudioManager|MainEditor|D3D12Wrapper|DescriptorHeapManager|RayEngine|GameManager|ObjectMetaRegistry|OptionManager)::(Instance|GetInstance)\(\)" Source | wc -l
```

---

## 3. 順番

| 順 | フェーズ | 直すもの | 効果（目安） | 危険度 | 規模 |
|---|---|---|---|---|---|
| 1 | 横断的関心事を切る | A | −40回・逆流59本のほぼ全部 | **低** | 半日〜1日 |
| 2 | 循環を切る | B | −20〜30回・循環1組 | **低** | 半日 |
| 3 | PassContext を太らせる | C | −30〜60回 | 中 | 2〜3日 |
| 4 | ResourceBuildContext の徹底 | C | −100〜150回 | 中 | 4〜7日 |
| 5 | 所有関係の整理 | C | −30回 | 低 | 1〜2日 |
| 6 | 図とドキュメントの作り直し | — | 見せ物 | 低 | 半日 |

**なぜこの順番か。**

- フェーズ1は「呼び出しの置き換えだけ」でロジックが1行も動かない。効果は最大級（逆流がほぼ全部消える）なのに危険が最小。**最初にやると図が目に見えて変わるので、続ける気力が持つ。**
- フェーズ4は呼び出し数では最大だが、**そこから始めてはいけない。** 機械的な置換が何日も続き、その間ビルドが不安定で、しかも図の見た目は「緑矢印が減る」だけ。仕組み（Context の形）が固まってから流し込む。
- フェーズ3を4より先にするのは、レンダーグラフがこのプロジェクトの見せ場だから。時間切れになったとき、手が入っているのがそこである方がよい。

---

## 4. 各フェーズの中身

### フェーズ1 — 横断的関心事（ログ・計測・デバッグ描画）を切る

**いちばん効く。しかも仕組みは既にある。**

`Source/Engine/Utility/Debug/DebugLog.h` に `Engine::Debug::SetLogCallback` があり、
エディターがコールバックを登録する形が既にできている。
にもかかわらず、14箇所が `MainEditor::Instance().AddLog / ErrorLog` を直接呼んでいる。
**自分で作った仕組みを使い切れていない**だけなので、置き換えるだけで終わる。

同じ形を、計測とデバッグ描画にも広げる。

| 用途 | 今 | 後 | 箇所 |
|---|---|---|---|
| ログ | `MainEditor::Instance().AddLog / ErrorLog` | `ENGINE_LOG` / `ENGINE_ERROR` | 約14 |
| 計測 | `MainEditor::Instance().StartTimer / StopTimer` | `Engine::Debug::ScopedTimer`（RAII） | 約22 |
| デバッグ描画 | `MainEditor::Instance().DrawBox / GetDebugLineDataVec` | `Engine::Debug::DrawBox` + コールバック | 約5 |

手順：

1. `Engine::Debug` に計測用のコールバックと `ScopedTimer` を足す。デバッグ描画も同じ形で足す。
2. `MainEditor::Init` で3つとも登録する（今のログと同じやり方）。
3. 呼び出し側を置き換える。`Archive` / `PipelineState` / `World.h` / `ComponentMetaRegistry` /
   `AssetDatabase` / `AnimatorAsset` / `ActionStateMachineAsset` / `CollisionWorld` /
   `BVHTraverser` / `RenderContext` / `GraphicEngine` / `App.cpp` / `GameManager`。
4. **ゲームモードでは no-op に落とす。** 毎フレームのログが重いのは既に踏んだ問題。
   `ScopedTimer` も同様に、計測しないビルドでは何も残らない形にする。

これで `Engine -> Editor` 38本と `App -> Editor` 21本のほとんどが消え、
**エンジンがエディターを知らない状態**になる。図05の「逆向き依存」が丸ごと片付く。

> 面接で使える言い方：「ログ・計測・デバッグ描画は層をまたぐので、
> インターフェースと登録の形にしてエンジンからツールへの依存を切りました」

### フェーズ2 — 循環を切る

`DescriptorHeapManager` が `D3D12Wrapper` を引いているのは **Device が欲しいだけ**。
`D3D12Wrapper::Instance().GetDevice()` は全体で27箇所あり、`D3D12Wrapper` の呼び出しの4割を占める。

1. `DescriptorHeapManager::Init(Device*)` にして Device を保持する。→ 循環が切れる。
2. 続けて `GetDevice()` 27箇所のうち、**生成時に受け取れるもの**を潰す。
   `ResourceBuildContext` には既に `pDevice` があるので、そこを通っている生成処理は
   Context から取るだけ。フェーズ4の下準備にもなる。

図06の相互矢印が1組消える。**循環が無くなったことは図で一目で分かる**ので、費用対効果が高い。

### フェーズ3 — PassContext を太らせる

30種以上あるパスが、それぞれ `ResourceManager` / `AssetDatabase` / `RayEngine` /
`MainEngine` を個別に引いている。`PassContext` は既に
`RenderGraph` / `GraphicsEngine` / `RenderContext` を配っているので、**そこに足すだけ**。

1. `PassContext` に足す：`pResourceManager` / `pAssetDatabase` / `pRayEngine`、
   および `MainEngine` 経由で取っていた `pParticleBufferManager` / `pPipelineStateManager`。
2. `GraphicsEngine` が Context を組み立てる位置で1回だけ詰める。
3. 各パスの `Instance()` を Context 参照に置換。パスは自己完結しているので1つずつ潰せる。

**同時にやる：`GraphicsEngine -> SceneManager / GameManager` の逆流を切る。**
描画層がシーンとゲームを見に行っているのはここだけ（`Engine -> Game` の1本がこれ）。
必要なデータはシーン更新側から積む形に変える。

### フェーズ4 — ResourceBuildContext の徹底（本丸）

`ResourceManager` 158 + `AssetDatabase` 95 = **253回、全体の40%**。
方針も構造体も既にあり、27ファイル・89箇所で使われている。**あとは残りを流し込むだけ。**

種別ごとに区切って進める。1種類 = 1コミット。

```
1. Model  (ModelIO / ModelConverter / ModelProcessor / Model)   ← いちばん依存が多い。最初にやると型が決まる
2. Texture(TextureIO / TextureImporter / TextureCreater / Texture)
3. Mesh / Material
4. Shader (ShaderIO / DXCCompiler)
5. Animator / ActionStateMachine / Particles / EffectAsset / AudioBehavior
6. Sound / Font / ShadingModelTable / Prefab / RenderingPipelineAsset
```

途中で気付くはずの2点：

- **Context に足りないものがある。** `Material` は `OptionManager`、`Mesh` は遅延解放のために
  `MainEngine` を引いている。足りない分は Context に足す（遅延解放は
  「解放を後回しにする」インターフェースを1つ切って渡す形が素直）。
- **`ResourceManager.h` が自分自身の `Instance()` をテンプレート内で12箇所呼んでいる。**
  ここを直さないと、呼び出し側が「どの ResourceManager か」を選べないままになる。
  種別の移行が一巡してから最後に手を付ける。

### フェーズ5 — 所有関係の整理

残りの掃除。ここまで来ると1件ずつが小さい。

- **所有されている側が親を引いている。** `ParticleBufferManager` / `GPUParticlePool` /
  `MouseCursor` → `MainEngine`。所有者が初期化時に必要なものを渡す。
- **`BaseScene` が9個を直引き。** `EngineServices` を組み立てているのは `BaseScene` 自身なので、
  作ったあとは自分もそれを経由する。
- **`OptionManager` の押し込みをやめる。** `WindowOption → MainEngine` /
  `AudioOption → AudioManager` / `InputOption → InputManager` を、
  受け手が起動時と変更通知で読む形にして片方向にする。
- **アプリ側の直引き。** `GunShootSystem → ResourceManager` / `ScoreSystem → GameManager` /
  `CameraStartSystem → OptionManager` / 各 `Sequence → SceneManager`。
  `SystemContext` / `ObjectContext` に経路があるので、そちらへ寄せる。
- **コンポーネントのヘルパーがヘッダーで `Instance()` を呼んでいる。**
  `ModelComponent` / `ParticlesComponent` / `SoundComponent` など。
  コンポーネントは POD なので、取得と返却はシステム側か `ComponentTraits::Release` 経由へ。

### フェーズ6 — 図とドキュメントの作り直し

`gen_excalidraw.py` を回して before / after を並べる。
数値（629 → N）と、各フェーズで何を判断したかを README に書く。

---

## 5. 時間が足りなくなったら

上から順に落とす。

- **フェーズ4を種別ごとに途中で止める** … Model と Texture だけでも「方針を通した」と言える。
  中途半端に見えないよう、やった種別とやっていない種別を README に明記しておく。
- **フェーズ5を丸ごと落とす** … 図の見た目への影響が小さい。
- **フェーズ1〜3だけは通す。** ここまでで、逆流と循環という「説明できない依存」が消える。
  残った緑矢印は「アプリ寿命のサービスを引いている」だけになり、**理由が説明できる状態**になる。

**逆にやってはいけないこと**：締め切り直前にフェーズ4を始めること。
動いていたものが動かなくなるリスクが、得られる見栄えに見合わない。

---

## 6. 就活での見せ方

リファクタリングそのものより、**判断を説明できること**が効く。
この `環境/` フォルダをそのまま提出物にできる形にしておく。

用意するもの：

1. **before / after の図**（同じレイアウトで並べる。緑矢印が減っているのが一目で分かる）
2. **数値**（629 → N、逆流59本 → 0本、循環1組 → 0組）
3. **残したシングルトンとその理由**（0章の表。ここがいちばん聞かれる）
4. **各フェーズで何を考えたか**（特にフェーズ1の「自分で作った仕組みを使い切れていなかった」は
   自己批判として素直で、印象がいい）

聞かれたときに答えられるようにしておくこと：

- 「なぜシングルトンを使ったのか」→ 当時の理由と、今どう思っているか
- 「なぜ全部消さなかったのか」→ 0章の判断基準
- 「どうやって安全に直したのか」→ 段階の切り方、起動確認の手順、測り方
