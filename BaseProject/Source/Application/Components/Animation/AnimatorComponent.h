#pragma once

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Editor/Helper/EditorField.inl"

#include "NodePoseComponent.h"
#include "SkeletonPoseComponent.h"
#include "Application/Components/Render/DynamicRaytracingComponent.h"

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
	// ---- 設計図(保存するのは GUID だけ) ----
	Engine::GUID animatorGUID = {};
	Engine::Handle<Engine::Resource::AnimatorAsset> animatorHandle = {};

	// ---- ステート ----
	UINT  prevStateHash    = 0;		// 前回のステート
	UINT  currentStateHash = 0;		// 現在のステート
	float stateTime        = 0.0f;	// 現在のステートに入ってからの経過時間(秒)

	// ---- クリップ ----
	// 再生位置(クリップの時間単位。dt × ステートの speed で進む)。
	// ステートが変わったら 0 に戻す(StateMachineCommitSystem)
	float clipTime = 0.0f;

	// ---- 遷移の条件に使うパラメータの実体 ----
	// プール(ItemPool<StateMachineInstance>)に置く。
	// 確保は StateMachineFixupSystem、返すのは AnimatorFreeSystem
	Engine::Handle<Engine::Resource::StateMachineInstance> instanceHandle = {};
};

//==========================================================================================
// AnimatorComponent
//
// アニメーションするモデルのアニメーター。
//
// ・今は土台のレイヤー(baseLayer)1枚だけ。上に重ねるレイヤーは持っていない。
// ・以前は StateMachineComponent(設計図とステート)と AnimatorComponent(そのステートの
//   クリップの写しと再生位置)の2つに分かれていた。
// ・「アニメーションするモデル」の目印も兼ねる(静的な描画系は Exclude<AnimatorComponent> で外す)。
//   そのため、ポーズの置き場とレイトレ用インスタンスは必須コンポーネントとして一緒に付ける。
//==========================================================================================
struct AnimatorComponent
{
	AnimatorLayer baseLayer = {};
};

template<>
struct Engine::ECS::ComponentTraits<AnimatorComponent>
{
	// ポーズの置き場・レイトレ用インスタンスは実行中の値なので、プレハブに書かずに自動で付ける
	using Requires = Engine::ECS::RequireComponents<NodePoseComponent, SkeletonPoseComponent, DynamicRaytracingComponent>;

	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//
	// コンポーネントはデストラクタが走らないので、参照を返すのはここの仕事。
	// ECS がエンティティを消すとき・コンポーネントを外すとき・
	// PostDeserialize へ入り直すとき(fixup が取り直す)に必ず呼ぶ。
	// (パラメータの実体はワールドのプールにあるので、ここではなく AnimatorFreeSystem が返す)
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

		Engine::Editor::HelpText("Base Layer");

		// 設計図の選択
		Engine::Editor::AssetField<Resource::AnimatorAsset>(
			*a_context.pWorld->RefEngineServices(),
			"Animator",
			"AnimatorAsset",
			_layer.animatorGUID,
			_layer.animatorHandle
		);

		// 現在のステートを表示
		const auto* _pAnimator = a_context.pWorld->RefEngineServices()->pResourceManager->Get(_layer.animatorHandle);
		if (_pAnimator)
		{
			std::string _nodeNameStr(_pAnimator->GetNodeName(_layer.currentStateHash));
			Engine::Editor::Value("Current Node", "%s", _nodeNameStr.c_str());
		}
		Engine::Editor::Value("State Time", "%.2f s", _layer.stateTime);
		Engine::Editor::Value("Clip Time", "%.2f", _layer.clipTime);
	}
};
