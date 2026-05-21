#pragma once

namespace Engine::Resource
{
	// 各ステートノード
	struct StateNode
	{
		// 識別名
		std::string	name;
		UINT		nameHash;

		// アニメーションとの連携
		Handle<AnimationData> animationHandle;	// 再生するアニメーションのハンドル
		bool isLoopAnm = false;					// ループするかどうか
		float animSpeed = 1.0f;					// モーション速度

		// ロボットアクション用のパラメーター
		float moveSpeedMultiplier = 1.0f;		// 移動速度倍率
		bool hasArmor = 0.0f;					// スーパーアーマーがあるかどうか
	};

	// 遷移データ
	struct TransitionData
	{
		UINT dstStartHash;		// 遷移先のステートハッシュ
	};


	// ステートマシン全体の設計図
	class StateMachineAsset
	{
	public:

	private:
		// 初期ステート
		UINT m_defaultStartHash = 0;


	};
}
