#pragma once

#include "../../../Particle/Core/ParticleData.h"

namespace Engine::Resource
{

	/// <summary>
	/// パーティクルアセット : 
	/// データとして保存される。
	/// インスタンスからの参照を受けるため１パーティクルにつき一つ
	/// </summary>
	class ParticlesAsset
	{
	public:
		ParticlesAsset() = default;
		~ParticlesAsset() = default;
		NON_COPYABLE_MOVABLE(ParticlesAsset);

		// 作成処理
		void Create(const std::string& a_name, const Engine::GUID& a_guid);

		// 解放処理
		void Release();

		// シリアライズ
		void Save(const std::string& a_filePath);
		void Load(const std::string& a_fileDir, const std::string& a_fileName);
		void Load(const std::string& a_filePath);

	private:

		/// <summary>
		/// 保存と読み込みで共通の項目並び
		/// </summary>
		/// <remarks>
		/// 以前は Save と Load 2つに同じ並びを3回書いていて、
		/// 項目を足すときに片方だけ直すと読み書きがずれた。1箇所に寄せてある。
		/// ※ 追加は末尾に。バイナリは順次読みなので途中に挿すと既存データが全部ずれる
		/// </remarks>
		void Archive(Persistence::Archive& a_ar);

		/// <summary>
		/// 読み込み後の後始末(値の下限補正とテクスチャの解決)
		/// </summary>
		void OnLoaded();

	public:

		// ---- アクセサ ----
		const std::string& GetName()const { return m_name; }				// パーティクル名
		const Engine::GUID& GetGUID() const { return m_guid; }				// パーティクルGUID
		const Engine::GUID& GetTexGUID() const { return m_texGUID; }		// テクスチャGUID
		Handle<Texture> GetTexHandle() const { return m_texHandle; }		// テクスチャハンドル
		float GetInitalSpeedMin() const { return m_initialSpeedMin; }		// 最小初速
		float GetInitalSpeedMax() const { return m_initialSpeedMax; }		// 最大初速
		float GetGravityPow() const { return m_gravityPow; }				// 重力影響度
		float GetLifeTimeMin() const { return m_lifeTimeMin; }				// 最小生存時間
		float GetLifeTimeMax() const { return m_lifeTimeMax; }				// 最大生存時間
		int GetCapacity() const { return m_capacity; }						// 最大生成数
		int GetEmissionRate() const { return m_emissionRate; }				// 発生レート
		Particle::EParticleOrientation GetOrientation() const { return m_orientation; }	// 板ポリの向き
		float GetStretch() const { return m_stretch; }						// 進行方向への伸ばし倍率
		float GetDrag() const { return m_drag; }							// 速度の減衰
		float GetEndSizeScale() const { return m_endSizeScale; }			// 寿命の終わりでのサイズ倍率
		const Math::Color& GetStartColor() const { return m_startColor; }	// 発生時の色
		const Math::Color& GetEndColor() const { return m_endColor; }		// 消える直前の色
		float GetFadeInRatio() const { return m_fadeInRatio; }				// フェードインの割合
		float GetFadeOutRatio() const { return m_fadeOutRatio; }			// フェードアウトの割合
		Particle::EParticleBlendMode GetBlendMode() const { return m_blendMode; }	// 色の重ね方
		Particle::EParticleSimulationSpace GetSimulationSpace() const { return m_simulationSpace; }	// どの座標系で回すか
		bool IsLocalSpace() const { return m_simulationSpace == Particle::EParticleSimulationSpace::Local; }

		// ---- 編集用アクセサ : エディターから直接書き換えるためのもの ----
		std::string& RefName() { return m_name; }
		float& RefInitalSpeedMin() { return m_initialSpeedMin; }
		float& RefInitalSpeedMax() { return m_initialSpeedMax; }
		float& RefGravityPow() { return m_gravityPow; }
		float& RefLifeTimeMin() { return m_lifeTimeMin; }
		float& RefLifeTimeMax() { return m_lifeTimeMax; }
		int& RefCapacity() { return m_capacity; }
		int& RefEmissionRate() { return m_emissionRate; }
		Particle::EParticleOrientation& RefOrientation() { return m_orientation; }
		float& RefStretch() { return m_stretch; }
		float& RefDrag() { return m_drag; }
		float& RefEndSizeScale() { return m_endSizeScale; }
		Math::Color& RefStartColor() { return m_startColor; }
		Math::Color& RefEndColor() { return m_endColor; }
		float& RefFadeInRatio() { return m_fadeInRatio; }
		float& RefFadeOutRatio() { return m_fadeOutRatio; }
		Particle::EParticleBlendMode& RefBlendMode() { return m_blendMode; }
		Particle::EParticleSimulationSpace& RefSimulationSpace() { return m_simulationSpace; }

		// テクスチャの差し替え : GUIDとハンドルを同時に更新する
		void SetTexture(const Engine::GUID& a_guid, const ResourceRef<Texture>& a_handle)
		{
			m_texGUID = a_guid;
			m_texHandle = a_handle;
		}


	private:

		// ---- 識別子 ----
		std::string m_name;		// アセット名
		Engine::GUID m_guid;

		// ---- 静的データ ----
		// 参照データ
		Engine::GUID m_texGUID;		// テクスチャ

		// 初速
		float m_initialSpeedMin = 1.0f;
		float m_initialSpeedMax = 5.0f;

		// 重力からの影響度。
		// 1 で普通に落ち、0 で無重力、負にすると浮き上がる(煙・炎向き)
		float m_gravityPow = 0.0f;

		// 生存時間
		float m_lifeTimeMin = 0.5f;
		float m_lifeTimeMax = 2.0f;

		// 最大パーティクル発生数
		int m_capacity = 10000;

		// 発生レート / s
		int m_emissionRate = 0;

		// 板ポリの向き : 進行方向へ画像を回すかどうか
		Particle::EParticleOrientation m_orientation = Particle::EParticleOrientation::Billboard;

		// 進行方向への伸ばし倍率 : 1 で伸ばさない。速い火花や弾道を線にしたいときに上げる
		// (Billboard 指定のときは使わない)
		float m_stretch = 1.0f;

		//----------------------------------------------------------------------------------
		// 寿命に沿った変化
		//
		// 爆発は「速く飛び出して失速し、膨らみながら白→オレンジ→煙へ落ちて消える」。
		// 初速と寿命だけでは等速で飛び続けて唐突に消えるので、その間を埋めるための設定。
		//----------------------------------------------------------------------------------

		// 速度の減衰(1秒あたりの割合)。0 で減衰なし。
		// 大きいほど早く失速する。爆発の破片は 2〜5 あたりが目安
		float m_drag = 0.0f;

		// 寿命の終わりでのサイズ倍率。1 で変化なし。
		// 煙のように膨らむなら 1 より大きく、火花のように縮むなら小さく
		float m_endSizeScale = 1.0f;

		// 発生時と消える直前の色。
		// RGB は 1 を超えてよく、超えたぶんがブルームのしきい値を抜けて光って見える
		// (爆発の芯を白く飛ばしたいときは startColor を 3〜10 くらいに上げる)
		Math::Color m_startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Math::Color m_endColor = { 1.0f, 1.0f, 1.0f, 1.0f };

		// 寿命に対する割合でのフェード。0 でそのフェードをしない。
		// 割合で持つので、粒ごとに寿命がばらついても見え方が揃う
		float m_fadeInRatio = 0.0f;
		float m_fadeOutRatio = 0.25f;

		// 色の重ね方。光り物は加算、煙や破片は半透明。
		// 既定を加算にしてあるのは、これを入れる前が加算固定だったため
		// (既存のアセットの見え方を変えない)
		Particle::EParticleBlendMode m_blendMode = Particle::EParticleBlendMode::Additive;

		// どの座標系で回すか。
		// 既定をワールドにしてあるのは、これを入れる前がワールド固定だったため
		// (既存のアセットの見え方を変えない)。
		// ブースターの噴射のように発生源へくっついてほしいものは Local にする
		Particle::EParticleSimulationSpace m_simulationSpace = Particle::EParticleSimulationSpace::World;

		// ---- ランタイム用データ ----
		ResourceRef<Texture> m_texHandle;
	};
}