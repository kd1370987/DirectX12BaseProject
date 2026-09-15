#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "Application/Utility/PrefabSpawnHelper.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Persistence/GUIDComponent.h"

//==========================================================================================
// BoidLeaderComponent
//
// ボイドの群れを率いる側。自分の周りにボイドのプレハブを出し、
// 出したボイドの FollowTargetComponent に自分を入れて追わせる。
// 追従先の座標をボイドの目標地点へ流すのは FollowLeaderSystem。
//
// ・今はテスト段階なので、出すのはエディターのボタンからだけ。
//==========================================================================================
struct BoidLeaderComponent
{
	Engine::GUID boidPrefabGUID = {};					// 出すボイド(セーブされる)
	Engine::Handle<Engine::Resource::Prefab> prefab;	// ランタイム用(初回生成時に解決)
	uint32_t spownNum = 0;		// 出現させるボイド数
	float spawnRadius = 5.0f;	// 自分を中心にこの半径の球内へばらまく(同じ座標に重ねると反発の向きが出ない)
};

template<>
struct Engine::ECS::ComponentTraits<BoidLeaderComponent>
{
	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData, const Engine::ECS::EngineServices& a_services)
	{
		BoidLeaderComponent& _comp = Engine::Editor::GetValue<BoidLeaderComponent>(a_pData);
		a_services.pResourceManager->ReleaseHandle(_comp.prefab);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoidLeaderComponent& _comp = Engine::Editor::GetValue<BoidLeaderComponent>(a_pData);
		a_ar.Field("boidPrefabGUID", _comp.boidPrefabGUID);
		a_ar.Field("spownNum", _comp.spownNum);
		a_ar.Field("spawnRadius", _comp.spawnRadius);
	}

	static void Edit(CompEditContext& a_context)
	{
		BoidLeaderComponent& _comp = Engine::Editor::GetValue<BoidLeaderComponent>(a_context.pData);
		auto& _services = *a_context.pWorld->RefEngineServices();

		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(_services, "Boid Prefab", "Prefab", _comp.boidPrefabGUID))
		{
			// GUIDが変わったら作り直し。借りていたぶんは返す
			_services.pResourceManager->ReleaseHandle(_comp.prefab);
		}
		ImGui::InputScalar("SpawnNum", ImGuiDataType_U32, &_comp.spownNum);
		ImGui::DragFloat("SpawnRadius", &_comp.spawnRadius, 0.1f, 0.0f);

		//------------------------------------------------------------------------------
		// テスト用の生成
		//------------------------------------------------------------------------------
		ImGui::Separator();
		ImGui::BeginDisabled(_comp.boidPrefabGUID == Engine::DefaultGUID || _comp.spownNum == 0);
		if (Engine::Editor::EditorHelper::CreateButton("Spawn Boids"))
		{
			SpawnBoids(a_context, _comp);
		}
		ImGui::EndDisabled();
	}

private:
	//----------------------------------------------------------------------------------
	// 自分の周りにボイドを spownNum 体出し、追従先に自分を入れる
	//----------------------------------------------------------------------------------
	static void SpawnBoids(CompEditContext& a_context, BoidLeaderComponent& a_comp)
	{
		auto& _world = *a_context.pWorld;
		const Engine::ECS::Entity _self = a_context.entity;

		App::Utility::SpawnParams _params = {};
		_params.followTarget = _self;

		// 追従先の GUID。Awake で GUID から引き直されるので揃えておく
		if (_world.HasComponent<GUIDComponent>(_self))
		{
			_params.followTargetGUID = _world.RefData<GUIDComponent>(_self)->guid;
		}

		Math::Vector3 _center = {};
		if (_world.HasComponent<LocalTransformComponent>(_self))
		{
			_center = _world.RefData<LocalTransformComponent>(_self)->pos;
		}

		for (uint32_t _i = 0; _i < a_comp.spownNum; ++_i)
		{
			// 球内に一様に散らす(半径は立方根で偏りを消す)
			Math::Vector3 _dir(
				Math::Random::Float(-1.0f, 1.0f),
				Math::Random::Float(-1.0f, 1.0f),
				Math::Random::Float(-1.0f, 1.0f));
			if (_dir.LengthSquared() < 1e-6f) _dir = Math::Vector3(0.0f, 0.0f, 1.0f);
			_dir.Normalize();
			const float _r = a_comp.spawnRadius * std::cbrt(Math::Random::Float(0.0f, 1.0f));

			_params.pos = _center + _dir * _r;

			if (!App::Utility::SpawnPrefab(
				_world,
				*_world.RefEngineServices()->pResourceManager,
				a_comp.boidPrefabGUID,
				a_comp.prefab,
				_params))
			{
				ENGINE_WARNING("BoidLeader : ボイドのプレハブを生成できませんでした");
				return;
			}
		}
	}
};
