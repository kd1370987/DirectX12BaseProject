#pragma once

namespace Engine::Resource
{
	// 各ステートノード
	struct StateNode
	{
		// 識別名
		std::string	name;

		// アニメーションとの連携
		Handle<AnimationData> animationHandle;	// 再生するアニメーションのハンドル
		bool isLoopAnm = false;					// ループするかどうか
		float animSpeed = 1.0f;					// モーション速度

		// ロボットアクション用のパラメーター
		float moveSpeedMultiplier = 1.0f;		// 移動速度倍率
		bool hasArmor = false;					// スーパーアーマーがあるかどうか
	};

	// 遷移データ
	struct TransitionArrow
	{
		UINT dstStartHash;		// 遷移先のステートハッシュ
	};


	// ステートマシン全体の設計図
	// 実際のランタイムデータや運用時の設定は各コンポーネントが対応する
	class StateMachineAsset
	{
	public:

		StateMachineAsset() = default;
		~StateMachineAsset() = default;

		UINT GetStateHash(const std::string& a_stateName) const;

		// 保存と読み込み
		void Save(const std::string& a_savePath);
		void Load(const std::string& a_filePath);

		// 解放
		void Release();

		// エディターからの呼び出し用
		// ここで設計図を作る
		void EditImGui();

	private:
		// 初期ステート
		UINT m_defaultStartHash = 0;

		// 名前ハッシュ値、データ
		std::unordered_map<UINT, StateNode> m_stateNodeMap = {};

		// 名前ハッシュ値、変更先
		std::unordered_map<UINT, std::vector<TransitionArrow>> m_transitionArrowMap = {};

	};
}
