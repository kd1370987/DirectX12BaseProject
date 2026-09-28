#pragma once

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Editor/Helper/EditorField.inl"

#include "NodePoseComponent.h"
#include "SkeletonPoseComponent.h"
#include "Application/Components/Render/DynamicRaytracingComponent.h"
#include "Application/Components/Render/ModelComponent.h"

namespace Engine::Resource
{
	class AnimatorAsset;
}

//==========================================================================================
// AnimatorLayer
//
// アニメーター(設計図 = AnimatorAsset)1枚ぶんの実行中の状態。
//
// ・ステートの遷移は StateMachineCommitSystem、クリップの再生は AnimationSystem が進める。
// ・再生するクリップ・速さ・ループ・加算ポーズの効きは、今のステートのノード
//   (AnimatorAsset::GetStateNode(currentStateHash))から使う側がその都度引く。
//   以前は AnimationStateSystem が毎フレーム AnimatorComponent へ写していたが、
//   ノードを引けば分かる値の写しなので持たない。
//==========================================================================================
struct AnimatorLayer
{
	// かけるボーンレイヤー(モデルの BoneMask を名前のハッシュで引く)。0 なら全身
	UINT boneMaskHash = 0;

	// ---- 設計図(保存するのは GUID だけ) ----
	Engine::GUID animatorGUID = {};
	Engine::Handle<Engine::Resource::AnimatorAsset> animatorHandle = {};

	// ---- ステート ----
	UINT  prevStateHash    = 0;		// 前回のステート
	UINT  currentStateHash = 0;		// 現在のステート
	float stateTime        = 0.0f;	// 現在のステートに入ってからの経過時間(秒)

	// ---- クリップ ----
	float clipTime = 0.0f;

	// ---- 遷移の条件に使うパラメータの実体 ----
	Engine::Handle<Engine::Resource::StateMachineInstance> instanceHandle = {};
};

//==========================================================================================
// AnimatorComponent
//
// アニメーションするモデルのアニメーター
//==========================================================================================
struct AnimatorComponent
{
	// 基本レイヤー : 全身にかかる
	AnimatorLayer baseLayer = {};

	// 上半身にかかるレイヤー
	AnimatorLayer upperLayer = {};
	bool isLayering = false;			// アニメーションレイヤリングをするかどうか
};

template<>
struct Engine::ECS::ComponentTraits<AnimatorComponent>
{
	// ポーズの置き場・レイトレ用インスタンスは実行中の値なので、プレハブに書かずに自動で付ける
	using Requires = Engine::ECS::RequireComponents<NodePoseComponent, SkeletonPoseComponent, DynamicRaytracingComponent>;

	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData, const Engine::ECS::EngineServices& a_services)
	{
		AnimatorComponent& _comp = Engine::Editor::GetValue<AnimatorComponent>(a_pData);
		a_services.pResourceManager->ReleaseHandle(_comp.baseLayer.animatorHandle);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AnimatorComponent& _comp = Engine::Editor::GetValue<AnimatorComponent>(a_pData);

		if (a_ar.BeginGroup("baseLayer"))
		{
			a_ar.Field("animatorGUID", _comp.baseLayer.animatorGUID);
			a_ar.EndGroup();
		}
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		AnimatorComponent& _comp = Engine::Editor::GetValue<AnimatorComponent>(a_context.pData);
		AnimatorLayer& _layer = _comp.baseLayer;

		auto _LayerEditFunc = [&_comp,&a_context](const char* a_label,AnimatorLayer& a_layer) 
			{
				Engine::Editor::IDScope _idScope(a_label);
				Engine::Editor::HelpText(a_label);

				// 設計図の選択
				Engine::Editor::AssetField<Resource::AnimatorAsset>(
					*a_context.pWorld->RefEngineServices(),
					"Animator",
					"AnimatorAsset",
					a_layer.animatorGUID,
					a_layer.animatorHandle
				);

				// 現在のステートを表示
				const auto* _pAnimator = a_context.pWorld->RefEngineServices()->pResourceManager->Get(a_layer.animatorHandle);
				if (_pAnimator)
				{
					std::string _nodeNameStr(_pAnimator->GetNodeName(a_layer.currentStateHash));
					Engine::Editor::Value("Current Node", "%s", _nodeNameStr.c_str());
				}
				// ボーンレイヤー : 自身のモデルが持つものから選ぶ
				const auto* _pModelComp = a_context.pWorld->RefData<ModelComponent>(a_context.entity);
				const auto* _pModel = _pModelComp ? a_context.pWorld->RefEngineServices()->pResourceManager->Get(_pModelComp->handle) : nullptr;
				Engine::Editor::ModelBoneMaskField("Bone Layer", _pModel, a_layer.boneMaskHash);

				Engine::Editor::Value("State Time", "%.2f s", a_layer.stateTime);
				Engine::Editor::Value("Clip Time", "%.2f", a_layer.clipTime);

			};

		_LayerEditFunc("Base Layer",_comp.baseLayer);
		_LayerEditFunc("Upper Layer",_comp.upperLayer);
	}
};
