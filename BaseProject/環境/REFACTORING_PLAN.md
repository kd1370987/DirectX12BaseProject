# リファクタリング手順

依存関係図（`01_Overview` 〜 `07_Input_Audio_Option`）で見えた汚さを、
どの順番で直していくかの計画と、その進み具合。

- 計画を立てた時点の図 … `Before/`（2026-09-10）
- 今の図 … このフォルダ直下（2026-09-15）

---

## 0. 先にゴールを決める

**シングルトンをゼロにするのは目的ではない。**

「アプリに1つしか無く、生存期間がアプリと同じ」ものは、
シングルトンのままでも設計として説明がつく。
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

> **実際にやってみての変更（2026-09-15）**
> `D3D12Wrapper` / `DescriptorHeapManager` は「残してよい」側に置いていたが、
> フェーズ2で Device を引数にした結果、持ち主（`GraphicsEngine`）に入れる方が素直だと分かり、両方ともシングルトンを外した。
> 「残す判断」の例としては、代わりに `ResourceManager::Instance()` を残している
> （`ResourceRef<T>` が値として資産に埋まり、引数で渡せないため）。

---

## 1. ベースラインと現在

`Instance()` / `GetInstance()` の呼び出し。

| | 2026-09-10 | 2026-09-15 |
|---|---:|---:|
| 呼び出し回数 | 629 | **205** |
| ファイル数 | 142 | **62** |
| シングルトンの数 | 13 | **10** |

| シングルトン | 2026-09-10 | 2026-09-15 | メモ |
|---|---:|---:|---|
| ResourceManager | 158 | 12 | 残りは全部 `ResourceRef<T>` の中。実体は MainEngine 所有 |
| AssetDatabase | 95 | 0 | ResourceManager の持ち物になった |
| D3D12Wrapper | 68 | 0 | 削除 |
| MainEditor | 64 | 17 | |
| MainEngine | 62 | 64 | **増えた**。付け替え先になっている（下の「残っていること」参照） |
| DescriptorHeapManager | 62 | 0 | GraphicsEngine の持ち物になった |
| SceneManager | 35 | 35 | 手付かず |
| OptionManager | 30 | 28 | |
| AudioManager | 15 | 15 | |
| InputManager | 13 | 13 | |
| GameManager | 12 | 12 | |
| RayEngine | 11 | 5 | |
| ObjectMetaRegistry | 4 | 4 | |

層をまたぐ向き：

```
                 2026-09-10   2026-09-15
Engine -> Editor     38           11     ← A(逆流)  MainEngine の駆動 8 / SceneManager の通知 3
App    -> Editor     21            4     ← A(逆流)  App.cpp 2 / RegisterEditFunc 2
Engine -> Game        1            1     ← A(逆流)  GraphicsEngine -> GameManager::Draw()
循環(D3D12層)        1組          0組    ← B
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

## 3. 順番と進み具合

| 順 | フェーズ | 直すもの | 効果（目安） | 状態 |
|---|---|---|---|---|
| 1 | 横断的関心事を切る | A | −40回・逆流59本のほぼ全部 | **完了** |
| 2 | 循環を切る | B | −20〜30回・循環1組 | **完了**（計画より踏み込んで D3D12Wrapper を削除） |
| 3 | PassContext を太らせる | C | −30〜60回 | **ほぼ完了**（描画層→シーン/ゲームの逆流が残る） |
| 4 | ResourceBuildContext の徹底 | C | −100〜150回 | **ほぼ完了**（Sound / Mesh / Prefab が残る） |
| 5 | 所有関係の整理 | C | −30回 | **途中**（ResourceManager / AssetDatabase の所有は済み） |
| 6 | 図とドキュメントの作り直し | — | 見せ物 | **済み**（2026-09-15。この更新） |

**なぜこの順番か。**（計画時の考え。結果的にこの順で問題なかった）

- フェーズ1は「呼び出しの置き換えだけ」でロジックが1行も動かない。効果は最大級（逆流がほぼ全部消える）なのに危険が最小。**最初にやると図が目に見えて変わるので、続ける気力が持つ。**
- フェーズ4は呼び出し数では最大だが、**そこから始めてはいけない。** 機械的な置換が何日も続き、その間ビルドが不安定で、しかも図の見た目は「緑矢印が減る」だけ。仕組み（Context の形）が固まってから流し込む。
- フェーズ3を4より先にするのは、レンダーグラフがこのプロジェクトの見せ場だから。時間切れになったとき、手が入っているのがそこである方がよい。

---

## 4. 各フェーズの中身

### フェーズ1 — 横断的関心事（ログ・計測・デバッグ描画）を切る　【完了】

**計画**

`Engine::Debug::SetLogCallback` があるのに、14箇所が `MainEditor::Instance().AddLog / ErrorLog` を直接呼んでいた。
**自分で作った仕組みを使い切れていない**だけなので、置き換えるだけで終わる。同じ形を計測とデバッグ描画にも広げる。

| 用途 | 前 | 後 |
|---|---|---|
| ログ | `MainEditor::Instance().AddLog / ErrorLog` | `ENGINE_LOG` / `ENGINE_WARNING` / `ENGINE_ERROR` / `ENGINE_ERRLOG` |
| 計測 | `MainEditor::Instance().StartTimer / StopTimer` | `ENGINE_PROFILE_SCOPE`（RAII。結果は `SetProfileCallback` で Profiler へ） |
| デバッグ描画 | `MainEditor::Instance().DrawBox / GetDebugLineDataVec` | `Engine::Graphics::DebugDraw`（GraphicsEngine 所有。EngineServices で配る） |

**結果**

- `MainEditor` の `AddLog` / `StartTimer` / `Draw*` 系は削除。`MainEditor::Init` がログと計測のコールバックを登録し、`Release` で外す。
- `EngineServices` から `MainEditor` を外し、代わりに `DebugDraw` を載せた。表示のオンオフは `DebugDrawOption`。
- `Engine -> Editor` 38 → 11、`App -> Editor` 21 → 4。
  ECS（World / ComponentMetaRegistry）・CollisionWorld・BVHTraverser・Archive・PipelineState・AssetDatabase からエディターへの依存は 0。
- 残り（エディターを「駆動」「通知」しているもの）は **フェーズ5の残り** に回した。

> 面接で使える言い方：「ログ・計測・デバッグ描画は層をまたぐので、
> インターフェースと登録の形にしてエンジンからツールへの依存を切りました」

### フェーズ2 — 循環を切る　【完了】

**計画**

`DescriptorHeapManager` が `D3D12Wrapper` を引いているのは **Device が欲しいだけ**。
`DescriptorHeapManager::Init(Device*)` にすれば循環が切れる。

**結果（計画より踏み込んだ）**

- `DescriptorHeapManager` は Device を `Init` で受け取り、`GraphicsEngine` が `unique_ptr` で持つ形にした。
- Device を引数で配れるようになったら `D3D12Wrapper` に残る役目が無くなったので、
  `GraphicsDevice` / `BackBuffer` / `CommandContext`（+ `CommandPool`）/ `FrameManager` / `AsyncGPUManager` に分けて **`GraphicsEngine` へ集約し、`D3D12Wrapper` を削除**した。
- D3D12 層（GPUResource / Texture / バッファ群 / RootSignatureBuilder / CBAllocator）は `Device*` と `DescriptorHeapManager*` を引数で受け取り、上の層を見ない。
- `D3D12Wrapper` 68回 + `DescriptorHeapManager` 62回 → 0。図06の相互矢印が消えた。

**気付いたこと**

- シングルトンを外した分の一部は `MainEngine::Instance().RefGraphicsEngine()` に付け替わっただけで、
  `MainEngine` の呼び出しは 62 → 64 と減っていない（BLAS / Mesh / ScopedResourceBuild / ImGuiContext / EditorHelper / SceneViewPanel）。
  **置き場が MainEngine に移っただけ**の箇所は、フェーズ5で引数に直す。

### フェーズ3 — PassContext を太らせる　【ほぼ完了】

**計画**

30種あるパスが `ResourceManager` / `AssetDatabase` / `RayEngine` / `MainEngine` を個別に引いていた。
`PassContext` に足して、組み立て位置で1回だけ詰める。

**結果**

- `PassContext` に `pMainEngine` / `pResourceManager` / `pAssetDatabase` / `pRayEngine` / `pParticleManager` / `pHeapManager` を追加。
- 組むのは `RenderGraph::MakeContext` だけ。**パス（30種類）の `Instance()` は 0。**
- `RenderContext` / `ParticleBufferManager` / `MouseCursor` / `RayEngine::CommitWorld` もヒープ・リソースマネージャーを引数で受け取る形になった。

**残っていること**

- `RenderGraph::MakeContext` 自身が `MainEngine` と `RayEngine` を `Instance()` で引いて詰めている。
  GraphicsEngine から受け取れば、描画層でシングルトンを引くのは GraphicsEngine の入口だけになる。
- **`GraphicsEngine -> SceneManager / GameManager` の逆流が残っている**（計画では同時にやる予定だった）。
  - `GameManager::Draw()`（「テスト」とコメントされた呼び出し）
  - `SceneManager::RefWorld()`（BLAS 初期化キューをワールドのリソースから読む）
  必要なデータはシーン更新側から積む形に変える。
- `ParticleSimulation` / `GPUParticlePool` が `MainEngine` を引く（パーティクルマネージャー・デルタタイム・遅延解放）。
- `GraphicsEngine` / `MouseCursor` / `DebugDraw` が `OptionManager` を直接読む。

### フェーズ4 — ResourceBuildContext の徹底（本丸）　【ほぼ完了】

**計画**

`ResourceManager` 158 + `AssetDatabase` 95 = 253回、全体の40%。種別ごとに区切って Context へ流し込む。

**結果**

- `ResourceManager` 158 → 12、`AssetDatabase` 95 → 0。IO 群と Model / Texture / Material / Shader / Animator / ActionStateMachine / Particles / EffectAsset / AudioBehavior / Font / RenderingPipelineAsset の直引きは 0。
- `ResourceBuildContext` に `pDevice` / `pHeapManager` / コマンドリスト3種 / `pGraphicsEngine` / `pMeshBufferAllocator` / `pPassMetaRegistry` / `pResourceManager` / `pAssetDatabase` が揃った。
- 計画で「最後に手を付ける」としていた **`ResourceManager.h` の自己 `Instance()` 12箇所**は、
  すべて `ResourceRef<T>` のコピー・破棄の中だった。値として資産に埋まるので引数で渡せない、という理由で **残す判断**をした。
  それに合わせて `Instance()` は「MainEngine が作った実体を指す入口」に変え、自分では作らないようにした。
- `ShadingModelTable` はシェーディングモデルごと削除したので対象から外れた。

**残っていること**

- `Sound` / `SoundIO` → `AudioManager`（読み込みにオーディオエンジンが要る）。Context に載せる。
- `ScopedResourceBuild` → `MainEngine`（バッチを開くのに GraphicsEngine が要る）。引数で受け取る。
- `Mesh::Release` → `MainEngine`（MeshBufferAllocator へ返す。Release には Context が来ない）。
- `Prefab` → `SceneManager`（生成先のワールドを決める）。引数で World を受け取る。

### フェーズ5 — 所有関係の整理　【途中】

**済んだこと**

- **`ResourceManager` の持ち主を `MainEngine` にし、`AssetDatabase` は `ResourceManager` に持たせた。**
- **`BaseScene` の直引き 9個 → 2個。** 残りはワールドの作成（`SceneManager::CreateWorld`）と `EngineServices` の写し。
- `ParticleBufferManager` / `AudioManager` / `MouseCursor` は持ち物を `Init` の引数で受け取る形になった。
- アプリ側：`GunShootSystem → ResourceManager` と、`ModelComponent` / `ParticlesComponent` のヘッダー内 `Instance()` は解消。
- エディター：`EditorContext` に `pServices` / `pProfiler` / `pEditorCamera` を載せ、パネルの多くはそこから引くようになった。

**残っていること（次にやる順）**

1. **エディターへの逆流の残り**（A）
   - `SceneManager → MainEditor::OnSceneChanged / RefEffectEditor` … 通知なのでコールバック登録の形にする。
   - `GameManager` / `InputActionManager → MainEditor::RegisterEditFunc`、`App.cpp → EndProfileFrame / IsModalActive`。
   - `MainEngine → MainEditor`（Init / Update / Draw / Release）は入口の駆動として許容。
2. **描画層の逆流**（A、フェーズ3の残り）… `GraphicsEngine → GameManager / SceneManager`。
3. **`OptionManager` の押し込みをやめる**（B）… `WindowOption → MainEngine` /
   `AudioOption → AudioManager` / `InputOption → InputManager` を、受け手が起動時と変更通知で読む形にして片方向にする。
4. **`MainEngine::Instance().RefGraphicsEngine()` の付け替え組**（C）… BLAS / Mesh / ScopedResourceBuild / ImGuiContext / EditorHelper / SceneViewPanel。
   `InputManager → MainEngine`（モードとウィンドウ）、`GPUParticlePool → MainEngine`（遅延解放）も同じ扱い。
5. **アプリ側の直引き**（C）… `Sequence 群 → SceneManager / GameManager` / `ScoreSystem・ScoreHUD → GameManager` /
   `CameraStartSystem → OptionManager`。`EngineServices` に `SceneManager` が無いのが消えない理由なので、載せるか細い口を足すかを先に決める。
6. **コンポーネントのヘッダー内 `Instance()`**（C）… `SoundComponent` / `HitSoundComponent` / `FlyingSoundResource → AudioManager`、
   `FollowTargetComponent` / `AttachmentSlotsComponent → SceneManager`。
7. **`RayEngine` を GraphicsEngine の持ち物にする**（C）… 持ち主が居ないので MainEngine と RenderGraph が引いている。
8. **エディターパネルの残り**（C）… `SceneViewPanel`（17回）と `EffectEditor`（9回）。EditorContext に SceneManager / GraphicsEngine への経路を足す。

**ついでに見つけたもの（コードには手を入れていない）**

- `MainEngine.h` に `m_upRenderContextVec` が宣言だけ残っている（実体は GraphicsEngine 側で使っている）。
- `FrameResourceManager` はどこからも使われておらず、`.cpp` も空。

### フェーズ6 — 図とドキュメントの作り直し　【済み】

- `gen_excalidraw.py` の `s1`〜`s7` を 2026-09-15 のコードに合わせて書き直し、図を再生成した。
- 計画時点の図は `Before/` に残した（同じレイアウトの考え方なので並べて見比べられる）。
- 数値（629 → 205、逆流 38+21 → 11+4、循環1組 → 0組）は README にも載せた。
- フェーズ5が進んだら、もう一度 `python gen_excalidraw.py` で作り直す。

---

## 5. 時間が足りなくなったら

上から順に落とす。

- **フェーズ5の 5〜8 を落とす** … アプリ側・パネル側の直引きは「アプリ寿命のサービスを引いている」だけで、説明がつく。
- **フェーズ5の 3〜4 を落とす** … 循環（Option の押し込み）は残るが、初期化順は README に書いておけば説明できる。
- **フェーズ5の 1〜2（逆流の残り）だけは通したい。** これで「エンジンがエディターとゲームを知らない」と言い切れる。

**逆にやってはいけないこと**：締め切り直前に大きな置換（SceneManager の経路追加など）を始めること。
動いていたものが動かなくなるリスクが、得られる見栄えに見合わない。

---

## 6. 就活での見せ方

リファクタリングそのものより、**判断を説明できること**が効く。
この `環境/` フォルダをそのまま提出物にできる形にしておく。

用意するもの：

1. **before / after の図**（`Before/` と直下。緑矢印が減っているのが一目で分かる）… 済み
2. **数値**（629 → 205、ファイル 142 → 62、シングルトン 13 → 10、Engine→Editor 38 → 11、循環1組 → 0組）… 済み
3. **残したシングルトンとその理由**（0章。`ResourceManager::Instance()` を `ResourceRef<T>` のために残した話が具体例になる）
4. **各フェーズで何を考えたか**
   - フェーズ1の「自分で作った仕組みを使い切れていなかった」は自己批判として素直で、印象がいい
   - フェーズ2で「残す予定だったものを、やってみて外した」判断の変化
   - `MainEngine` の呼び出しが減らなかった理由（置き場が移っただけの箇所がある）を自分で指摘できること

聞かれたときに答えられるようにしておくこと：

- 「なぜシングルトンを使ったのか」→ 当時の理由と、今どう思っているか
- 「なぜ全部消さなかったのか」→ 0章の判断基準と `ResourceRef<T>` の例
- 「どうやって安全に直したのか」→ 段階の切り方、起動確認の手順、測り方
