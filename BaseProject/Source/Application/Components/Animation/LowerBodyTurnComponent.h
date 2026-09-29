#pragma once

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Editor/Helper/EditorField.inl"

#include "Application/Components/Render/ModelComponent.h"

//==========================================================================================
// LowerBodyTurnComponent
//
// 腰から下だけを進行方向へ向ける(戦車のような脚と上半身の分離)。
//
// ・機体(エンティティ)の向きは上半身の向き。脚の向きはここでワールドの Yaw として持ち、
//   機体との差の分だけ、ポーズを組んだ後に腰(pivot)を Y 軸まわりにひねる。
// ・腰の子の上半身の根元(counter)は同じだけ逆にひねり、上半身の向きを保つ。
// ・進行方向は目標速度(DesiredVelocityComponent)の水平成分。止まっている間は
//   最後に向いた方向のまま残す(機体だけが旋回する)。
// ・接地していない間(GroundStateComponent)は進行方向を追わず、機体の正面へ戻す。
// ・適用は LowerBodyTurnSystem(クリップ・加算ポーズの後、ワールド行列を組む前)。
//==========================================================================================
struct LowerBodyTurnComponent
{
	// ---- 設定(保存) ----
	// ノードは名前のハッシュで持つ(コンポーネントはトリビアルコピーできる型に限るので文字列は持てない)
	// ひねるノード(腰)。ここから下が脚として回る
	UINT pivotNodeHash = Engine::String::ToHash("pelvis");

	// 逆にひねって打ち消すノード(pivot の直下にある上半身の根元)
	UINT counterNodeHash = Engine::String::ToHash("spine_01");

	// 旋回の追従の強さ(1秒あたりの補間の強さ)
	float turnRate = 10.0f;

	// この速さ(水平)を超えたら進行方向とみなす
	float minMoveSpeed = 0.1f;

	// ---- 実行中の値(保存しない) ----
	float legYaw = 0.0f;			// 脚が向いているワールドの Yaw(ラジアン)
	bool  isLegYawValid = false;	// legYaw を初期化済みか(最初は機体の向きから始める)
};

template<>
struct Engine::ECS::ComponentTraits<LowerBodyTurnComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		LowerBodyTurnComponent& _comp = Engine::Editor::GetValue<LowerBodyTurnComponent>(a_pData);
		a_ar.Field("pivotNodeHash", _comp.pivotNodeHash);
		a_ar.Field("counterNodeHash", _comp.counterNodeHash);
		a_ar.Field("turnRate", _comp.turnRate);
		a_ar.Field("minMoveSpeed", _comp.minMoveSpeed);
	}

	// ハッシュで持っているノードを、モデルのノード一覧から選ぶ
	static void NodeField(const char* a_label, const Engine::Resource::Model* a_pModel, UINT& a_inoutHash)
	{
		UINT _nodeIdx = UINT_MAX;
		if (a_pModel)
		{
			const auto& _nodes = a_pModel->GetOriginalNodeVec();
			for (size_t _i = 0; _i < _nodes.size(); ++_i)
			{
				if (_nodes[_i].nodeNameHash == a_inoutHash) { _nodeIdx = static_cast<UINT>(_i); break; }
			}
		}
		Engine::Editor::ModelNodeField(a_label, a_pModel, _nodeIdx, a_inoutHash);
	}

	static void Edit(CompEditContext& a_context)
	{
		LowerBodyTurnComponent& _comp = Engine::Editor::GetValue<LowerBodyTurnComponent>(a_context.pData);

		// ノードは自身のモデルが持つものから選ぶ
		const auto* _pModelComp = a_context.pWorld->RefData<ModelComponent>(a_context.entity);
		const auto* _pModel = _pModelComp ? a_context.pWorld->RefEngineServices()->pResourceManager->Get(_pModelComp->handle) : nullptr;
		NodeField("Pivot Node", _pModel, _comp.pivotNodeHash);
		NodeField("Counter Node", _pModel, _comp.counterNodeHash);

		Engine::Editor::Slider("Turn Rate", _comp.turnRate, 0.0f, 30.0f, "%.1f");
		Engine::Editor::Slider("Min Move Speed", _comp.minMoveSpeed, 0.0f, 5.0f, "%.2f");

		Engine::Editor::Value("Leg Yaw", "%.1f deg", DirectX::XMConvertToDegrees(_comp.legYaw));
	}
};
