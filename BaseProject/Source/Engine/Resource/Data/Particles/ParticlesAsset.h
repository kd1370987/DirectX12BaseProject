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

		// 重力からの影響度
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

		// ---- ランタイム用データ ----
		ResourceRef<Texture> m_texHandle;
	};
}