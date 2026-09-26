#pragma once

//==========================================================================================
// EmissiveOverrideComponent
//
// 実行中に発光(ModelComponent の emissiveColor / emissiveIntensity)を差し替えたいときの受け皿。
//
// ・書くのは演出側のシステム(今は BoidWaveSystem)。ModelComponent へ写すのは
//   ApplyEmissiveOverrideSystem(PreDraw)だけにしてある。
//   ModelComponent は描画・物理・銃など多くのシステムが読むので、演出側が直接書くと
//   その全員と書き込みがぶつかり、並べ替えも並列化もできなくなるため。
// ・isOverride が立つまでは何も写さない。プレハブに設定された発光がそのまま残る。
//   一度立てたら、最後に書いた値が残り続ける(以前 ModelComponent を直接書いていたときと同じ)。
// ・付けるのは差し替える側(ボイドなら SwarmBossController の生成時)。プレハブには入れない。
// ・実行中の値だけなので保存しない。
//==========================================================================================
struct EmissiveOverrideComponent
{
	Math::Vector3	emissiveColor		= { 1.0f, 1.0f, 1.0f };	// 発光色(0〜1)
	float			emissiveIntensity	= 0.0f;					// 発光の強さ
	bool			isOverride			= false;				// 差し替えるか(false なら何もしない)
};

template<>
struct Engine::ECS::ComponentTraits<EmissiveOverrideComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		EmissiveOverrideComponent& _comp = Engine::Editor::GetValue<EmissiveOverrideComponent>(a_context.pData);

		// 毎フレーム書き換わる値なので表示のみ
		Engine::Editor::Value("IsOverride", "%s", _comp.isOverride ? "true" : "false");
		Engine::Editor::Value("Emissive Color", "%.2f, %.2f, %.2f", _comp.emissiveColor.x, _comp.emissiveColor.y, _comp.emissiveColor.z);
		Engine::Editor::Value("Emissive Intensity", "%.2f", _comp.emissiveIntensity);
	}
};
