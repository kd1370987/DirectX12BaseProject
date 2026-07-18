#include "StateGraphTypes.h"

namespace Engine::StateGraph
{
	//======================================================================================
	// Archive
	//======================================================================================
	void StateParameter::Archive(Persistence::Archive& a_arch)
	{
		a_arch.StringField("name", name);
		a_arch.Field("type", type);
		a_arch.Field("defaultFloat", defaultFloat);
		a_arch.Field("defaultInt", defaultInt);
		a_arch.Field("defaultBool", defaultBool);
	}

	void TransitionCondition::Archive(Persistence::Archive& a_arch)
	{
		a_arch.Field("paramHash", paramHash);
		a_arch.Field("op", op);
		a_arch.Field("thresholdFloat", thresholdFloat);
		a_arch.Field("thresholdInt", thresholdInt);
	}

	void TransitionArrow::Archive(Persistence::Archive& a_arch)
	{
		a_arch.Field("linkID", linkID);
		a_arch.Field("dstStartHash", dstStartHash);
		a_arch.Field("blendDuration", blendDuration);

		// 条件配列
		size_t _condSize = conditions.size();
		a_arch.Field("ConditionSize", _condSize);
		conditions.resize(_condSize);
		if (a_arch.BeginArray("Conditions", _condSize))
		{
			conditions.resize(_condSize);
			for (size_t _i = 0; _i < _condSize; ++_i)
			{
				if (a_arch.BeginObject(_i))
				{
					conditions[_i].Archive(a_arch);
					a_arch.EndObject();
				}
			}
			a_arch.EndArray();
		}
	}

	void TransitionArrow::EditArrow(int a_srcOutPinID, int a_dstInPinID) const
	{
		ImNodes::Link(linkID, a_srcOutPinID, a_dstInPinID);
	}

	void StateNodeBase::ArchiveTopology(Persistence::Archive& a_arch)
	{
		a_arch.StringField("nodeName", name);
		a_arch.Field("nodePos", editorPos);
		a_arch.Field("nodeID", nodeID);
		a_arch.Field("inPinID", inPinID);
		a_arch.Field("outPinID", outPinID);
	}

	//======================================================================================
	// 遷移評価(共有アルゴリズム)
	//
	// 挙動は既存の StateMachineAsset::EvaluateNextState /
	// GameFlowStateMachine::Evaluate と一致させている(抽出のみ、仕様変更なし)。
	//======================================================================================
	UINT EvaluateTransition(
		const std::unordered_map<UINT, std::vector<TransitionArrow>>& a_arrowMap,
		const std::unordered_map<UINT, StateParameter>& a_parameters,
		UINT a_currentHash,
		ParamSet& a_instance)
	{
		// 現在ステートから伸びる矢印のリスト
		auto _it = a_arrowMap.find(a_currentHash);
		if (_it == a_arrowMap.end()) return a_currentHash;	// 遷移先なし → 現状維持

		for (const auto& _arrow : _it->second)
		{
			// TODO: 条件なし矢印は現状スキップ(既存挙動を踏襲)。
			//       「アニメ終了で無条件に戻る」等を入れるなら、ここを見直す。
			if (_arrow.conditions.empty())
			{
				continue;
			}

			bool _allConditionsMet = true;
			for (const auto& _cond : _arrow.conditions)
			{
				// パラメータ定義
				auto _paramIt = a_parameters.find(_cond.paramHash);
				if (_paramIt == a_parameters.end())
				{
					_allConditionsMet = false;
					break;
				}

				const auto& _paramDef = _paramIt->second;
				bool _conditionMet = false;

				switch (_paramDef.type)
				{
				case EParamType::Float:
				{
					if (a_instance.floatParams.find(_cond.paramHash) == a_instance.floatParams.end())
					{
						a_instance.floatParams[_cond.paramHash] = _paramDef.defaultFloat;
					}
					float _val = a_instance.floatParams[_cond.paramHash];
					if (_cond.op == ECompareOp::Greater)		_conditionMet = (_val > _cond.thresholdFloat);
					else if (_cond.op == ECompareOp::Less)		_conditionMet = (_val < _cond.thresholdFloat);
					else if (_cond.op == ECompareOp::Equal)		_conditionMet = (_val == _cond.thresholdFloat);
					else if (_cond.op == ECompareOp::NotEqual)	_conditionMet = (_val != _cond.thresholdFloat);
					break;
				}
				case EParamType::Int:
				{
					if (a_instance.intParams.find(_cond.paramHash) == a_instance.intParams.end())
					{
						a_instance.intParams[_cond.paramHash] = _paramDef.defaultInt;
					}
					int _val = a_instance.intParams[_cond.paramHash];
					if (_cond.op == ECompareOp::Equal)			_conditionMet = (_val == _cond.thresholdInt);
					else if (_cond.op == ECompareOp::NotEqual)	_conditionMet = (_val != _cond.thresholdInt);
					else if (_cond.op == ECompareOp::Greater)	_conditionMet = (_val > _cond.thresholdInt);
					else if (_cond.op == ECompareOp::Less)		_conditionMet = (_val < _cond.thresholdInt);
					break;
				}
				case EParamType::Bool:
				case EParamType::Trigger: // Triggerの評価自体はBoolと同じ
				{
					if (a_instance.boolParams.find(_cond.paramHash) == a_instance.boolParams.end())
					{
						a_instance.boolParams[_cond.paramHash] = _paramDef.defaultBool;
					}
					bool _val = a_instance.boolParams[_cond.paramHash];
					if (_cond.op == ECompareOp::True)			_conditionMet = (_val == true);
					else if (_cond.op == ECompareOp::False)		_conditionMet = (_val == false);
					// UI側で誤ってEqual/NotEqualが選ばれた場合のフェイルセーフ
					else if (_cond.op == ECompareOp::Equal)		_conditionMet = (_val == true);
					else if (_cond.op == ECompareOp::NotEqual)	_conditionMet = (_val == false);
					break;
				}
				}

				if (!_conditionMet)
				{
					_allConditionsMet = false;
					break; // 1つでも満たさなければこの矢印は破棄
				}
			}

			if (_allConditionsMet)
			{
				// 使用したTriggerを消費(フラグを折る)
				for (const auto& _cond : _arrow.conditions)
				{
					auto _pIt = a_parameters.find(_cond.paramHash);
					if (_pIt != a_parameters.end() && _pIt->second.type == EParamType::Trigger)
					{
						a_instance.boolParams[_cond.paramHash] = false;
					}
				}
				return _arrow.dstStartHash;
			}
		}

		// どの条件も満たさなかった → 現状維持
		return a_currentHash;
	}
}
