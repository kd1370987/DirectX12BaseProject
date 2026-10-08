#pragma once
#include "Engine/Graphics/Raytracing/DynamicRaytracingData.h"

namespace App::Component
{
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
		Engine::Handle<Engine::Graphics::Raytracing::DynamicRaytracingData> dynamicInstanceHandle = {};
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::DynamicRaytracingComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::DynamicRaytracingComponent& _comp = Engine::EditorField::RefValue<App::Component::DynamicRaytracingComponent>(a_context.pData);
		Engine::EditorField::HandleInfo(_comp.dynamicInstanceHandle);
	}
};
