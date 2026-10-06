#pragma once

namespace App::Component
{
	//==========================================================================================
	// BoidMembershipComponent
	//
	// ボイドがどの小隊に属しているか(小隊長のエンティティ)。
	//
	// ・書くのは SwarmBossController(生成時)だけ。システムは読むだけ
	//   (BoidSnapshotSystem / BoidSteeringSystem / SwarmLookSystem / BoidWaveSystem)。
	// ・以前は BoidComponent に操舵の設定と一緒に入っていた。
	// ・BoidSteeringParamsComponent の必須コンポーネントなので、プレハブに書かなくても付く。
	//   エンティティ ID は実行中だけ通じる値なので保存しない。
	//==========================================================================================
	struct BoidMembershipComponent
	{
		Engine::ECS::Entity platoonID = Engine::ECS::Limits::INVALID_ENTITY;	// 所属している小隊長
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::BoidMembershipComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::BoidMembershipComponent& _comp = Engine::EditorField::GetValue<App::Component::BoidMembershipComponent>(a_context.pData);

		if (_comp.platoonID == Engine::ECS::Limits::INVALID_ENTITY)
		{
			Engine::EditorField::HelpText("PlatoonID : (none)");
		}
		else
		{
			Engine::EditorField::Value("PlatoonID", "%llu", static_cast<unsigned long long>(_comp.platoonID));
		}
	}
};
