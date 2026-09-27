#pragma once

//==========================================================================================
// BoidTargetComponent
//
// ボイドが向かう目標地点。書くのは FollowLeaderSystem(追従先の位置)、
// 読むのは BoidSteeringSystem(Seek)。
//
// ・以前は BoidComponent::targetPos として操舵の設定と一緒に入っていた。
//   毎フレーム書かれる値と、読むだけの設定が同じ型だと、読むだけの側まで書き手とぶつかる。
// ・BoidSteeringParamsComponent の必須コンポーネントなので、プレハブに書かなくても付く。
//   毎フレーム書き直すので保存しない。
//==========================================================================================
struct BoidTargetComponent
{
	Math::Vector3 targetPos = { 0.0f, 0.0f, 0.0f };	// 目標地点(ワールド)
};

template<>
struct Engine::ECS::ComponentTraits<BoidTargetComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		BoidTargetComponent& _comp = Engine::Editor::GetValue<BoidTargetComponent>(a_context.pData);

		// 毎フレーム書き直される値なので表示のみ
		Engine::Editor::Value("TargetPos", "%.2f, %.2f, %.2f", _comp.targetPos.x, _comp.targetPos.y, _comp.targetPos.z);
	}
};
