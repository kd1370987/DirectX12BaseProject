#pragma once

//==========================================================================================
// DynamicRaytracingComponent
//
// アニメーションするモデルのレイトレーシング用インスタンス
// (スキニング後の頂点と、毎フレーム作り直す BLAS)。
//
// ・確保は AnimationModelStartSystem、解放は AnimationMatrixFreeSystem。
// ・読むのは描画側(SkinningRegister / AnimationOptionalDraw / RegisterAnimatedRayWorld)。
// ・以前は AnimatorComponent が持っていた。アニメーターの状態とは関係のない描画用の資源なので分けた。
// ・AnimatorComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない。
//==========================================================================================
struct DynamicRaytracingComponent
{
	Engine::Handle<Engine::Raytracing::DynamicRaytracingData> dynamicInstanceHandle = {};
};

template<>
struct Engine::ECS::ComponentTraits<DynamicRaytracingComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		DynamicRaytracingComponent& _comp = Engine::Editor::GetValue<DynamicRaytracingComponent>(a_context.pData);
		Engine::Editor::HandleInfo(_comp.dynamicInstanceHandle);
	}
};
