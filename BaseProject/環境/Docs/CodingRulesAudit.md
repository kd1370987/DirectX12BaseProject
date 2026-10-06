# Coding Rules 適合性監査(2026-10-06)

基準 : `環境/Docs/CodingStandards.md`(依頼文中の「CodingRules.md」はこのファイル。リポジトリに `CodingRules.md` という名前のファイルは無い)

対象 : `BaseProject/Source` 以下の `.h / .cpp / .inl`(1023 ファイル・約 12.7 万行)。外部ライブラリは `Library/` にあり対象外。

ベースライン : `Debug|x64` フルリビルド **成功 / エラー 0 / 警告 2156(重複除去 420)**

---

## 0. 監査の進め方

* 機械的に判定できるもの(global namespace の定義・namespace スコープ変数・`shared_ptr`・`assert`・RTTI・`enum`・定数名・メンバ prefix・アクセサの戻り値)は、
  コメント/文字列/プリプロセッサを除去してブレース深度と namespace を追跡するスクリプトで全数を出し、誤検出を目で除いた。
* 設計に関わるもの(依存方向・所有権・ECS・RenderGraph・PCH)はコードを読んで判断した。
* 既存の設計判断が `環境/REFACTORING_PLAN.md` などに書かれているものは、それと突き合わせた。

---

## 1. 件数のまとめ

修正前(HEAD)と修正後の両方に同じ計測スクリプトを当てた数。件数の単位は「違反箇所」(定数名・略語は出現箇所)。
設計判断が必要で機械的に数えられない項目は下の表に分けた。

### 1.1 機械計測した項目

| 優先度 | 項目 | 修正前 | 修正後 |
| --- | --- | ---: | ---: |
| Critical | `ENGINE_ERRLOG` の条件反転 | 2 | **0** |
| Critical | 層の逆流(Engine/Editor → App、Engine → Editor) | 2 | 2 |
| Critical | PCH 構造(トップ PCH が Engine 全体を読む) | 1 | 1 |
| Critical | 描画の global state(`g_skinning` / `g_particle`) | 2 | 2 |
| High | global namespace への直接定義 | 269 | 254(すべて App。Engine 側は **0**) |
| High | `assert` | 95 | **0** |
| High | `shared_ptr`(共有理由なし) | 35 | **0** |
| High | 生ポインタでの所有権移譲 | 4 | **0** |
| High | plain `enum` | 5 | **0** |
| High | 可変 global 変数(namespace スコープ) | 11 | 10 |
| High | `Get` が可変参照を返す | 3 | **0** |
| Medium | 定数名(大文字+`_` でない) | 195 | **0** |
| Medium | `enum class` の型名に `E` が無い | 11 | **0** |
| Medium | enum 値が PascalCase でない | 6 | **0** |
| Medium | static メンバの `s_` 無し | 2 | **0** |
| Medium | 独自略語(NG 例と同種 : `_rg` `_rt` `_resMgr` `m_useFlg` `_cpDebDev`) | 60 | **0** |
| Medium | const でない `Get` メンバ関数 | 70 | 13 |
| Low | クラス/構造体の役割コメント無し | 307 | 307 |
| Low | ヘッダ公開関数のコメントが `///` でない | 91 | **0** |
| Low | global namespace の型エイリアス(Pch.h の `ComPtr` / `DXSM`) | 2 | 2 |

### 1.2 設計判断が必要な項目(手動で数えたもの)

| 優先度 | 項目 | 修正前 | 修正後 |
| --- | --- | ---: | ---: |
| High | namespace とディレクトリの不一致(D3D12 / Raytracing / Particle / Animation / JobSystem / StateGraph / Utility 2) | 8 | 8 |
| High | トップレベルが `Engine`/`App` 以外(`Math`) | 1 | 1 |
| High | App のディレクトリで `Engine::ECS` に型を定義(PhaseTag) | 1 | 1 |
| High | Build 構成(GPU Validation 無効 / VS 構成が Debug・Release のみ) | 2 | 2 |
| High | 残すと決めていないシングルトン(SceneManager / OptionManager / AudioManager / InputManager / GameManager) | 5 | 5 |
| Medium | クラスレイアウト(変数の後の関数 / public 変数 / private の後の public) | 9 | **0** |
| Medium | Component がメソッドを持つ | 8 | 8 |
| Medium | 遅延実行の命名が `Reserve` でない(`Request*` 等) | 10 | 10 |
| Medium | 責務が大きい System(BossCombatIntentSystem 622 行) | 1 | 1 |
| Low | ヘッダ公開関数にコメントが無い | 77 | 77 |
| Low | 関数内 static の可変状態 | 26 | 20 |
| Low | 綴り誤りの識別子(`isBoostTriger` `SerchGround` `BoidSpowner` `gPosOnry` `_nameCach` `IsSomethigInput`) | 6 | 5 |

### 1.3 合計

```text
修正前:
Critical   7
High     439
Medium   372
Low      509

修正後:
Critical   5
High     281   (うち App の global namespace 254)
Medium    32
Low      411   (うち役割コメント無し 307)
```

### 1.4 ルール解釈待ち(件数に含めない)

| 項目 | 件数 | 状況 |
| --- | ---: | --- |
| `struct` のフィールドに `m_` が無い | 2143 | `class` のメンバは 1364/1364 件すべて `m_`。`struct` は一貫して prefix 無し(§7.1) |
| `ctx` / `cmd` / `buf` / `tex` の略語 | 約 1100 | 一般的な略語として許すかの決定待ち(§7.2) |

---

## 2. Critical

> §2〜§5 は**修正前**の監査結果(行番号も修正前のもの)。どれを直したかは §6 を参照。

| 優先度 | ファイル | 行 | カテゴリ | 現在のコード | 問題 | 修正案 |
| --- | --- | -: | --- | --- | --- | --- |
| Critical | Engine/Graphics/Device/GraphicsDevice/GraphicsDevice.cpp | 56 | Error Handling | `if (FAILED(_hr)) { ENGINE_ERRLOG(FAILED(_hr), "DXGIファクトリの生成に失敗"); ...` | `ENGINE_ERRLOG(cond)` は **cond が false のとき**記録する。失敗時に cond が true になるので何も記録されず止まらない | `ENGINE_ERRLOG(SUCCEEDED(_hr), ...)` |
| Critical | 同 | 158 | Error Handling | `ENGINE_ERRLOG(FAILED(_hr), "デバイス生成に失敗")` | 同上(デバイス生成失敗が無言で通る) | 同上 |
| Critical | Engine/Editor/EffectEditor/EffectEditor.cpp, Engine/Editor/Panel/HierarchyPanel/HierarchyPanel.cpp | 27–31, 4–16 | Dependency | `#include "Application/Components/..."` / `App::Utility::SpawnEffectAt(...)` | Editor は `Engine/` 配下・`Engine::Editor` にあるのに App を読む。ディレクトリ上は **Engine → App** の逆流 | Editor を Engine の外(`Source/Editor`)へ出し、`Editor → App → Engine` を構造で表す。または App 側の型を受け取る窓口(コールバック/登録)を Engine に置く |
| Critical | Engine/MainEngine.cpp, Engine/Scene/SceneManager/SceneManager.cpp, Engine/Scene/BaseScene/BaseScene.cpp, Engine/Option/* | 22–36 ほか | Dependency | `#include "Editor/Editor.h"` など | **Engine → Editor**(REFACTORING_PLAN の「MainEngine の駆動 8 / SceneManager の通知 3」の残り) | 計画書フェーズ1の続き。Editor の駆動を App/最上位へ移し、通知はコールバック化 |
| Critical | Source/Pch.h → Engine/EngineCommon.h | 132 | PCH / Include | トップ PCH が EngineCommon.h(Engine のほぼ全ヘッダ)を読み、さらに ForcedInclude | ルール 5.1(トップは基礎ヘッダのみ)・5.2(分類別 PCH)違反。全ヘッダが Engine 全体を暗黙に見えるため、**3章の「自分で include する」が検証できない** | Pch.h は STL/Windows/DirectX まで。EngineCommon.h を Engine 用カテゴリ PCH に、App 用 PCH を新設。ヘッダ単体コンパイルのチェックを用意してから段階的に |
| Critical | Engine/Graphics/FrameCompute/SkinningPass/SkinningPass.cpp | 28 | Global Variable | `SkinningRuntime g_skinning = {};`(global namespace の無名 namespace) | 描画の実行状態(PSO・ルートシグネチャ・PSOManager)が global。初期化/解放順がコードから読めない | GraphicsEngine が所有する `SkinningCompute` クラスへ |
| Critical | Engine/Graphics/FrameCompute/ParticleSimulation/ParticleSimulation.cpp | 46 | Global Variable | `ParticleRuntime g_particle = {};` | 同上 | 同上 |

---

## 3. High

### 3.1 Namespace(global namespace への直接定義)

| 優先度 | ファイル | 行 | カテゴリ | 現在のコード | 問題 | 修正案 |
| --- | --- | -: | --- | --- | --- | --- |
| High | Application/Components/**/*.h(約 170 型) | – | Namespace | `struct ActualVelocityComponent { ... };` | Component がすべて global namespace | `App::Components`(ディレクトリ名)へ。`ComponentTraits` の特殊化は namespace を閉じてから書く |
| High | Application/Systems/**/*.h/.cpp(約 70 型・関数 21) | – | Namespace | `class GravitySystem : public App::ECS::APPISystem` | System が global namespace(`App::Systems::HierarchyTransform` 等の一部だけ namespace 付き) | `App::Systems` へ |
| High | Application/InstanceResource/*.h(17 型) | – | Namespace | `struct HitEventResource`, `enum class EHitEventType` | global namespace | `App::InstanceResource`(または `App::ECS`) |
| High | Application/App.h | 3 | Namespace | `class Application` | アプリ本体が global namespace | `App::Application` |
| High | Engine/Graphics/D3D12/CBAllocator/CBAllocator.h | 4 | Namespace | `class CBAllocator` | global namespace | `Engine::D3D12` |
| High | Engine/Graphics/D3D12/D3D12Common.h | 3 | Namespace | `struct SAMPLER` | global namespace・定数風の名前 | `Engine::D3D12` |
| High | Engine/Graphics/GraphicCommon.h | 20 | Namespace | `enum class EGeometryQueue` | global namespace | `Engine::Graphics` |
| High | Engine/Graphics/Animation/AnimationEvaluator/AnimationEvaluator.cpp | 8 | Namespace | `template<class T> int BinarySearchNextAnimKey(...)` | global namespace | `Engine::Animation` の無名 namespace |
| High | Engine/Resource/Data/Model/IO/Parser/tinyGLTF/tinyGLTF.cpp | 41–1405 | Namespace | `class GLTFBufferGetter`, `ParseMaterial` ほか 8 関数 | global namespace(外部リンケージ) | `Engine::Resource::Parse` の無名 namespace |
| High | Engine/Window/NativeWindow.cpp | 62 | Namespace | `LRESULT CALLBACK WndProc(...)` | global namespace | `Engine::Window` の無名 namespace |
| High | Engine/Utility/Math/** | – | Namespace | `namespace Math { ... }` | トップレベルが `Engine` / `App` 以外(ルール 1.4) | `Engine::Math`(参照 数千箇所。§8 参照) |
| High | Application/ECS/PhaseTag/PhaseTag.h | – | Namespace / Dependency | App のディレクトリで `namespace Engine::ECS { struct AwakeTag ... }` | App が Engine の namespace に型を足している | `App::ECS` へ(Engine 側の参照有無を確認してから) |

### 3.2 ディレクトリと namespace の不一致

| 優先度 | ディレクトリ | 現在の namespace | 修正案 |
| --- | --- | --- | --- |
| High | Engine/Graphics/D3D12 | `Engine::D3D12` | `Engine::Graphics::D3D12` か、D3D12 を `Engine/D3D12` へ移す |
| High | Engine/Graphics/Raytracing | `Engine::Raytracing`(クラス名も `RayEngine` / `RayWorld`) | 同上 |
| High | Engine/Graphics/Particle | `Engine::Particle` | 同上 |
| High | Engine/Graphics/Animation | `Engine::Animation` | 同上 |
| High | Engine/JobSystem | `Engine::Thread` | `Engine::JobSystem` かディレクトリを `Thread` に |
| High | Engine/Resource/StateGraph | `Engine::StateGraph` | `Engine::Resource::StateGraph` |
| High | Engine/Utility/GUID, Engine/Utility/Pool の一部 | `Engine` 直下 | ルール 1.4「Utility は役割の namespace」→ `Engine::Pool` に統一 |
| High | Engine/Editor | `Engine::Editor` | §2 の層の逆流と合わせて判断 |

### 3.3 Ownership

| 優先度 | ファイル | 行 | カテゴリ | 現在のコード | 問題 | 修正案 |
| --- | --- | -: | --- | --- | --- | --- |
| High | Engine/ECS/System/SystemManager.h/.cpp, Engine/ECS/World/World.h, Application/ECS/World/APPWorld.h | 146, 216 / 89 / 237 / 244 | Ownership | `std::vector<std::shared_ptr<ISystem>> m_systemVec;` / `make_shared<System>()` | 所有者は SystemManager だけ。共有理由が無い | `std::unique_ptr` |
| High | Engine/Input/InputCollector/InputCollector.h/.cpp | 74–91 | Ownership | `unordered_map<ActionID, shared_ptr<InputButtonBase>>` / `GetButton()` が `shared_ptr` を返す | デバイスは Collector ごとに新規生成され共有されていない。Get が所有権を配っている | `unique_ptr` で持ち、`Get` は `const T*` を返す |
| High | Engine/Input/InputCollector/InputCollector.cpp | 119–130 | Ownership(2.4) | `AddButton(ActionKey, InputButtonBase* a_pButton)` → 内部で `shared_ptr` に包む | 生ポインタで所有権を受け取る(ルール 2.4 違反)。呼び出し元なし | 削除 |
| High | Engine/Input/InputDevice/Axis/InputAxisForWindows*, Application/Game/InputActions/InputManager/InputActionManager.cpp, Engine/Input/InputManager/InputManager.cpp | – | Ownership | `std::make_shared<InputButtonForWindows>(...)` | 同上 | `std::make_unique` |
| High | Engine/Resource/Data/Model/IO/Parser/tinyGLTF/tinyGLTF.cpp | 435–633 | Ownership | `std::vector<std::shared_ptr<GLTFPrimitive>> _tmpPrimitives;` | 関数内だけの一時配列。共有なし | `unique_ptr` |
| High | Engine/Utility/Algorithm/Graph/TopologicalSort.h | 16 | Ownership | `TopologicalSort(const std::vector<std::shared_ptr<Node>>&, ...)` | 呼び出し元なし。`shared_ptr` を前提にした API を残す理由が無い | 削除 |
| 許可 | EmitterSlotPool.cpp:41,145 / GPUParticlePool.cpp:59–61,177 / ParticleBufferManager.cpp:27 | – | Ownership | `shared_ptr` を遅延解放ラムダへ捕獲 | `std::function` はコピー可能でなければならず、GPU 完了後に解放するため。**ルール 2.3 の例外(遅延解放)に該当** | 変更しない |
| Low | Engine/Resource/Data/Texture/IO/Importer/TextureImporter.cpp | 201 | Ownership | `std::make_shared<ComPtr<ID3D12Resource>>(...)` | 遅延解放だが `ComPtr` 自体が参照カウントを持つので二重 | `ComPtr` をそのまま捕獲 |

### 3.4 Error Handling(`assert`)

| 優先度 | ファイル | 行 | カテゴリ | 現在のコード | 問題 | 修正案 |
| --- | --- | -: | --- | --- | --- | --- |
| High | Engine 配下 44 ファイル(RootSignatureBuilder 7・ShaderTable 5・PipelineStateManager 5・StaticBuffer 5 …) | – | Error Handling | `assert(0 && "ルートシグネチャの生成に失敗");` / `assert(cond && "...")` | ルール 2.8 : `assert` ではなく `ENGINE_ERRLOG` 系 | `ENGINE_ERRLOG(false, "...")` / `ENGINE_ERRLOG(cond, "...")` |

RTTI : `dynamic_cast` / `typeid` の**実使用は 0**(コメント内の言及のみ)。型判別は `Engine::TypeInfo` が担っており適合。

### 3.5 Enum

| 優先度 | ファイル | 行 | 現在のコード | 修正案 |
| --- | --- | -: | --- | --- |
| High | Engine/Common/EngineConfigTypes.h | 13 | `enum : UINT { ... }`(定数の束) | `constexpr` 定数 |
| High | Engine/Graphics/Raytracing/Common/Common.h | 12, 21 | `enum ShaderCategory`, `enum LocalRootSignature` | `enum class E...` |
| High | Engine/Input/InputDevice/Axis/InputAxisForWindows/InputAxisForWindows.h | 23 | `enum EDir` | `enum class EDir` |
| High | Engine/Input/InputDevice/Button/InputButtonBase.h | 11 | `enum EState : short` | `enum class EState`(戻り値 `short` との変換を整理) |

### 3.6 可変 global 変数

| 優先度 | ファイル | 行 | 現在のコード | 判断 |
| --- | --- | -: | --- | --- |
| High | Application/Object/SequenceBgm.cpp | 18–19 | `float g_globalDuck` / `std::vector<SequenceBgm*> g_livingBgmVec` | 修正候補(BGM の管理者に持たせる)。設計変更を伴う |
| High | Engine/Editor/Helper/EditorField.cpp | 150–163 | `s_itemWidthStack` / `s_nextItemWidth` / `s_hasNextItemWidth` / `s_lastRow` | ImGui 同様の即時モード UI 状態。Editor コンテキストへ移すのが筋 |
| High | Engine/Editor/ImGui/ImGuiContext.cpp | 31 | `g_pImGuiHeapManager` | ImGui の C コールバック(ユーザーデータ無し)から参照するため。**外部ライブラリ都合の例外** |
| High | Engine/Editor/Panel/.../AudioBehaviorEdit.cpp | 20–21 | `g_previewGUID` / `g_previewInstance` | エディターのプレビュー状態。パネルのメンバへ |
| High | Engine/Editor/Panel/.../ModelEdit.cpp | 104 | `s_dirtyModelSet` | 同上 |
| High | Engine/Utility/GUID/GUID.h | 41 | `inline GUID DefaultGUID = {};` | 実質定数なのに `const` が無く書き換え可能 → `const` 化 |
| High | Engine/Resource/Data/*/IO/*IO.cpp(6 ファイル) | – | `static std::string _dir = "Asset/..."` | 実質定数なのに可変 → `const` 化 |
| 許可 | Engine/Utility/Debug/DebugLog.h:7, Profile/TimeProfileScope.h:11 | – | `g_logCallback` / `g_profileCallback` | REFACTORING_PLAN フェーズ1で決めた「横断的関心事はコールバック」。設計上の例外 |
| 許可 | Engine/Utility/Math/Random.h:8 / JobSystem/Profile/ThreadProfiler.cpp:109 | – | `thread_local` の乱数エンジン / 計測ポインタ | スレッドごとの作業領域。例外として扱う |
| High | Engine/ECS/System/SystemManager.h:213, ECS/Resource/ResourceTypeManager.h | – | `inline static uint32_t s_systemCounter` など | 型 ID の採番。ルール上は global state だが、型 ID の仕組みそのもの(2.7 の Type ID)。例外として明記を推奨 |
| High | シングルトン 9 個(MainEngine / SceneManager / OptionManager / AudioManager / InputManager / GameManager / MainEditor / ObjectMetaRegistry / ResourceManager) | – | `static X& Instance()` | REFACTORING_PLAN §0 で「残す判断」が書かれているもの以外(SceneManager / OptionManager / AudioManager / InputManager / GameManager)は計画書フェーズ5の続き |

### 3.7 Accessor

| 優先度 | ファイル | 行 | 現在のコード | 修正案 |
| --- | --- | -: | --- | --- |
| High | Engine/ECS/World/World.h | 261 | `ResourceType& GetResource();`(呼び出し 106 箇所) | `RefResource()` に改名し、読み取り用の `const ResourceType& GetResource() const` を追加 |
| High | Engine/ECS/Resource/ResourceStore.h | 24 | `ResourceType& Get();` | `Ref()` + `const Get() const` |
| High | Engine/Option/OptionManager.h | 121 | `static OptionManager& GetInstance()` | 他のシングルトンと同じ `Instance()` |
| Low | Engine/Editor/Helper/EditorField.h | 48 | `T& GetValue(void* a_data)` | 型付けの変換関数でありアクセサではない。全 Component の Traits が使うので据え置き |

### 3.8 Build Configuration

| 優先度 | ファイル | 行 | 現在 | 問題 | 修正案 |
| --- | --- | -: | --- | --- | --- |
| High | Engine/MainEngine.cpp | 73–84 | Debug モードで `EnableDebugLayer()` のみ | ルール 6.1 の **GPU Validation が無効** | `ID3D12Debug1::SetEnableGPUBasedValidation(TRUE)`。ただし DXR/バインドレス込みで大幅に遅くなるため、DebugOptions のフラグで切り替えを推奨 |
| High | BaseProject.vcxproj | – | VS の構成は Debug / Release のみ。Debug/Development/Shipping は **実行時に JSON(BuildConfig)で切り替え** | Editor・デバッグ機能がどの構成でもコンパイルに含まれる。Shipping の「すべて除外」を実現できない構造 | Shipping 着手時に VS 構成(またはプリプロセッサ定義)を追加 |

---

## 4. Medium

| 優先度 | 対象 | 件数 | 例 | 修正案 |
| --- | --- | -: | --- | --- |
| Medium | 定数名(namespace スコープ) | 約 45 | `kMaxInstanceNum`, `kGroupBits`, `GameSettingDir`, `gPosOnryLayout`, `StaticLayout` | `MAX_INSTANCE_NUM` など大文字+`_` |
| Medium | 定数名(クラスの `static constexpr`) | 約 40 | `kRootCameraCB`, `kAtlasTiles`, `kInvalid`, `kExtension` | 同上 |
| Medium | 定数名(関数内の `constexpr`) | 約 120 | `_kMaxDepth`, `_kTurnSpeed`, `_TWO_PI`, `_pi` | 定数は Prefix を付けない : `MAX_DEPTH` |
| 許可 | 型特性の変数テンプレート | 4 | `IsQueryOnlyTag_v`, `IsCastTarget_v`, `kAlwaysFalse`, ヒープ型の `type` | STL の `_v` / `::type` 慣習に合わせたもの。据え置き |
| Medium | `enum class` の型名に `E` が無い | 11 | `TextureUsage`, `GPUTier`, `ArchiveFormat`, `RootParameterType`, `SceneChangeType` … | `ETextureUsage` など |
| Medium | enum 値が PascalCase でない | 6 | `EAccessType`, `ECoordinateSystem`, EngineConfigTypes の無名 enum | PascalCase |
| Medium | static メンバの `s_` 無し | 2 | `ComponentTypeSlot::id`, `::pOwner` | `s_id`, `s_pOwner` |
| Medium | 独自略語(ルールの NG 例と同種) | 5 識別子 | `_rg`(RenderGraphCompiler), `_rt`(平行移動), `_resMgr`(8 ファイル), `m_useFlg`, `_cpDebDev` | `_compiler`, `_translation`, `_resourceManager`, `m_usage`, `_cpDebugDevice` |
| Medium | クラスレイアウト | 13 | EffectEditor / AsyncGPUManager / SystemManager / ComponentMetaRegistry / ParticlesAsset / CopyPass / MonitorPass / IPanel(public メンバ変数) / FinalOutputPass(public メンバ変数) … | public 関数 → private 関数 → private 変数。シングルトンの `Instance()` を末尾の別 public に置く慣習は 8 クラスで一貫しているので据え置き候補 |
| Medium | Component がメソッドを持つ | 8 | `GunStateComponent::HeatRatio()`, `MissileLockComponent::IsFiring()`, `ActionIntentComponent::IsAnyWeaponShoot()` … | 自分のフィールドだけを読む派生値で、他 Component への依存は無い。free function へ出すかを判断(ロジックとまでは言えない) |
| Medium | 遅延実行の命名が `Reserve` でない | 約 10 | `BaseObject::RequestDestroy()`(フラグを立てて後で破棄), `StateMachine::RequestChangeState`, `ParticleBufferManager::RequestEmit`, `MainEngine::RegisterDeferredResource` | ルール 1.5 の `Reserve*`。ただし `Request` は「取得 or 生成」(PSO / Shader / ResourceManager::RequestLoad)の意味でも使われており、語彙の整理が先 |
| Medium | 大きい System | 1 | BossCombatIntentSystem.cpp(622 行) | 責務の分割を検討 |
| Medium | RenderGraph : グラフ外でのステート遷移 | 1 | MonitorPass.cpp:110–112(エディターのプレビュー用テクスチャ) | グラフ外の持ち物なので自前遷移。**既存設計上の例外**として据え置き |
| Medium | RenderGraph : 1 Pass 複数 Draw | 1 | ShadowMapPass.cpp:101(カスケードごとにアトラスへ描く) | 1 リソース(アトラス)への書き込みを 1 Pass にまとめる設計。**例外**として据え置き |
| Medium | RenderGraph 外の GPU 処理 | 3 | FrameCompute/Skinning・ParticleSimulation・UpdateBLAS(手動バリア) | 「カメラに依存せずフレーム 1 回」のためグラフに載せない設計(コメントに明記)。例外として据え置き。global state は §2 で扱う |

ECS の良い点(適合を確認したもの) :
* Component は `ComponentMetaRegistry` で `static_assert(std::is_trivially_copyable_v<Comp>)` が強制されている。他 Component へのポインタ保持は 0。
* System は状態を持たず、タスク登録時に `Reads/Writes/After/Before` で依存を明示(ECS 監査 Step3/4 の結果)。
* 遅延操作は `World::Reserve*` → `ApplyReserved*`、即時版 `RemoveEntity` などは `protected`(ルール 1.7 適合)。

---

## 5. Low

| 優先度 | 対象 | 件数 | 内容 |
| --- | --- | -: | --- |
| Low | クラス/構造体の役割コメント | 約 300 無し / 形式混在 | `//====` バナー 224、`//` 196、`/// <summary>` 127、`//---` 20、無し 307(うち約 105 は `ComponentTraits` 特殊化で不要) |
| Low | ヘッダ公開の free function の `///` | 約 150 | `//` で書かれている(EditorField.h 52 など)。無しも一部 |
| Low | 関数内 `static` の可変状態 | 18 | エディターの入力バッファ(`static char _name[256]`)・一度だけのログ抑止フラグ・キャッシュ |
| Low | 綴り誤りの識別子 | 7 | `isBoostTriger`, `SerchGroundComponent`, `BoidSpownerComponent`, `gPosOnry*`, `_nameCach`, `IsSomethigInput` |
| Low | global namespace の型エイリアス | 2 | Pch.h の `ComPtr` / `DXSM`。外部ライブラリの別名で全体が使うため据え置き候補 |
| Low | トップレベルの無名 namespace | 約 60 | App の System の .cpp。`App::Systems` 化と同時に中へ入れる |

---

## 6. 修正結果

修正はすべて「1 種類ずつ変更 → Debug|x64 ビルド」で確認しながら進めた。
置換はコメント・文字列を除いたコードだけに行い、関数内の定数は宣言のあるブロックの中だけを対象にした(同名の別変数を巻き込まないため)。

### 6.1 主な変更

**Error Handling**
* `GraphicsDevice.cpp` の `ENGINE_ERRLOG(FAILED(_hr), ...)` 2 件を `SUCCEEDED(_hr)` に修正(DXGI ファクトリ・デバイス生成の失敗が記録されていなかった)
* `assert` 95 件(45 ファイル)を `ENGINE_ERRLOG` へ。メッセージの無かった 6 件には内容を足した。
  `ENGINE_ERRLOG` を使うようになったヘッダには `DebugLog.h` の include を足した
  (TopologicalSort.h などは EngineCommon.h の中で DebugLog.h より先に読まれるため必須)
  * 注意 : `ENABLE_RELEASE_LOG` が定義されているので、Release でも条件の評価とログ出力が行われる(以前の `assert` は Release で消えていた)

**Ownership**
* System の保持 : `SystemManager::Hold` / `World::HoldSystem` / `APPWorld::RegisterSystem` を `std::unique_ptr` に(所有者は SystemManager だけで変わらない)
* Input : `InputCollector` のボタン/軸、`InputAxisForWindows` の方向ボタン、`InputAxisForWindowsMouse` の固定ボタンを `std::unique_ptr` に。
  `GetButton` / `GetAxis` は `shared_ptr` を配らず `const T*` を返す。生ポインタで所有権を受け取る `AddButton(ActionKey, InputButtonBase*)` / `AddAxis(...)` は呼び出し元が無かったので削除
* glTF パーサの一時配列を `unique_ptr` に。呼び出し元の無い `TopologicalSort(vector<shared_ptr<Node>>)` を削除
* `TextureImporter` の `make_shared<ComPtr<...>>`(二重の参照カウント)を `ComPtr` の直接捕獲に
* 遅延解放の `shared_ptr`(EmitterSlotPool / GPUParticlePool / ParticleBufferManager)はルール 2.3 の例外なので変更していない

**Namespace**
* `CBAllocator` → `Engine::D3D12`、`SAMPLER` → `Engine::D3D12::SamplerTag`、`EGeometryQueue` → `Engine::Graphics`
* `BinarySearchNextAnimKey` → `Engine::Animation` の無名 namespace
* glTF パーサの補助関数・`GLTFBufferGetter` → `Engine::Resource::GLTF` の無名 namespace(外部リンケージだったものを内部へ)
* `WndProc` → `Engine::Window` の無名 namespace
* `Application` → `App::Application`

**Enum**
* plain enum : `EngineConfigTypes` の無名 enum → `inline constexpr UINT BACKBUFFER_COUNT / CPU_FRAME_COUNT`、
  `ShaderCategory` / `LocalRootSignature` → `enum class EShaderCategory / ELocalRootSignature`、
  `InputAxisForWindows::EDir` → `enum class`、`InputButtonBase::EState` → `enum class`
  (ビットフラグなので `EnumFlags.h` に `operator~` を他の演算子と同じ形で足し、判定は `Utility::HasFlag`。
  `GetState()` / `GetButtonState()` は `short` ではなく `EState` を返す。
  `InputButtonForXInput` は基底の `UpdateState()` と同じ処理を重複して持っていたので呼び出しに置き換えた)
* `E` 無しの enum class 11 件 : `EAlpha` `EArchiveFormat` `EAsyncCommandType` `EGPUTier` `ERangeType` `ERootParameterType`
  `ESceneChangeType` `ETextureUsage` `Archive::EMode`、App の衝突レイヤー `Layer` → `ECollisionLayer`
  (Engine の `Physics::Layer` namespace と紛らわしかったため)。D3D12 構造体のメンバ `.RangeType` は対象外にした
* enum 値 : `Depth_Read/Write` → `DepthRead/Write`、`RightHanded_YUp` → `RightHandedYUp` など(保存データに値名が入っていないことを確認済み)

**Naming**
* 定数 194 件を大文字+`_` に(`kRootCameraCB` → `ROOT_CAMERA_CB`、`_kMaxDepth` → `MAX_DEPTH`、`gPosOnryLayout` → `POS_ONLY_LAYOUT` など)。
  `EBroadPhaseLayer::Static` は `JPH::EMotionType::Static` を巻き込まないよう修飾付きの箇所だけ置き換えた。
  `RaytracingWorld` のローカル別名(`_maxInstanceNum = kMaxInstanceNum`)は不要になったので削除
* `DefaultGUID` → `const GUID DEFAULT_GUID`(書き換え可能な global だった)、IO の `static std::string _dir` 6 件 → `static const std::string ASSET_DIR`
* `ComponentTypeSlot::id / pOwner` → `s_id / s_pOwner`
* 独自略語 : `_rg` → `_compiler`、`_rt` → `_translation`、`_resMgr` → `_resourceManager`、`m_useFlg` → `m_usage`、`_cpDebDev` / `_debDev` → `_cpDebugDevice`

**Accessor**
* `World::GetResource()`(可変参照)→ `RefResource()` + 読み取り用 `const GetResource() const` を追加。呼び出し 106 箇所は const 参照で受けている 24 箇所を `GetResource`、残り 82 箇所を `RefResource` にした
* `ResourceStore::Get()` → `Ref()` + `const Get() const`
* `OptionManager::GetInstance()` → `Instance()`(他のシングルトンと同じ名前)
* `IPanel::m_isOpen`(public 変数)→ private にして `RefIsOpen()`

**Const correctness**
* `Get` 系メンバ関数 55 件に `const` を付けた(EntityManager / EntityStorage / World / DescriptorHeap / RayWorld / TLAS / ShaderTable / AssetDatabase など)

**Class Layout**
* `SystemManager` / `ParticlesAsset` : private の後ろにあった public(アクセサ)を前へ
* `EffectEditor` / `CopyPass` / `MonitorPass` / `AsyncGPUManager` / `ComponentMetaRegistry` : メンバ変数の後ろの関数を関数の private 塊へ
* `FinalOutputPass::m_isMismatchReported` を private へ

**Include**
* `InputManager.h` が `InputButtonBase` を自分で include していなかった(EngineCommon.h の読み込み順でたまたま通っていた)。enum class 化で表面化したので追加
* `InputCollector.h` は戻り値に `InputButtonBase::EState` を使うようになったので、前方宣言をやめて include

**Comment**
* ヘッダ公開の free function 91 件の直前コメントを `//` → `///` に(新しいコメントは足していない)

### 6.2 未修正項目

| ファイル | 内容 | 自動修正しなかった理由 | 推奨対応 |
| --- | --- | --- | --- |
| Application/Components・Systems・InstanceResource(254 定義) | global namespace | namespace 名を決める必要がある。参照側(App/Object・Engine/Editor)の修飾が数百ファイルに及ぶ大規模変更 | `App::Components` / `App::Systems` / `App::InstanceResource` を決め、ディレクトリ単位で段階的に移す。`ComponentTraits` の特殊化は namespace を閉じてから書く |
| Pch.h / Engine/EngineCommon.h | トップ PCH が Engine 全体 + ForcedInclude | include 漏れが一斉に表面化する。今回も 1 件(InputManager.h)見つかった | 先にヘッダ単体コンパイルの確認手段を作り、PCH をトップ/Engine/App/Editor に分ける |
| Engine/Editor/*、MainEngine.cpp、SceneManager.cpp | 層の逆流 | Editor の配置そのものの問題(計画書フェーズ1の残り) | Editor を Engine の外へ出す。`EditorField` は「Engine が持つ編集 UI の窓口」として Engine 側(Editor 以外)へ移す |
| FrameCompute/SkinningPass.cpp, ParticleSimulation.cpp | `g_skinning` / `g_particle` | 所有関係の変更になる(依頼の「勝手に変更しない」に該当) | `SkinningCompute` / `ParticleSimulationCompute` クラスにして GraphicsEngine がメンバで持つ。Setup/Execute の呼び出し元は GraphicsEngine だけなので影響は小さい |
| SequenceBgm.cpp | `g_globalDuck` / `g_livingBgmVec` | BGM の管理者を決める設計変更 | AudioManager か Sequence 側の管理者に持たせる |
| EditorField.cpp / AudioBehaviorEdit.cpp / ModelEdit.cpp | エディターの即時モード状態を global に持つ | ImGui 風 API の内部状態で、置き場所(EditorContext 等)を決める必要がある | EditorContext に持たせる |
| ImGuiContext.cpp | `g_pImGuiHeapManager` | ImGui の C コールバックにユーザーデータが無いため | 外部ライブラリ都合の例外としてルールに明記 |
| ResourceManager.h | `GetState` / `GetCache` が const にできない | 内部で `IsValid` / `RefSlot` / `RefData`(非 const)とキャッシュ用 mutex を使う | const 版の内部取得関数を足し、mutex を `mutable` に |
| DescriptorHeapManager.h | `GetCPU` / `GetGPU` が const にできない | `RefAllocator<T>()` が非 const | const 版の `GetAllocator<T>() const` を足す |
| SceneManager / CommandPool / RenderContext / World | `GetCurrentTopScene` / `GetCommandQueue` / `GetFence` / `GetCurrentCmdList` / `GetComponentArray` が可変ポインタを返す | 呼び出し側が書き込む用途なので const 化では済まない | 書き込み用途なら `Ref*` に改名 |
| APPWorld.h | `GetEntity(GUID)` が非 const | 索引を遅延で作り直すため | 索引を `mutable` にするか、作り直しを別関数へ |
| Engine/Utility/Math | トップレベル `Math` namespace | App(global namespace)からの参照が数千箇所 | App の namespace 化と同時に `Engine::Math` へ |
| Engine/Graphics/D3D12 ほか 8 | ディレクトリと namespace の不一致 | Graphics 全体の修飾が変わる | 方針(namespace に合わせてディレクトリを動かすか、その逆か)を決める |
| Application/ECS/PhaseTag | App で `Engine::ECS` に型を定義 | Engine/Editor が参照している | 層の整理と同時に `App::ECS` へ |
| MainEngine.cpp | GPU Validation が無効 | Debug の実行速度が大きく落ちる | DebugOptions のフラグで切り替え |
| BaseProject.vcxproj | VS 構成が Debug/Release のみ | Shipping 未着手 | Shipping 着手時に構成とプリプロセッサ定義を追加 |
| Component 8 件 | 派生値のメソッド(`HeatRatio` 等) | ロジックとまでは言えない小さなもの。free function にするかは判断 | ルールで「自分のフィールドだけを読む const の派生値は可」とするか、Utility へ出す |
| `RequestDestroy` / `RequestChangeState` / `RequestEmit` / `RegisterDeferredResource` など | 遅延実行が `Reserve` でない | `Request` が get-or-create(PSO / Shader / RequestLoad)にも使われている | 語彙を決めてから改名 |
| クラス 307 件 / ヘッダ公開関数 77 件 | 役割コメント無し | 「コメントを大量に追加しない」 | 触るたびに足す |
| 綴り誤り 5 件 | `SerchGroundComponent` など | 型名・ファイル名の変更を伴う | まとめて改名(保存キーは文字列なので互換は保てる) |

### 6.3 Build Result

```text
Debug|x64 フルリビルド
  修正前 : Build: Success / Warning: 2156(重複除去 420) / Error: 0
  修正後 : Build: Success / Warning: 2156(重複除去 420) / Error: 0
  ※ 警告は行番号を除いて修正前と同じ集合(新しい警告なし)

Release|x64
  修正後 : Build: Success / Warning: 2128 / Error: 0
```

実行(起動・シーン読み込み・ゲームモード往復)は確認していない。
特に Input(`EState` の enum class 化・`unique_ptr` 化)と `ENGINE_ERRLOG` の Release での評価は実機で確認してほしい。

---

## 7. ルール解釈待ち(CodingStandards.md への追記を推奨)

### 7.1 `struct` のフィールドに `m_` を付けるか
* 現状 : **`class` のメンバは 1364/1364 件すべて `m_`。`struct` のフィールドは 2143/2172 件が prefix 無し。** 偶然ではなく一貫した規約。
* `struct` は Component・Desc・CB データ・イベントなどの「データだけの型」で、`a_ar.Field("maxFuel", _comp.maxFuel)` のように保存キーとフィールド名が対になっている。
* 推奨 : CodingStandards.md 1.2 に「データだけの `struct` の公開フィールドは prefix を付けない。`m_` は振る舞いを持つ `class` のメンバに付ける」と明記する。

### 7.2 `ctx` / `cmd` / `buf` / `tex` は「一般的な省略」か
* `a_ctx` 783 回・`_pCtx` 154 回・`_cmd` / `a_cmd` 130 回・`_buf` 81 回・`_tex` 20 回。
* NG 例 `_rndCtx` は `rnd` の方が独自。`ctx`(context)・`cmd`(command list)は D3D12 / グラフィックスでは広く使われる。
* 推奨 : 許可する略語の一覧(GPU / CPU / D3D / DXGI / RT / ctx / cmd …)をルールに書く。決まれば機械的に揃えられる。

---

## 8. 大規模・設計判断が必要なもの(今回は変更せず報告)

| 項目 | 現在 → 問題 → 推奨 → 影響 |
| --- | --- |
| App の Component / System / InstanceResource の namespace 化 | global namespace(約 290 型) → ルール 2.1 違反 → `App::Components` / `App::Systems` / `App::InstanceResource` → **namespace 名の決定が必要**。参照する App/Object・Engine/Editor 側の修飾も要る(数百ファイル)。保存キーは文字列で明示登録されているのでデータ互換は壊れない |
| PCH の再構成 | §2 → 全翻訳単位が Engine 全体を見る → トップ/Engine/App/Editor の 4 段に分ける → ヘッダの include 漏れが一斉に表面化する。単体コンパイルのチェックを先に作る |
| Editor の層 | `Engine/Editor` が App を読み、Engine 本体が Editor を読む → ルール 4.1 → Editor を Engine の外へ → MainEngine の駆動・SceneManager の通知の付け替え(計画書フェーズ1の残り) |
| `Math` → `Engine::Math` | トップレベル namespace → ルール 1.4 → `Engine::Math` → App(global namespace)からの参照が数千箇所。App の namespace 化と同時が安全 |
| D3D12 / Raytracing / Particle / Animation / Thread の namespace | ディレクトリ不一致 → ルール 1.4 → namespace かディレクトリのどちらかを動かす → Graphics 全体の修飾が変わる |
| FrameCompute の global state | §2 → `SkinningCompute` などのクラスにして GraphicsEngine が所有 → 初期化・解放順が変わる(所有関係の変更) |
| GPU Validation | ルール 6.1 → 有効化 → Debug の実行速度が大きく落ちる。オプション化を推奨 |
| `Request*` → `Reserve*` | 遅延と get-or-create が同じ語 → 語彙を決めてから改名 |
| シングルトン | 計画書で「残す」と決めたもの以外 → 所有者へ移す(計画書フェーズ5) |
