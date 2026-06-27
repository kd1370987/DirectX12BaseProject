#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../Engine/Resource/Loader/Particles/ParticlesLoader.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

struct ParticlesComponent
{
	// 参照データ
	Engine::GUID particleGUID;
	Engine::Handle<Engine::Resource::ParticlesAsset> particlesAssetHandle;

	// パラメータ
	int particleCount = 0;

	DirectX::XMFLOAT3 posOffset;		// 噴出座標オフセット
	DirectX::XMFLOAT3 rotation;			// 噴出方向 : クォータニオンで持たせる必要はない
	DirectX::XMFLOAT2 scale;			// スケール

	bool isPlay = false;				// 現在再生中か
	float time = 0.0f;					// 経過時間
	
	bool isBillboard = true;			// ビルボードするかどうか
};


template<>
struct Engine::ECS::ComponentTraits<ParticlesComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_pData);
		a_ar.Field("particleCount",_comp.particleCount);
		a_ar.Field("particleGUID",_comp.particleGUID);
		a_ar.Field("posOffset",_comp.posOffset);
		a_ar.Field("rotation",_comp.rotation);
		a_ar.Field("scale",_comp.scale);
		a_ar.Field("isBillboard",_comp.isBillboard);
	}

	static void Edit(void* a_pData)
	{
		using namespace Engine;
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_pData);

		ImGui::Text("Parameter");
		ImGui::DragInt("ParticleNum", &_comp.particleCount, 1, 0);
		ImGui::DragFloat3("Dir", &_comp.rotation.x, 0.1f, 0.0f);


		ImGui::Separator();

		Editor::Helper::DrawHandle(_comp.particlesAssetHandle);

		ImGui::Separator();

		// パーティクル選択
		if (ImGui::BeginCombo("Change Particle", "Select..."))
		{
			for (auto& _prop : Resource::AssetDatabase::Instance().GetTypeMetaVec("ParticlesAsset"))
			{
				// 現在のステートマシンと同じGUIDなら選択中フラグを立てる
				bool _selected = (_comp.particleGUID == _prop.guid);

				// 選択欄
				if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
				{
					_comp.particlesAssetHandle = Resource::ResourceManager::Instance().GetCache<Resource::ParticlesAsset>(_prop.guid);
					_comp.particleGUID = _prop.guid;
				}
			}
			ImGui::EndCombo();
		}
	}
};