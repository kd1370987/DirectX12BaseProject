# エフェクト リファクタリング手順

GPUパーティクルとエフェクト(EffectAsset / ParticlesComponent / 音のコンポーネント群)を
どの順番で直していくかの計画と、その進み具合。

- 調査日 … 2026-10-05
- 対象 … `Engine/Graphics/Particle`、`Engine/Graphics/FrameCompute/ParticleSimulation`、
  `ParticlePass`、`Asset/Shader/Source/Particle`、`Engine/Resource/Data/EffectAsset`、
  `Application/Components/Effect`・`Components/Audio`、`Application/Systems/Effect`・`Systems/Audio`

---

## 0. 先にゴールを決める

| # | 直すもの | なぜ |
|---|---|---|
| A | **発生源の席が増えない** | 席はアセット単位・定数バッファに固定 8 個(実際に使えるのは 7 個)。ブースターが多いと席が尽き、ワールド空間へ落ちる |
| B | **向きが決まらない** | 発生源を「位置 + 方向ベクトル 1 本」で渡しているので、方向軸まわりの回転(ロール)が無い。テクスチャを発生源の軸に合わせる手段がない |
| C | **同じことの経路が 3 重** | ParticlesComponent 系 / 音のコンポーネント 4 種 / EffectAsset 系。発生源の計算・時間・終了判定・後片付けがそれぞれにある |

最終形は **「エンティティに付けるのはエフェクトだけ。パーティクルと音はエフェクトのパーツ」**。
Unreal の Niagara(エミッターはシステムの部品で、単体では置けない)と同じ形。

**残してよいもの**は最初に決めておく(後で迷わないため)。

- `EffectPrefab` … 「エンティティを何体も撒く」演出(破片・砂柱)用。EffectAsset とは役割が違う
- BGM・UI 音・システム音 … 位置を持たない音はエンティティに付けない。シーン/AudioManager から直接鳴らす
  (環境設定を BaseScene に持たせたのと同じ考え方)
- `AudioListenerComponent` … 音を出すものではない
- 半透明パーティクルの奥行きソート … 今回はやらない(alive list を作るので、やるならその後)

---

## 1. 現状の問題一覧

### 1-1 バグ級

| # | 問題 | 場所 |
|---|---|---|
| B1 | ローカル空間の行列が**粒を出したフレームしか**更新されない。発生レートが fps より低いと噴射がガタつき、止めた後の残り粒は置き去りになる | `EffectDrawSystem.cpp:99`(continue)が `:122`(席の確保と行列の更新)より前 |
| B2 | 席の回収がフレーム数基準。120 フレームは 144fps で 0.83 秒しかなく、生きている粒の席を奪える | `ParticleData.h:25` |
| B3 | 新しいアセットの最初の発生が必ず捨てられる(単発の爆発は初回だけ出ない) | `ParticleBufferManager.cpp:72` |
| B4 | アセットが無いと、警告を出した後に null 参照する | `GPUParticlePool.cpp:14-20` |
| B5 | エフェクトの向き上書きが**全パーツに同じ向き**を入れる。複数パーツの構成が崩れる | `EffectDrawSystem.cpp:197` |

### 1-2 性能

| # | 問題 | 場所 |
|---|---|---|
| P1 | 描画が常に容量ぶん。粒が 0 個でも `容量 × プール数 × カメラ数` インスタンスの頂点シェーダーが走る | `ParticlePass.cpp:163` |
| P2 | 更新も全容量を Dispatch している | `ParticleSimulation.cpp:244` |
| P3 | 発生処理が 1 スレッド = 1 命令で、emitCount を直列にループする | `EmitParticleShaeder.hlsl:65` |
| P4 | 命令バッファがアセットごとに固定 100 件で、あふれた分を黙って捨てる | `ParticleBufferManager.cpp:270` |
| P5 | プールを作り直す経路も解放する経路もない(Capacity の変更が反映されず、VRAM も残る) | `ParticleBufferManager` |

### 1-3 表現

| # | 問題 | 場所 |
|---|---|---|
| E1 | 重力がワールドの -Y で固定。Local 空間では発生源の軸に掛かる | `ParticleSimulation.cpp:216` |
| E2 | 同じブレンドモード内の描画順が `unordered_map` の反復順で決まる | `ParticlePass.cpp:188` |
| E3 | 粒に回転が無い(テクスチャの角度を付けられない) | `ParticleData`(`pad0/pad1` が空いている) |
| E4 | 色・サイズがアセット単位の定数で、個体ごとに変えられない | `ParticleDrawData` |

### 1-4 構造

| # | 問題 |
|---|---|
| S1 | ParticlesComponent 経由の経路は Local 空間を一切見ない。`EEmitSpace` と `EEffectSpace` がほぼ同じ enum として 2 つある |
| S2 | EffectAsset(Resource 層)が再生ロジックを持ち、AudioManager に依存している |
| S3 | 開始/継続/終了のフェーズを表せない(AudioBehavior の Start/Loop/End、ParticlesComponent の火花)。ブースターのスパークは別エンティティを Spawn して代用している |
| S4 | 同じ型を 1 エンティティに 1 つしか持てないため、音を足すたびに型が増える(HitSoundComponent が存在するのはこれが理由) |
| S5 | 音の声を Fixup の時点で確保する。一発ものを大量に出すと、まだ鳴っていないのに声を握り続ける |
| S6 | `EFFECT_POINTLIGHT_MAX` は定義されているのに、ライトのパーツが無い |

---

## 2. 進め方の全体像

```
Phase1 即効のバグ修正 ─┐
                       ├─ Phase2 EmitterSlotPool ─ Phase3 ローカル生成→行列 ─┐
                       │                                                      ├─ Phase6 移行と削除
                       └──────────────── Phase5 EffectAsset の拡張 ────────────┘
                         Phase4 GPU の回し方(Phase2 以降ならいつでも。独立)
```

| Phase | 内容 | 規模 | 進捗 |
|---|---|---|---|
| 1 | 即効のバグ修正(B1・B3・B4、P4 の警告) | 小 | **実装済み・動作確認待ち**(2026-10-05) |
| 2 | 発生源テーブルを全体共通・動的にする(EmitterSlotPool) | 中 | **実装済み・動作確認待ち**(2026-10-05)。デバッグ表示は Profiler パネル(Engine)の「Particles」 |
| 3 | ローカルで形状を作り、最後に行列を掛ける。粒の回転 | 中 | **3-A〜3-D 実装済み・動作確認待ち**(2026-10-05)。向きは EmitterAxis / EmitterFacing の2つを追加。3-D に合わせて Booster_Jett / BoostSpark / MazleFlash / Worm_GroundDust のパーツの向きを +Y → +Z に変更。Local のオフセットに持ち主のスケールを掛けるかは保留 |
| 4 | GPU の回し方(prefix sum・alive list・間接描画) | 中〜大 | **4-A・4-B 実装済み・動作確認待ち**(2026-10-05)。4-C は1段目(ExecuteIndirect)・2段目(生存リスト。描画は生きている粒の数ぶんだけ)まで実装済み・動作確認待ち。4-D は1段目(命令バッファを全プール共通の1本に)・2段目(1スレッド = 1粒)まで実装済み・動作確認待ち。4-E は1段目(取り残されたプールの解放・遅延解放での Release 漏れの修正)・2段目(Capacity が変わったら 0.5 秒落ち着いてから作り直す)まで実装済み・動作確認待ち。4-E の3段目と 4-F は未着手 |
| 5 | EffectAsset の拡張(統一の前提条件) | 大 | 未着手 |
| 6 | ParticlesComponent・音のコンポーネント群を移行して削除 | 中 | 未着手 |

Phase 2 と 3 は、どちらも EmitterData・ParticleData・シェーダーを触る。
**続けて(できれば同じブランチで)やる**と、シェーダーとの並び合わせを 1 回で済ませられる。

---

## Phase 1 : 即効のバグ修正

今の席表のままで直せるもの。Phase 2 で置き換わる部分にも、
不具合が見えているうちは手を入れておく(Phase 2 の比較対象にもなる)。

| # | やること | 完了条件 |
|---|---|---|
| 1-1 | EffectDrawSystem で、席の確保と行列の更新を `pendingEmit<=0` の continue より前に出す(エフェクト 1 つにつき 1 回) | emitRate を低くしても噴射がガタつかない |
| 1-2 | (a) 「使えるか」をフレームの頭(BeginFrame)で1回だけ確定させる(完了コールバックはワーカースレッドから任意のタイミングで来るため)<br>(b) 準備中に来た発生命令を捨てずに持ち越す<br>(c) Fixup で参照しているパーティクルのプールを先に作る | 単発の爆発が初回から出る |
| 1-3 | `GPUParticlePool::Init` の null ガード | アセットが欠けていても落ちない |
| 1-4 | 命令バッファがあふれたら、アセット名付きで 1 回だけ警告を出す | あふれていることが分かる |

B2(フレーム数基準の回収)は Phase 2 で仕組みごと消えるので、ここでは直さない。

---

## Phase 2 : EmitterSlotPool(発生源テーブル)

### 形

- **GPU 側**: 全アセット共通で `StructuredBuffer<EmitterTransform>` を **1 本だけ**持つ(bindless の SRV)。
  ParticleDrawData の `emitterMatrices[8]` は削除する(定数バッファが 512 バイト軽くなる)。
- **CPU 側**: `HandlePool` で席番号と世代を管理し、行列の写しを `std::vector` で持つ。
  ブロック(例: 64 席)は「伸ばす単位」として使う。
- **席 0 = 単位行列**(ワールド空間の粒)は今と同じ。初期化で確保し、二度と返さない。
- **持ち主**: エフェクト 1 つにつき 1 席。`EffectRuntimeComponent` がハンドルを持つ。
  パーツの相対配置は、粒のローカル座標に焼き込まれる(今もそうなっている)。
- **寿命**:
  - 確保 … 初めて Local のパーツを出すとき(または Fixup)
  - 更新 … **毎フレーム**。粒を出したかどうかは関係ない(B1 が直る)
  - 返却 … `ComponentTraits<EffectRuntimeComponent>::Release` で「返却の予約」をする。
    **使っているアセットの最大寿命(秒)が経ってから**空きに戻す(B2 が直る)
  - Stop しても席は返さない。エンティティが生きている限り、残り粒は持ち主について動く

### API の案

```cpp
class EmitterSlotPool
{
public:
	void Init(uint32_t a_blockSize);	// 席 0 を単位行列で予約する
	void Release();						// GPU バッファを返す(DescriptorHeapManager より先)

	Handle<EmitterTransform> Acquire();								// 足りなければ blockSize ぶん伸ばす(CPU 側だけ)
	void SetTransform(Handle<EmitterTransform> a_h, const Math::Matrix& a_world);	// 拡縮を落として書く。世代が違えば無視
	void ReserveReturn(Handle<EmitterTransform> a_h, float a_holdSeconds);			// 期限が来たら空きへ戻す

	void BeginFrame(float a_dt);										// 期限切れの席を HandlePool へ戻す
	void Upload(D3D12::GraphicsCommandList* a_pCmd, UINT a_frameIndex);	// 足りなければ作り直し → [0, 使用最大) を転送

	uint32_t GetSRVIndex() const;
	static uint32_t ToGPUIndex(Handle<EmitterTransform> a_h);			// 無効なら 0(単位行列)
};
```

### 置き場と呼ぶ順

| いつ | 誰が | 何を |
|---|---|---|
| MainEngine::BeginFrame | ParticleBufferManager::BeginFrame | `EmitterSlotPool::BeginFrame`(期限切れの席を戻す) |
| ECS Draw | EffectDrawSystem | `Acquire` / `SetTransform` |
| ECS Release | `EffectRuntimeComponent` の Release フック | `ReserveReturn` |
| GraphicsEngine(UploadEmitData の隣) | ParticleBufferManager | `Upload` |
| ParticleSimulation / ParticlePass | - | `GetSRVIndex` をルート定数で渡す |

### 完了条件

- プレイヤーとボスのブースターを全部出しても「席が足りません」の警告が出ない
- シーンを何度読み直しても、使用中の席数が増え続けない
- `PARTICLE_EMITTER_MAX`・`EMITTER_SLOT_KEEP_FRAMES`・`EmitterSlotTable`・`AcquireEmitterSlot` が消えている

---

## Phase 3 : ローカルで形状を作り、最後に行列を掛ける

| # | やること |
|---|---|
| 3-1 | EmitterData の `emitPos`/`emitDirection` をやめて `float3x4 emitMatrix` を持たせる。形状(Cone/Sphere/Hemisphere)は**常にローカル(+Z 前方)**で作り、`emitMatrix` を掛ける |
| 3-2 | `emitMatrix` の中身は CPU 側で切り替える。<br>World 空間 = 置き場の行列 × パーツのローカル行列(結果をワールドで保存し、emitterIndex は 0)<br>Local 空間 = パーツのローカル行列だけ(席の座標系で保存し、描画時に席の行列で戻す)<br>**GPU 側のコードは 1 本**になる |
| 3-3 | `EEffectSpace` の WorldMatrix / ReverseVelocity は、「置き場の回転の作り方」として CPU 側で吸収する(ReverseVelocity = 速度の逆を +Z にした回転) |
| 3-4 | ParticleData の `pad0/pad1` を `rotation` と `angularVelocity` にし、発生時の向き `float4 orientation`(クォータニオン)を足す。48 → 64 バイト(容量 10000 で +160KB/プール)。アセットに初期角・回転速度の範囲を足す(Archive は末尾に追加) |
| 3-5 | `EParticleOrientation` に `EmitterAxis`(板の軸を `orientation` から取る。Local なら席の回転も掛ける)を末尾に足す。Billboard にも回転を掛ける |
| 3-6 | Update にも席の SRV を渡し、Local の粒には重力を回転の転置でローカルへ回してから掛ける(E1) |
| 3-7 | EffectOverride の `overridePosOffset`/`overrideEmitDir` を「置き場の位置・回転」に変える。パーツは相対配置のまま(B5) |

**決めること**: いまの Local 空間はパーツのオフセットに持ち主のスケールを掛けていない(World 空間とメッシュパーツは掛けている)。
「常にワールドの発生行列を作り、Local なら席の逆行列を掛ける」形に寄せると Local も掛かる側に揃う。
揃えるなら、Local のアセットを使っているもの(ブースター)のオフセットを見直す。

**完了条件**: ブースターの噴射テクスチャが機体のロールに追従する。横向きのパーツを含むエフェクトに上書きを掛けても構成が崩れない。
EffectDrawSystem のローカル/ワールドの分岐が、行列を組み立てるだけになっている。

---

## Phase 4 : GPU の回し方

Phase 2 以降なら、いつやってもよい。上から順に進める(2026-10-05 に並べ直し)。

| # | やること | 効くもの | 規模 |
|---|---|---|---|
| 4-A | 眠っているプールを飛ばす : 最後に出してから「最大寿命」が経ったプールは、更新も描画もしない(CPU 側だけで判定できる) | P1・P2 | 小 |
| 4-B | アセットに `sortOrder` を持たせて安定ソートする | E2 | 小 |
| 4-C | Update で生きている粒を alive list に積み、間接引数を作って ExecuteIndirect で描く(コマンドシグネチャの仕組みから作る) | P1 | 中 |
| 4-D | 命令バッファをフレームで 1 本にまとめ(足りなければ伸ばす)、プールには `[offset, count]` を渡す。あわせて CPU 側で命令ごとの開始位置(prefix sum)を作り、Emit を 1 スレッド = 1 粒にする | P3・P4 | 中 |
| 4-E | 眠ったまま一定時間経ったプールを解放し、Capacity が変わったら作り直す(4-A の判定を流用) | P5 | 小〜中 |
| 4-F | 更新も alive list の数で間接 Dispatch にする(alive list を2本で回す。余力があれば) | P2 | 大 |

計測は PIX で取る(エンジンに GPU 時間の計測はまだ無い)。4-A の前後で取っておくと、以降の効き目が比べられる。

### 4-C の進め方(2段)

- **状態の約束** : バッファは ExecuteCommandLists が終わると COMMON に戻る(decay)。`GPUResource` は CPU 側で状態を覚えるだけで、戻ったことは知らない。
  そこで「フレームの頭は COMMON、使う前に明示で遷移、フレームの終わり(Submit の前)に明示で COMMON へ戻す」を守る(`StaticBuffer::UploadFrame` と同じ考え方)。
  暗黙の昇格(COMMON → UAV)に頼ったまま `Barrier(INDIRECT_ARGUMENT)` を呼ぶと、遷移前の状態が食い違う。
- **1段目(枠組み)** : 間接引数バッファ・コマンドシグネチャ・リセット用 CS・`RenderContext` の間接描画を作り、
  インスタンス数は今と同じ「容量」のまま ExecuteIndirect で描く。見た目は変わらないので、状態遷移と仕組みだけを確かめられる。
- **2段目(生存リスト)** : リセットでインスタンス数を 0 にし、Update が生き残った粒を alive list に積んで数える。VS は alive list 経由で粒を引く。
- 半透明の粒は描く順が毎フレーム入れ替わるようになる(アトミックで積むため)。ちらつくなら後で奥行きで並べ替える。

### 4-D の進め方(2段)

- **1段目(命令バッファを1本に)** : アセットごとの固定100件の命令バッファをやめ、全プールの命令をフレームごとに1本へ連結して送る。
  足りなければ伸ばす(作り直し + 古い方は遅延解放。EmitterSlotPool と同じ)。プールには `[offset, count]` を CB で渡す。
  シェーダーの変更は「命令を offset から読む」だけなので、見た目は変わらない。
- **2段目(1スレッド = 1粒)** : EmitterData の末尾に `emitStart`(そのプールの命令の中での累積の粒数)を足す(144 → 148 バイト)。
  CB で「その回に出す粒の合計」を渡し(容量で頭打ち)、シェーダーは命令を二分探索して「何番目の命令の何個目か」を求める。
  乱数の種は今と同じく(命令の番号, 何個目)から作るので、ばらつき方は変わらない。

### 4-E の進め方(3段)

調べて分かったこと(2026-10-05):
- **シーンの切れ目でプールが取り残されている。** `SweepUnusedAll` が参照の切れた ParticlesAsset を捨てると、そのハンドルは無効になり、
  プールは誰にも使われないまま残る(次に読み直したアセットは別のハンドルで別のプールを作る)。
  しかも 4-A の `IsAwake` は「アセットが引けないときは起きている」扱いなので、取り残されたプールは毎フレーム更新が回っている。
- **バッファは壊すだけではディスクリプタを返さない。** `GPUResource` のデストラクタは既定のままで、返すのは `Release()`。
  遅延解放のラムダで `shared_ptr` を抱えて壊すだけの箇所(EmitterSlotPool と命令バッファの作り直し、
  `GPUParticlePool::Init` の転送用バッファ3本 = DynamicBuffer は SRV を取る)は、そのたびにヒープの席が漏れている。

段取り:
1. **取り残されたプールを解放する**(アセットが引けず、ロード中でもないもの)+ `IsAwake` を「引けなければ眠っている」に + 遅延解放で `Release()` を呼ぶ
2. **Capacity が変わったら作り直す**(エディターで値を動かしている間は作り直さないよう、変わってから少し待つ)
3. **長く眠っているプールを解放する**(秒数は定数。次に使うときは作り直しで数フレーム遅れる)

---

## Phase 5 : EffectAsset の拡張(統一の前提条件)

これが揃わないうちに ParticlesComponent や音のコンポーネントを消すと、表現できないものが出る。

| # | やること | 吸収するもの |
|---|---|---|
| 5-1 | データと実行を分ける。EffectAsset は設計図だけにし、`Play/Update/Stop`・音の発行は実行側(EffectPlayer など)へ移す | S2 |
| 5-2 | パーツに**トリガー**を持たせる: `OnPlay` / `WhilePlaying` / `OnStop` | AudioBehavior の Start/Loop/End、火花、ブースターのスパーク(S3) |
| 5-3 | **個体ごとのパラメータ**: 名前付きの float を数個持ち、アセット側で「どこに効くか」(初速・大きさ・色・音量・ピッチ)を結び付ける。EffectOverride の決め打ちの欄を置き換える | E4、ブーストの太さ、チャージ量 |
| 5-4 | **1 エンティティに複数のエフェクト**を持たせる<br>・付いたまま出し続けるもの → 子エンティティ(ブースターは既にこの形)<br>・一発もの → `EffectEventsComponent`(OnSpawn / OnHit / OnDeath → EffectAsset)。発火したら SpawnEffectAt | S4、DeathEffect / HitSound / `isPlayOnSpawn` |
| 5-5 | 声は鳴らす直前に借りる。同じサウンドの最短間隔・同時発音数は **AudioManager 側**に持たせる(エフェクトをまたいで効かせるため) | S5、HitSound の minInterval |
| 5-6 | ライトのパーツを実装する(実装しないなら `EFFECT_POINTLIGHT_MAX` を消す) | S6 |

パーツ数の上限(`EFFECT_PARTICLE_MAX` などの固定長配列)は、コンポーネントに入れる都合で固定長のまま残す。上限が足りなくなったら値を上げる。

---

## Phase 6 : 移行と削除

**作業前に `Asset/` をバックアップすること**(git 管理外)。

| # | 移すもの | データ |
|---|---|---|
| 6-1 | ParticlesComponent → EffectAsset | 6 件(Missile / RazerBullet / Boss_01 / Player / Player_01 / Ex_Fier) |
| 6-2 | SoundComponent → EffectAsset のサウンドパーツ / `EffectEventsComponent` | 12 件 |
| 6-3 | HitSoundComponent → `EffectEventsComponent` の OnHit | 8 件 |
| 6-4 | AudioBehaviorComponent → トリガー付きのサウンドパーツ | 6 件 |
| 6-5 | FlyingSoundComponent → ミサイルのエフェクトに `WhilePlaying` のループ音として | 1 件 |
| 6-6 | DeathEffectComponent → `EffectEventsComponent` の OnDeath | 7 件 |
| 6-7 | ExplosionComponent(既に EffectAsset に置き換え済みで、残骸) | 1 件 |

**消すもの**: 上の各コンポーネント、`ParticleEmitSystem`・`EmitParticlesSystem`・`ParticleFixupSystem`、
`Sound*System` 群(BGM 用に残すものを除く)、`EEmitSpace`。

**型を消すときの注意**

- テキスト形式(`.oj*`)は未登録のコンポーネントを読み飛ばす(`Prefab.cpp:379`)ので、型を消しても読める
- バイナリ形式(`.ob*`)は登録順のタイプ ID に依存している可能性がある(ExplosionComponent のコメント)。
  消す前に、全アセットをテキストから保存し直してバイナリを作り直すこと

---

## 付録 : 作成中の EmitterSlotPool へのコメント(2026-10-05 時点)

方向性(全体共通の表・世代付きハンドル・足りなければ伸ばす)は合っている。変えた方がよい点:

1. **GPU バッファはブロックごとに分けず、1 本にする。**
   ブロックごとに別バッファにすると、粒ごとに「どのバッファを読むか」が変わる。
   頂点シェーダーと Update で `ResourceDescriptorHeap[NonUniformResourceIndex(...)]` が必要になり、
   ブロックの SRV 番号の表も別に渡さなければならない。
   行列は毎フレーム全部送り直すので、伸ばすときは「大きいバッファを作り直す(古いものは遅延解放)」だけで済み、
   中身のコピーも要らない。ブロックは CPU 側の「伸ばす単位」として残せば十分。
2. **HandlePool も 1 つにする。** `Create(0)` なら上限なしで伸びる。添字は uint16 なので上限は 65535 席。
3. **返却を遅らせる仕組みが要る。** `HandlePool::Remove` は即座に空きへ戻すので、
   そのまま使うと、まだ生きている粒の席を別の発生源が拾ってしまう。
   `ReserveReturn(handle, 秒)` で予約リストに積み、`BeginFrame` で期限切れのものだけ `Remove` する。
4. **席 0 を予約する。** HandlePool は最初に添字 0 を返すので、Init で確保して単位行列を書き、二度と返さないこと。
5. **HandlePool の世代は 0 始まり**なので、`Handle(0,0)` の id は 0 になる。
   リソース側の ItemPool は世代 1 始まりで、`id == 0` を無効扱いしている箇所がある(`ParticleBufferManager::RequestEmit`)。
   席のハンドルをそういう判定に通さないこと。GPU へ渡すときは `ToGPUIndex`(無効なら 0)を通す。
6. **GPU バッファは Upload の時点で作る。** `StaticStructuredBuffer::Create` はコマンドリストを要求し、
   `SetTransform` を呼ぶ ECS の Draw にはコマンドリストが無い。CPU 側だけ先に伸ばし、
   GPU 側は `UploadEmitData` の隣で容量を比べて作り直す。
   転送は `UploadFrame`(フレームごとの区画を経由して先頭から写す)で、使用最大までで足りる。
7. **`EmitterTransform` は HLSL 側と並びを合わせる**(`Particle.hlsli` に同名の構造体を置く)。
   将来、個体ごとの色(E4)を入れるならここに足す。容量を詰めたければ 3x4(48 バイト)にできる。
8. **呼ぶスレッドはメインに限る。** EffectDrawSystem は ActiveTask(メイン)、Release フックもメインで呼ばれる。
   Job から呼ぶようにするなら、そのときにロックを足す。
9. 今の下書きの `emitterSlotsBuffer.Create()` は引数が足りないので通らない(上の 6 で場所ごと変わる)。
   `GPUParticlePool.h` に空の `ParticlePool` クラスが追加されているが、使う予定が無ければ消しておく。
