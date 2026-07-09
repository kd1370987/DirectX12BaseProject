#pragma once

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

		// エディターでの編集
		void EditImGui();

		// ---- アクセサ ----
		const std::string& GetName()const { return m_name; }				// パーティクル名
		const Engine::GUID& GetGUID() const { return m_guid; }				// パーティクルGUID
		Handle<Texture> GetTexHandle() const { return m_texHandle; }		// テクスチャハンドル
		float GetInitalSpeedMin() const { return m_initialSpeedMin; }		// 最小初速
		float GetInitalSpeedMax() const { return m_initialSpeedMax; }		// 最大初速
		float GetGravityPow() const { return m_gravityPow; }				// 重力影響度
		float GetLifeTimeMin() const { return m_lifeTimeMin; }				// 最小生存時間
		float GetLifeTimeMax() const { return m_lifeTimeMax; }				// 最大生存時間
		int GetCapacity() const { return m_capacity; }						// 最大生成数
		int GetEmissionRate() const { return m_emissionRate; }				// 発生レート


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
 
		// ---- ランタイム用データ ----
		ResourceRef<Texture> m_texHandle;
	};
}