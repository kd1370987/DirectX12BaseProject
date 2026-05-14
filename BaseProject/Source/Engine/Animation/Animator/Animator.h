#pragma once
namespace Engine::Animation
{
	struct ClipData
	{
		// 再生中の情報
		UINT m_clipID = 0;
		float m_time = 0.0f;
		float m_speed = 1.0f;

		// 繰り返しするか
		bool m_isLoop = false;
	};

	class Animator
	{
	public:

		// アニメーター作成
		void Create(Resource::Handle<Resource::Model> a_modelHandle);

		// アニメーション再生（再生中から強制的に切り替え）
		void Play(UINT a_clipID, float a_speed = 1.0f, bool a_isLoop = false);
		void Play(const std::string& a_clipName, float a_speed = 1.0f, bool a_isLoop = false);

		// 停止
		void Stop();

		// 更新
		void Update(float a_dt);

	private:

		// 現在再生中のアニメーションクリップ群
		std::vector<ClipData> m_clipDataVec = {};

		// 現在管理しているアニメーションモデル
		Resource::Handle<Resource::Model> m_modelHandle = {};

		// データ
		std::vector<DXSM::Matrix> m_nodePoseVec = {};	// ノードポーズ
		std::vector<DXSM::Matrix> m_boneVec = {};		// ボーン行列配列
	};
}