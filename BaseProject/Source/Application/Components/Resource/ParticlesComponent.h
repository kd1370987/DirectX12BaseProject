#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

struct ParticlesComponent
{
	// 参照データ
	Engine::GUID particleGUID;
	Engine::Handle<Engine::Resource::ParticlesAsset> particlesAssetHandle;

	// パラメータ
	DirectX::XMFLOAT3 posOffset;		// 噴出座標オフセット
	DirectX::XMFLOAT3 rotation;			// 噴出方向 : クォータニオンで持たせる必要はない
	DirectX::XMFLOAT2 scale;			// スケール

	bool isPlay = false;				// 現在再生中か
	float time = 0.0f;					// 経過時間
	
	bool isBillboard = true;			// ビルボードするかどうか


	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const ParticlesComponent*>(a_ptr);
		Engine::JSONHelper::SetValue("particleGUID",a_json,_comp->particleGUID);
		Engine::JSONHelper::SetValue("posOffset",a_json,_comp->posOffset);
		Engine::JSONHelper::SetValue("rotation",a_json,_comp->rotation);
		Engine::JSONHelper::SetValue("scale",a_json,_comp->scale);
		Engine::JSONHelper::SetValue("isBillboard",a_json,_comp->isBillboard);

	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<ParticlesComponent*>(a_ptr);
		_comp->particleGUID = Engine::JSONHelper::GetValue("particlleGUID", a_json, Engine::DefaultGUID);
		_comp->posOffset = Engine::JSONHelper::GetValue("posOffset", a_json, DirectX::XMFLOAT3({ 0,0,0 }));
		_comp->rotation = Engine::JSONHelper::GetValue("rotation", a_json, DirectX::XMFLOAT3({ 0,0 ,0 }));
		_comp->scale = Engine::JSONHelper::GetValue("scale", a_json, DirectX::XMFLOAT2({ 0,0 }));
		_comp->isBillboard = Engine::JSONHelper::GetValue("isBillboard", a_json, true);
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_data);

	}
};