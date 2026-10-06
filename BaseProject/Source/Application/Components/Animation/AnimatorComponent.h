#pragma once

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/EditorField/EditorField.inl"

#include "NodePoseComponent.h"
#include "SkeletonPoseComponent.h"
#include "Application/Components/Render/DynamicRaytracingComponent.h"
#include "Application/Components/Render/ModelComponent.h"

namespace Engine::Resource
{
	class AnimatorAsset;
}

namespace App::Component
{
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
		// かけるボーンレイヤー(モデルの BoneMask を名前のハッシュで引く)。0 なら全身。
		// 上に重ねるレイヤー(UpperAnimatorComponent)だけが使う。基本レイヤーは常に全身
		UINT boneMaskHash = 0;

		// ---- 設計図(保存するのは GUID だけ) ----
		Core::GUID animatorGUID = {};
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

	//----------------------------------------------------------------------------------
	// レイヤー1枚ぶんのエディター表示(AnimatorComponent / UpperAnimatorComponent で共通)
	// a_isMasked : ボーンレイヤー(かける範囲)を選ばせるか
	//----------------------------------------------------------------------------------
	inline void EditAnimatorLayer(Engine::ECS::CompEditContext& a_context, const char* a_label, AnimatorLayer& a_layer, bool a_isMasked)
	{
		using namespace Engine;
		const auto& _services = *a_context.pWorld->RefEngineServices();

		Engine::EditorField::IDScope _idScope(a_label);
		Engine::EditorField::HelpText(a_label);

		// 設計図の選択
		Engine::EditorField::AssetField<Resource::AnimatorAsset>(
			_services,
			"Animator",
			"AnimatorAsset",
			a_layer.animatorGUID,
			a_layer.animatorHandle
		);

		// 現在のステートを表示
		const auto* _pAnimator = _services.pResourceManager->Get(a_layer.animatorHandle);
		if (_pAnimator)
		{
			std::string _nodeNameStr(_pAnimator->GetNodeName(a_layer.currentStateHash));
			Engine::EditorField::Value("Current Node", "%s", _nodeNameStr.c_str());
		}

		// ボーンレイヤー : 自身のモデルが持つものから選ぶ
		if (a_isMasked)
		{
			const auto* _pModelComp = a_context.pWorld->RefData<ModelComponent>(a_context.entity);
			const auto* _pModel = _pModelComp ? _services.pResourceManager->Get(_pModelComp->handle) : nullptr;
			Engine::EditorField::ModelBoneMaskField("Bone Layer", _pModel, a_layer.boneMaskHash);
		}

		Engine::EditorField::Value("State Time", "%.2f s", a_layer.stateTime);
		Engine::EditorField::Value("Clip Time", "%.2f", a_layer.clipTime);
	}

	//==========================================================================================
	// AnimatorComponent
	//
	// アニメーションするモデルのアニメーター。
	//
	// ・「アニメーションするモデル」の目印も兼ねる(静的な描画系は Exclude<AnimatorComponent> で外す)。
	//   そのため、ポーズの置き場とレイトレ用インスタンスは必須コンポーネントとして一緒に付ける。
	// ・上半身など一部だけ別のアニメーションを重ねるときは UpperAnimatorComponent を足す
	//==========================================================================================
	struct AnimatorComponent
	{
		// 基本レイヤー : 全身にかかる
		AnimatorLayer baseLayer = {};
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::AnimatorComponent>
{
	// ポーズの置き場・レイトレ用インスタンスは実行中の値なので、プレハブに書かずに自動で付ける
	using Requires = Engine::ECS::RequireComponents<App::Component::NodePoseComponent, App::Component::SkeletonPoseComponent, App::Component::DynamicRaytracingComponent>;

	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData, const Engine::ECS::EngineServices& a_services)
	{
		App::Component::AnimatorComponent& _comp = Engine::EditorField::RefValue<App::Component::AnimatorComponent>(a_pData);
		a_services.pResourceManager->ReleaseHandle(_comp.baseLayer.animatorHandle);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::AnimatorComponent& _comp = Engine::EditorField::RefValue<App::Component::AnimatorComponent>(a_pData);

		if (a_ar.BeginGroup("baseLayer"))
		{
			a_ar.Field("animatorGUID", _comp.baseLayer.animatorGUID);
			a_ar.EndGroup();
		}
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::AnimatorComponent& _comp = Engine::EditorField::RefValue<App::Component::AnimatorComponent>(a_context.pData);

		// 基本レイヤーは全身にかかるので、ボーンレイヤーは選ばせない
		EditAnimatorLayer(a_context, "Base Layer", _comp.baseLayer, false);
	}
};
