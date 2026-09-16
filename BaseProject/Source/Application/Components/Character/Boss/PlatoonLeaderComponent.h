#pragma once

#include "Engine/Editor/Helper/EditorHelper.h"

//==========================================================================================
// PlatoonLeaderComponent
//
// リーダーに連なって追従する小隊長。自分の周りのボイド(BoidSpownerComponent で出す)を率いる。
//
// ・一つ前の相手(preLeader)は生成する側(SwarmBossController)が書き込むランタイム値。
//   先頭の小隊長はリーダーを、それ以降は一つ前の小隊長を指す。
//   エンティティIDはシーンを読み直すと変わるので保存しない。
// ・distance は前の相手との間隔。生成時の並べ方にも使う(リーダーの後ろへこの間隔で一列)。
// ・追従は PlatoonFollowSystem(Update)。前の相手の実速度に、間隔のずれ × followGain を
//   足したものを目標速度にする。座標を動かすのは MovementIntegrationSystem のまま。
//==========================================================================================
struct PlatoonLeaderComponent
{
	Engine::ECS::Entity preLeader = Engine::ECS::Limits::INVALID_ENTITY;	// 一つ前の小隊長(先頭はリーダー)
	float distance = 0.0f;			// 前の小隊長からの間隔(保存される)
	float followGain = 4.0f;		// 間隔のずれを詰める強さ(1/秒。ずれ1mにつき出す速さ。保存される)
	int platoonIndex = -1;			// リーダーから数えて何番目か(0 始まり。生成時に書き込む)
};

template<>
struct Engine::ECS::ComponentTraits<PlatoonLeaderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		PlatoonLeaderComponent& _comp = Engine::Editor::GetValue<PlatoonLeaderComponent>(a_pData);
		a_ar.Field("distance", _comp.distance);
		a_ar.Field("followGain", _comp.followGain);
	}

	static void Edit(CompEditContext& a_context)
	{
		PlatoonLeaderComponent& _comp = Engine::Editor::GetValue<PlatoonLeaderComponent>(a_context.pData);
		ImGui::DragFloat("Distance", &_comp.distance, 0.1f, 0.0f);
		ImGui::DragFloat("FollowGain", &_comp.followGain, 0.05f, 0.0f);
		ImGui::TextDisabled("(max speed : MovementComponent.moveSpeed)");

		// 生成時に書き込まれる値なので表示のみ
		ImGui::Text("PlatoonIndex : %d", _comp.platoonIndex);
		if (_comp.preLeader == Engine::ECS::Limits::INVALID_ENTITY)
		{
			ImGui::TextDisabled("PreLeader    : (none)");
		}
		else
		{
			ImGui::Text("PreLeader    : %llu", static_cast<unsigned long long>(_comp.preLeader));
		}
	}
};
