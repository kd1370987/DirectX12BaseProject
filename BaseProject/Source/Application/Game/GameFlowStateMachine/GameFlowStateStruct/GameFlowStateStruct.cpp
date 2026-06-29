#include "GameFlowStateStruct.h"

namespace App::Game
{
	void FlowStateParameter::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("name", name);
		a_arch.Field("type", type);
		a_arch.Field("defaultFloat", defaultFloat);
		a_arch.Field("defaultInt", defaultInt);
		a_arch.Field("defaultBool", defaultBool);
	}

	void FlowStateNode::Archive(Engine::Persistence::Archive& a_arch)
	{
		// ノード名
		a_arch.StringField("nodeName", name);

		// ノード位置
		a_arch.Field("nodePos", editorPos);

		// シーンアセットの保存
		a_arch.Field("sceneGUID",sceneGUID);

		// ノード管理ID
		a_arch.Field("nodeID", nodeID);
		a_arch.Field("inPinID", inPinID);
		a_arch.Field("outPinID", outPinID);
	}

	void FlowTransitionArrow::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("linkID", linkID);
		a_arch.Field("dstStartHash", dstStartHash);

		// 配列開始
		size_t _condSize = conditions.size();
		a_arch.Field("ConditionSize", _condSize);
		conditions.resize(_condSize);
		if (a_arch.BeginArray("Conditions", _condSize))
		{
			conditions.resize(_condSize);
			for (size_t _i = 0; _i < _condSize; ++_i)
			{
				// オブジェクト開始
				if (a_arch.BeginObject(_i))
				{
					auto& _cond = conditions[_i];
					a_arch.Field("paramHash", _cond.paramHash);
					a_arch.Field("op", _cond.op);
					a_arch.Field("thresholdFloat", _cond.thresholdFloat);
					a_arch.Field("thresholdInt", _cond.thresholdInt);
					a_arch.EndObject();
				}
			}
			a_arch.EndArray();
		}
	}

	void FlowTransitionArrow::EditArrow(int a_srcOutPinID, int a_dstInPinID)
	{
		// 引数で受け取ったピンIDをそのまま渡す
		ImNodes::Link(
			linkID,
			a_srcOutPinID,  // 出力元のピンID
			a_dstInPinID    // 入力先のピンID
		);
	}
}