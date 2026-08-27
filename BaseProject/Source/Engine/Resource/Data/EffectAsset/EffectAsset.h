#pragma once

#include "../../../Particle/Core/ParticleData.h"

namespace Engine::Audio
{
	class AudioManager;
}

namespace Engine::Resource
{
	//==========================================================================================
	// パーツの上限 : コンポーネント側で使われるため、固定長の要素数
	//==========================================================================================
	inline constexpr size_t EFFECT_PARTICLE_MAX = 4;
	inline constexpr size_t EFFECT_MESH_MAX = 4;
	inline constexpr size_t EFFECT_SOUND_MAX = 4;
	inline constexpr size_t EFFECT_POINTLIGHT_MAX = 1;

	//==========================================================================================
	// 発生源の取り方 : エフェクトがついているものにたいして基準を選ぶ 末尾に追加必須
	//==========================================================================================
	enum class EEffectSpace : uint32_t
	{
		LocalOffset,		// 相手の行列基準でオフセット/方向を合成する(既定。ノズル位置など)
		WorldMatrix,		// 相手の位置と前方(+Z)をそのまま使う
		ReverseVelocity,	// 位置は行列基準、向きは進行方向の逆(噴射・排気)。
							// 見た目の姿勢と進行方向が一致しない弾やミサイル向け
	};

	const char* ToString(EEffectSpace a_space);

	//==========================================================================================
	// パーツ共通の時間指定
	//==========================================================================================
	struct EffectTiming
	{
		float startDelay = 0.0f;	// 再生開始からこの秒数だけ待ってから出る
		float duration = 0.0f;		// 出している長さ(秒)。0 なら止めるまで出しっぱなし

		// 経過時間から見て、今出している最中か
		bool IsActiveAt(float a_elapsed) const
		{
			if (a_elapsed < startDelay) return false;
			if (duration <= 0.0f) return true;	// 出しっぱなし
			return a_elapsed < (startDelay + duration);
		}

		// 出し終わっているか(0〜1のうち、終わりまで来たか)
		bool IsFinishedAt(float a_elapsed) const
		{
			if (duration <= 0.0f) return false;	// 出しっぱなしは終わらない
			return a_elapsed >= (startDelay + duration);
		}

		// 出している区間のどこまで進んだか(0〜1)。出しっぱなしのときは常に 0
		float GetProgressAt(float a_elapsed) const
		{
			if (duration <= 0.0f) return 0.0f;
			const float _t = (a_elapsed - startDelay) / duration;
			return std::clamp(_t, 0.0f, 1.0f);
		}

		void Archive(Persistence::Archive& a_ar);
	};

	//==========================================================================================
	// パーティクル
	//==========================================================================================
	struct EffectParticlePart
	{
		// ---- 何を出すか ----
		Engine::GUID particleGUID = Engine::DefaultGUID;
		Handle<ParticlesAsset> particleHandle = {};		// ランタイム用(読み込み時に解決)

		// ---- どこから出すか ----
		EEffectSpace space = EEffectSpace::LocalOffset;
		Math::Vector3 posOffset = { 0.0f, 0.0f, 0.0f };	// 相手の行列基準の発生位置
		Math::Vector3 emitDir = { 0.0f, 0.0f, 1.0f };	// 相手の行列基準の発生方向

		// ---- どっちへ出すか ----
		// Cone       : emitDir を軸にした円錐(directionAngle が広がり)。噴射・排気
		// Sphere     : 中心から全方向へ均等。爆発
		// Hemisphere : emitDir 側の半球だけ。地面での爆発
		// ※ Cone の角度を 360 にしても全方向にはならない(円錐の半頂角なので)。
		//    四方八方へ飛び散らせたいときは Sphere を選ぶこと
		Particle::EParticleEmitShape emitShape = Particle::EParticleEmitShape::Cone;

		// ---- どれだけ出すか ----
		int   emitCount = 8;		// 1回の発生数
		float emitRate = 0.0f;		// >0 : 毎秒この回数だけ発生 / 0 : 開始時に1回だけ(バースト)

		// ---- いつ出すか ----
		EffectTiming timing = {};

		// ---- 散らばり方 ----
		// 1粒の速度・寿命はアセット側。ここは「どれくらいの範囲にどう散らすか」
		float baseScale = 1.0f;			// 全体スケール
		float minScale = 0.1f;			// 1粒のスケール下限
		float maxScale = 1.0f;			// 1粒のスケール上限
		float positionRadius = 0.5f;	// 発生位置のばらつき半径
		float directionAngle = 10.0f;	// 発生方向のばらつき(度)。Cone のときだけ効く

		bool IsValid() const { return particleGUID != Engine::DefaultGUID; }

		void Archive(Persistence::Archive& a_ar);
	};

	//==========================================================================================
	// メッシュ : 芯の表現などはっきりしたもの
	//==========================================================================================
	struct EffectMeshPart
	{
		// ---- 何を出すか ----
		Engine::GUID modelGUID = Engine::DefaultGUID;
		Handle<Model> modelHandle = {};					// ランタイム用(読み込み時に解決)

		// ---- どこに出すか : 相手の行列基準のローカル配置 ----
		Math::Vector3 posOffset = { 0.0f, 0.0f, 0.0f };
		Math::Vector3 rotation = { 0.0f, 0.0f, 0.0f };	// オイラー角(度)
		Math::Vector3 scale = { 1.0f, 1.0f, 1.0f };

		// ---- いつ出すか ----
		EffectTiming timing = {};

		// ---- 見た目 ----
		Math::Color colorScale = { 1.0f, 1.0f, 1.0f, 1.0f };
		Math::Vector3 emissiveColor = { 1.0f, 1.0f, 1.0f };	// 発光色(0〜1)
		float emissiveIntensity = 0.0f;						// 発光の強さ(0でオフ)。
															// ブルームのしきい値を超えると光って見える

		// ---- 時間で変える終値 ----
		Math::Vector3 endScale = { 1.0f, 1.0f, 1.0f };	// duration の終わりでのスケール倍率
		float endAlpha = 1.0f;							// duration の終わりでの不透明度
		float endEmissiveIntensity = 0.0f;				// duration の終わりでの発光の強さ

		bool IsValid() const { return modelGUID != Engine::DefaultGUID; }

		void Archive(Persistence::Archive& a_ar);
	};

	//==========================================================================================
	// サウンド : 座標はエフェクトの中心
	//==========================================================================================
	struct EffectSoundPart
	{
		// ---- 何を鳴らすか ----
		Engine::GUID soundGUID = Engine::DefaultGUID;
		Handle<Sound> soundHandle = {};	// ランタイム用(読み込み時に解決)。
										// 鳴らす瞬間に波形の読み込みが走らないよう先に持っておく

		// ---- いつ鳴らすか ----
		// startDelay : 再生開始から何秒後に鳴らし始めるか
		// duration   : ループ音を鳴らし続ける長さ(0 なら止めるまで)。
		//              単発音(isLoop = false)では使わない
		EffectTiming timing = {};

		// ---- どう鳴らすか ----
		float vol = 1.0f;			// 音量。1 で 100%
		bool  isLoop = false;		// 鳴りっぱなしにするか(噴射音など)。エフェクトを止めれば止まる
		bool  is3DSound = true;		// 位置による定位・距離減衰を付けるか。false なら常に同じ音量で鳴る
		float distanceScaler = 1.0f;// 3D の減衰倍率。大きいほど遠くまで届く(3D のときだけ効く)

		// ---- 出し切ったかの判定に混ぜるか ----
		// true : この音が鳴り終わるまで「エフェクトは終わっていない」とみなす。
		//        destroyOnFinish のエフェクト(単発の爆発など)で、絵が消えた瞬間に
		//        エンティティごと消えて音が途切れるのを防ぐ。
		// false: 音の長さを見ない。絵が終わればエフェクトも終わる
		bool isWaitFinish = true;

		bool IsValid() const { return soundGUID != Engine::DefaultGUID; }

		void Archive(Persistence::Archive& a_ar);
	};

	//==========================================================================================
	// 実体側 : 再生するものが1つずつ持つ、コンポーネントに持たせる
	//==========================================================================================
	struct EffectInstance
	{
		bool  isPlaying = false;	// 再生中か
		float elapsed = 0.0f;		// 再生開始からの経過時間(秒)

		// パーティクルパーツごとの進行状態
		float rateAccum[EFFECT_PARTICLE_MAX] = {};	// 連続発生の端数繰り越し
		int   pendingEmit[EFFECT_PARTICLE_MAX] = {};// このフレームの発生数(Update が積み、Draw が消費)
		bool  wasEmitting[EFFECT_PARTICLE_MAX] = {};// バーストの立ち上がり検出用

		//------------------------------------------------------------------
		// サウンドパーツごとの進行状態
		//
		// ハンドルは「借りている声」なので Reset() では消さないこと。
		// 消すと返却先が分からなくなって、鳴りっぱなしの声がプールに残る。
		// 発行は CreateSoundInstances / 返却は ReleaseSounds の担当
		//------------------------------------------------------------------
		Handle<SoundInstance> soundHandles[EFFECT_SOUND_MAX] = {};
		bool  soundTriggered[EFFECT_SOUND_MAX] = {};	// もう鳴らしたか(単発を1回だけにする)
		Math::Vector3 soundPos = { 0.0f, 0.0f, 0.0f };	// 3D 指定のパーツを鳴らす位置

		// その声を何から発行したか。
		// アセット側の指定と食い違っていたら作り直す(SyncSoundInstances)。
		// エディターで音や 3D 指定を差し替えたとき、
		// すでに出ているエフェクトにも次のフレームから効かせるためのもの
		Engine::GUID soundSourceGUID[EFFECT_SOUND_MAX] = {};
		bool         soundSource3D[EFFECT_SOUND_MAX] = {};

		// 頭から再生し直す
		void Reset()
		{
			elapsed = 0.0f;
			for (size_t _i = 0; _i < EFFECT_PARTICLE_MAX; ++_i)
			{
				rateAccum[_i] = 0.0f;
				pendingEmit[_i] = 0;
				wasEmitting[_i] = false;
			}
			for (size_t _i = 0; _i < EFFECT_SOUND_MAX; ++_i)
			{
				soundTriggered[_i] = false;
			}
		}

		//------------------------------------------------------------------
		// 発行済みインスタンスに対しての操作
		//
		// どれもアセットの中身を見ないので、
		// アセットが読めていない・破棄された後でも呼べる(AudioBehaviorInstance と同じ)
		//------------------------------------------------------------------

		// 3D再生の位置を更新する。鳴っている音にも即時反映される
		void SetSoundPos(Engine::Audio::AudioManager& a_audioManager, const Math::Vector3& a_pos);

		// 発行済みインスタンスをすべて返却して空にする
		void ReleaseSounds(Engine::Audio::AudioManager& a_audioManager);
	};

	//==========================================================================================
	// エフェクト
	//
	// メッシュ形状やパーティクル、鳴らす音をまとめて扱う。
	// 例) ジェット噴射されている中心は白い縦長のメッシュで、周りにパーティクルを散らす。
	//     爆発なら、粒と火球に「ドン」という音まで込みで1枚。
	//
	// 使う側は「再生する・止める」だけを伝えればよく、
	// 何個のパーティクルとメッシュと音で出来ているかを知らなくてよい。
	// (音だけ別に鳴らしに行かなくてよい、というのがサウンドパーツの狙い)
	//==========================================================================================
	class EffectAsset
	{
	public:
		EffectAsset() = default;
		explicit EffectAsset(const std::string& a_name) : m_name(a_name) {}
		~EffectAsset() = default;
		NON_COPYABLE_MOVABLE(EffectAsset);

		//--------------------------------------------------------------------
		// 定義へのアクセス
		//--------------------------------------------------------------------
		const std::vector<EffectParticlePart>& GetParticleParts() const { return m_particleParts; }
		const std::vector<EffectMeshPart>& GetMeshParts() const { return m_meshParts; }
		const std::vector<EffectSoundPart>& GetSoundParts() const { return m_soundParts; }

		// 編集用 : エディターから直接書き換える
		std::vector<EffectParticlePart>& RefParticleParts() { return m_particleParts; }
		std::vector<EffectMeshPart>& RefMeshParts() { return m_meshParts; }
		std::vector<EffectSoundPart>& RefSoundParts() { return m_soundParts; }

		// 追加・削除 : 上限を超えないようにここを通す
		bool AddParticlePart();
		bool AddMeshPart();
		bool AddSoundPart();
		void RemoveParticlePart(size_t a_index);
		void RemoveMeshPart(size_t a_index);
		void RemoveSoundPart(size_t a_index);

		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& a_name) { m_name = a_name; }

		//--------------------------------------------------------------------
		// 保存・読み込み
		//--------------------------------------------------------------------
		void Archive(Persistence::Archive& a_ar);
		void Save(const std::string& a_baseFilePath);

		/// <summary>
		/// GUIDから参照アセット(パーティクル・モデル・サウンド)のハンドルを解決する
		/// </summary>
		/// <remarks>
		/// ロード直後と、エディターで差し替えた後に呼ぶ。
		/// ロードはリソースマネージャー経由なので、アセット単体では解決できない
		/// </remarks>
		void ResolveReferences();

		//--------------------------------------------------------------------
		// 再生
		//
		// 鳴らす対象は使う側が持っている実体(EffectInstance)。
		//--------------------------------------------------------------------

		/// <summary>
		/// 定義に沿って再生用のサウンドインスタンスを発行する
		/// </summary>
		/// <remarks>
		/// すでに発行済みなら一度返してから作り直すので、二重発行にはならない。
		/// 音を設定していないパーツのハンドルは無効のままになる。
		/// 鳴らす瞬間に読み込みが走らないよう、生成時に呼んでおくこと(EffectFixupSystem)
		/// </remarks>
		void CreateSoundInstances(Engine::Audio::AudioManager& a_audioManager, EffectInstance& a_inst) const;

		// 頭から再生する
		void Play(EffectInstance& a_inst) const;

		/// <summary>
		/// 止める。出ている途中のパーティクルはそのまま寿命で消える
		/// </summary>
		/// <param name="a_pAudioManager">
		/// 渡すと鳴っている音も止める。null なら音はそのまま鳴り続ける
		/// (単発音を最後まで鳴らしたい場合)
		/// </param>
		void Stop(EffectInstance& a_inst, Engine::Audio::AudioManager* a_pAudioManager = nullptr) const;

		/// <summary>
		/// 時間を進めて、このフレームの発生数を決める。時間が来たサウンドもここで鳴らす
		/// </summary>
		/// <param name="a_pAudioManager">null ならサウンドパーツは鳴らさない</param>
		/// <remarks>
		/// 実フレーム時間が要るので Update フェーズで呼ぶこと。
		/// 実際の発生要求は Draw フェーズ側が pendingEmit を見て行う
		/// </remarks>
		void Update(
			EffectInstance& a_inst,
			float a_dt,
			Engine::Audio::AudioManager* a_pAudioManager = nullptr) const;

		/// <summary>
		/// 全パーツが出し終わったか
		/// </summary>
		/// <param name="a_pAudioManager">
		/// 渡すと isWaitFinish のサウンドが鳴り終わるまで false を返す。
		/// null ならサウンドは見ない
		/// </param>
		/// <remarks>
		/// 出しっぱなし(duration = 0)のパーツが1つでもあれば、いつまでも false。
		/// 単発エフェクトの後片付け(自分を消す)の判断に使う
		/// </remarks>
		bool IsFinished(
			const EffectInstance& a_inst,
			Engine::Audio::AudioManager* a_pAudioManager = nullptr) const;

		/// <summary>
		/// メッシュパーツの今フレームの描画情報を作る
		/// </summary>
		/// <param name="a_ownerWorld">エフェクトが付いている相手のワールド行列</param>
		/// <returns>今出していないパーツなら false(描画しない)</returns>
		bool BuildMeshDraw(
			size_t a_index,
			const EffectInstance& a_inst,
			const Math::Matrix& a_ownerWorld,
			Math::Matrix& a_outWorld,
			Math::Color& a_outColorScale,
			Math::Vector3& a_outEmissiveAdd
		) const;

	private:

		/// <summary>
		/// 発行済みの声を、今のアセットの指定に合わせ直す
		/// </summary>
		/// <remarks>
		/// 食い違っているスロットだけ発行し直すので、毎フレーム呼んでよい。
		/// 3D かどうかは声を発行するときにしか決められないため、
		/// エディターで音や 3D 指定を差し替えたときは、ここが作り直しの受け口になる
		/// </remarks>
		void SyncSoundInstances(Engine::Audio::AudioManager& a_audioManager, EffectInstance& a_inst) const;

	private:

		// 識別子
		std::string m_name;

		// パーツ
		std::vector<EffectParticlePart> m_particleParts;
		std::vector<EffectMeshPart> m_meshParts;
		std::vector<EffectSoundPart> m_soundParts;
	};
}
