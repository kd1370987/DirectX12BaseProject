#pragma once

#include "../../../Particle/Core/ParticleData.h"

namespace Engine::Resource
{
	//==========================================================================================
	// パーツの上限
	//
	// 実体側(EffectInstance)は ECS のコンポーネントに埋まる。
	// コンポーネントはチャンク間を memcpy で移動するので std::vector を持てず、
	// 進行状態は固定長配列で持つしかない。
	// そのためアセット側のパーツ数もここで頭打ちにして、両者の数を揃えている
	//==========================================================================================
	inline constexpr size_t EFFECT_PARTICLE_MAX = 4;
	inline constexpr size_t EFFECT_MESH_MAX = 4;

	//==========================================================================================
	// 発生源の取り方
	//
	// エフェクトは何かに付いて出るものなので、基準はいつも
	// 「付いている相手のワールド行列」。そこからどう取るかだけを選ぶ。
	//
	// ※ 値は保存されるので、増やすときは必ず末尾に足すこと
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
	//
	// 爆発のように「少し遅れて出る」「何秒かで終わる」を1つのアセットで表したいので、
	// パーツごとに開始の遅れと長さを持たせている。
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
	// パーティクル1件
	//
	// パーティクルアセット自体は「1粒がどう飛んでどう消えるか」(初速・寿命・重力・絵)を持つ。
	// こちらはそれを「どこから・どっちへ・どれだけ」出すかを持つ。
	// 同じアセットでも、噴射なら細く連続、爆発なら太く一発、と使い分けられる
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
	// メッシュ1件
	//
	// 噴射の芯や爆発の火球のように、パーティクルだけでは出せない
	// 「はっきりした形」を足すためのもの。
	//
	// 置きっぱなしのメッシュはエフェクトにならないので、
	// duration の間にスケールと不透明度を終値へ寄せていく仕掛けを持たせてある。
	// (duration が 0 のときは変化させず、開始時の値のまま出し続ける)
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
	// 実体側 : 再生するものが1つずつ持つ
	//
	// アセットはGUID単位で全員に共有されるので、進行状態をアセットへ持たせると
	// 同じエフェクトを使うエンティティ同士で再生位置を奪い合う。
	// そのため実体はここに分けて、使う側(コンポーネント)が持つ。
	//
	// ECS のコンポーネントに埋まるので POD であること(std::vector を持たせない)
	//==========================================================================================
	struct EffectInstance
	{
		bool  isPlaying = false;	// 再生中か
		float elapsed = 0.0f;		// 再生開始からの経過時間(秒)

		// パーティクルパーツごとの進行状態
		float rateAccum[EFFECT_PARTICLE_MAX] = {};	// 連続発生の端数繰り越し
		int   pendingEmit[EFFECT_PARTICLE_MAX] = {};// このフレームの発生数(Update が積み、Draw が消費)
		bool  wasEmitting[EFFECT_PARTICLE_MAX] = {};// バーストの立ち上がり検出用

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
		}
	};

	//==========================================================================================
	// エフェクト
	//
	// メッシュ形状やパーティクルをまとめて扱う。
	// 例) ジェット噴射されている中心は白い縦長のメッシュで、周りにパーティクルを散らす。
	//
	// 使う側は「再生する・止める」だけを伝えればよく、
	// 何個のパーティクルとメッシュで出来ているかを知らなくてよい。
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

		// 編集用 : エディターから直接書き換える
		std::vector<EffectParticlePart>& RefParticleParts() { return m_particleParts; }
		std::vector<EffectMeshPart>& RefMeshParts() { return m_meshParts; }

		// 追加・削除 : 上限を超えないようにここを通す
		bool AddParticlePart();
		bool AddMeshPart();
		void RemoveParticlePart(size_t a_index);
		void RemoveMeshPart(size_t a_index);

		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& a_name) { m_name = a_name; }

		//--------------------------------------------------------------------
		// 保存・読み込み
		//--------------------------------------------------------------------
		void Archive(Persistence::Archive& a_ar);
		void Save(const std::string& a_baseFilePath);

		/// <summary>
		/// GUIDから参照アセット(パーティクル・モデル)のハンドルを解決する
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

		// 頭から再生する
		void Play(EffectInstance& a_inst) const;

		// 止める。出ている途中のパーティクルはそのまま寿命で消える
		void Stop(EffectInstance& a_inst) const;

		/// <summary>
		/// 時間を進めて、このフレームの発生数を決める
		/// </summary>
		/// <remarks>
		/// 実フレーム時間が要るので Update フェーズで呼ぶこと。
		/// 実際の発生要求は Draw フェーズ側が pendingEmit を見て行う
		/// </remarks>
		void Update(EffectInstance& a_inst, float a_dt) const;

		/// <summary>
		/// 全パーツが出し終わったか
		/// </summary>
		/// <remarks>
		/// 出しっぱなし(duration = 0)のパーツが1つでもあれば、いつまでも false。
		/// 単発エフェクトの後片付け(自分を消す)の判断に使う
		/// </remarks>
		bool IsFinished(const EffectInstance& a_inst) const;

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

		// 識別子
		std::string m_name;

		// パーツ
		std::vector<EffectParticlePart> m_particleParts;
		std::vector<EffectMeshPart> m_meshParts;
	};
}
