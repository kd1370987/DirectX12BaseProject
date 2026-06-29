#pragma once
namespace App::Game
{
	enum class EParamType { Float, Int, Bool, Trigger };

	// ステートノードが持つ遷移比較対象
	struct FlowStateParameter
	{
		// 遷移対象名(SpeedとかIsGroundとか)
		std::string name = "";
		UINT hash = 0;
		EParamType type;
		float defaultFloat = 0.0f;
		int defaultInt = 0;
		bool defaultBool = false;

		void Archive(Engine::Persistence::Archive& a_arch);
	};

	// 各ステートノード
	struct FlowStateNode
	{
		// 識別名
		UINT hash;
		std::string	name;

		// シーンアセットのGUID
		Engine::GUID sceneGUID = {};

		// エディター用情報
		DXSM::Vector2 editorPos = {};		// エディター上の位置情報
		int nodeID = 0;			// 自身のID
		int inPinID = 0;		// 自身の入り口
		int outPinID = 0;		// 自身の出口

		void Archive(Engine::Persistence::Archive& a_arch);
	};

	enum class ECompareOp { Greater, Less, Equal, NotEqual, True, False };
	// 遷移条件
	struct TransitionCondition
	{
		UINT paramHash;	// 比較対象のノードハッシュ
		ECompareOp op;		// 比較演算子

		// 比較する閾値
		float thresholdFloat = 0.0f;
		int thresholdInt = 0;
	};


	// 遷移データ
	struct FlowTransitionArrow
	{
		int linkID;
		UINT dstStartHash;		// 遷移先のステートハッシュ

		// 遷移条件
		std::vector<TransitionCondition> conditions;

		void Archive(Engine::Persistence::Archive& a_arch);
		void EditArrow(int a_srcOutPinID, int a_dstInPinID);
	};
}