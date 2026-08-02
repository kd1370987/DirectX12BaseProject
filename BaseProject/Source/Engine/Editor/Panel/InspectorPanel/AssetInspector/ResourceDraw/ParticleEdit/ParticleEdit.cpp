#include "ParticleEdit.h"

#include "../../../../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../../../../../Resource/Data/Texture/IO/TextureIO.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// パーティクルアセットの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void ParticleEdit(EditorContext& a_editContext, Resource::ParticlesAsset* a_pParticles)
	{
		if (!a_pParticles) { return; }

		if (ImGui::Button("Save"))
		{
			// ファイルパス取得
			auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_pParticles->GetGUID());
			a_pParticles->Save(_filePath);
			MainEditor::Instance().AddLog("%s", _filePath.c_str());
			MainEditor::Instance().AddLog(" : Save Particles\n");
		}

		// パラメーター変更
		ImGui::InputText("Name", &a_pParticles->RefName());
		ImGui::Text("%s", a_pParticles->GetGUID().String().c_str());

		ImGui::Separator();

		ImGui::PushID(1);
		ImGui::Text("InitialSpeed");
		ImGui::DragFloat("Min", &a_pParticles->RefInitalSpeedMin(), 0.1f, 0.0f);
		ImGui::DragFloat("Max", &a_pParticles->RefInitalSpeedMax(), 0.1f, 0.0f);
		ImGui::PopID();

		ImGui::Separator();

		ImGui::DragFloat("GravityPow", &a_pParticles->RefGravityPow(), 0.1f, 0.0f);

		ImGui::Separator();

		ImGui::PushID(2);
		ImGui::Text("LifeTime");
		ImGui::DragFloat("Min", &a_pParticles->RefLifeTimeMin(), 0.1f, 0.0f);
		ImGui::DragFloat("Max", &a_pParticles->RefLifeTimeMax(), 0.1f, 0.0f);
		ImGui::PopID();

		ImGui::Separator();

		ImGui::DragInt("Capacity", &a_pParticles->RefCapacity(), 1, 0);
		ImGui::DragInt("EmissionRate", &a_pParticles->RefEmissionRate(), 1, 0);

		ImGui::Separator();

		// 現在選択されているテクスチャ
		const auto* _pTex = Resource::ResourceManager::Instance().Ref(a_pParticles->GetTexHandle());

		ImGui::Separator();

		// テクスチャ選択コンボボックス
		// 反映は専用のロード関数を通すので、選択だけを共通ヘルパーに任せる
		GUID _selectedGUID = {};
		if (Editor::EditorHelper::DrawAssetGUIDCombo(
			"SelectTexture",
			"Texture",
			a_pParticles->GetTexGUID(),
			_selectedGUID))
		{
			// テクスチャのハンドル取得
			// ロードされていなかったら止まる
			a_pParticles->SetTexture(_selectedGUID, Resource::TextureIO::LoadTexture(_selectedGUID, TexColor::WHITE));
		}

		// テクスチャの画像を表示
		if (_pTex)
		{
			auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
			Helper::DrawSRVView(_gpuHandle, static_cast<float>(_pTex->GetDesc().Width), static_cast<float>(_pTex->GetDesc().Height));
		}
	}
}
