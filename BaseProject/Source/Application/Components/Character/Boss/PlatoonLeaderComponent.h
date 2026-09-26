#pragma once

#include "Engine/Editor/Helper/EditorField.h"

//==========================================================================================
// PlatoonLeaderComponent
//
// リーダーに連なって追従する小隊長。自分の周りのボイド(BoidSpownerComponent で出す)を率いる。
//
// ・一つ前の相手(preLeader)は生成する側(SwarmBossController)が書き込むランタイム値。
//   先頭の小隊長はリーダーを、それ以降は一つ前の小隊長を指す。
//   エンティティIDはシーンを読み直すと変わるので保存しない。
// ・distance は前の相手との間隔。生成時の並べ方にも使う(リーダーの後ろへこの間隔で一列)。
// ・追従は PlatoonFollowSystem(Update)。目標地点は「前の相手の前方の逆側へ distance だけ
//   下がった点」なので、前の相手が曲がれば、その軌跡をなぞるように後ろへ付いていく。
//   座標を動かすのは MovementIntegrationSystem のまま。
// ・自分の向き(LookAngleComponent)は SwarmLookSystem が進んでいる向きへ turnSpeedDeg で寄せる。
//   体の向きにするのは既存の RotationSystem(Yaw のみ)。
//==========================================================================================
struct PlatoonLeaderComponent
{
	Engine::ECS::Entity preLeader = Engine::ECS::Limits::INVALID_ENTITY;	// 一つ前の小隊長(先頭はリーダー)
	float distance = 5.0f;			// 前の小隊長からの間隔(保存される)
	float followGain = 8.0f;		// 間隔のずれを詰める強さ(1/秒。ずれ1mにつき出す速さ。保存される)
	float turnSpeedDeg = 360.0f;	// 進んでいる向きへ向き直る速さ(度/秒。保存される)
	int platoonIndex = -1;			// リーダーから数えて何番目か(0 始まり。生成時に書き込む)

	float distanceAlongWorm = 0.0f;	// ワーム上での小隊長の位置 : 曲がっているなど関係のない純粋な距離、一次元、生成時に決まる
};

template<>
struct Engine::ECS::ComponentTraits<PlatoonLeaderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		PlatoonLeaderComponent& _comp = Engine::Editor::GetValue<PlatoonLeaderComponent>(a_pData);
		a_ar.Field("distance", _comp.distance);
		a_ar.Field("followGain", _comp.followGain);
		a_ar.Field("turnSpeedDeg", _comp.turnSpeedDeg);
	}

	static void Edit(CompEditContext& a_context)
	{
		PlatoonLeaderComponent& _comp = Engine::Editor::GetValue<PlatoonLeaderComponent>(a_context.pData);
		Engine::Editor::Field("Distance", _comp.distance, 0.1f, 0.0f);
		Engine::Editor::Field("FollowGain", _comp.followGain, 0.05f, 0.0f);
		Engine::Editor::Field("TurnSpeedDeg", _comp.turnSpeedDeg, 1.0f, 0.0f);
		Engine::Editor::HelpText("(max speed : MovementComponent.moveSpeed)");

		// 生成時に書き込まれる値なので表示のみ
		Engine::Editor::Text("PlatoonIndex : %d", _comp.platoonIndex);
		Engine::Editor::Text("AlongWorm    : %.1f m", _comp.distanceAlongWorm);
		if (_comp.preLeader == Engine::ECS::Limits::INVALID_ENTITY)
		{
			Engine::Editor::HelpText("PreLeader    : (none)");
		}
		else
		{
			Engine::Editor::Text("PreLeader    : %llu", static_cast<unsigned long long>(_comp.preLeader));
		}
	}
};
