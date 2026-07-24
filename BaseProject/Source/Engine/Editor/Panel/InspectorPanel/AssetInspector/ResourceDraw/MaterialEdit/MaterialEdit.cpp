#include "MaterialEdit.h"

#include "../../../../../EditorUI/EditorUI.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// マテリアルの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void MaterialEdit(EditorContext& a_editContext, Resource::Material* a_pMaterial)
	{
		if (!a_pMaterial) { return; }

		auto _guid = a_editContext.pAssetProp->guid;

		if (ImGui::Button("Save"))
		{
			auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			auto _fileDir = FileUtility::GetDirFromPath(_filePath);
			auto _fileName = FileUtility::GetFileNameWithoutExtension(_filePath);
			Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "mtrl");
			a_pMaterial->Archive(_ar);
		}

		ImGui::InputText("name", &a_pMaterial->name);
		ImGui::Separator();
		Editor::DrawEnumFlagsCombo("AlphaMode", a_pMaterial->alphaMode);

		// シェーディングモデル
		Editor::UI::DrawAssetSelectCombo<Resource::ShadingModelTable>(
			"Change Shading Model",
			"ShadingModelTable",
			a_pMaterial->shedingModelGUID,
			a_pMaterial->shadingModelHandle
		);

		ImGui::Separator();

		// 各テクスチャの描画
		if (ImGui::CollapsingHeader("Albedo"))
		{
			Editor::UI::DrawAssetSelectCombo<Resource::Texture>(
				"Change AlbedTex",
				"Texture",
				a_pMaterial->baseColorTexGUID,
				a_pMaterial->baseColorTex
			);
			ImGui::DragFloat4("AlbedScale", &a_pMaterial->baseColor.x, 0.01f, 0.0f);
			Editor::UI::DrawTexture(a_pMaterial->baseColorTex, 256, 256);
		}
		if (ImGui::CollapsingHeader("Metallic / Roughness"))
		{
			Editor::UI::DrawAssetSelectCombo<Resource::Texture>(
				"Change MetaricRoughnessTex",
				"Texture",
				a_pMaterial->metaRoughTexGUID,
				a_pMaterial->metaRoughTex
			);
			ImGui::DragFloat("MetallicScale", &a_pMaterial->metallic, 0.01f, 0.0f);
			ImGui::DragFloat("RoughnessScale", &a_pMaterial->roughness, 0.01f, 0.0f);
			Editor::UI::DrawTexture(a_pMaterial->metaRoughTex, 256, 256);
		}
		if (ImGui::CollapsingHeader("Emissive"))
		{
			Editor::UI::DrawAssetSelectCombo<Resource::Texture>(
				"Change EmissiveTex",
				"Texture",
				a_pMaterial->emissiveTexGUID,
				a_pMaterial->emissiveTex
			);
			ImGui::DragFloat3("EmissiveScale", &a_pMaterial->emissive.x, 0.01f, 0.0f);
			Editor::UI::DrawTexture(a_pMaterial->emissiveTex, 256, 256);
		}
		if (ImGui::CollapsingHeader("Normal"))
		{
			Editor::UI::DrawAssetSelectCombo<Resource::Texture>(
				"Change NormalTex",
				"Texture",
				a_pMaterial->normalTexGUID,
				a_pMaterial->normalTex
			);
			Editor::UI::DrawTexture(a_pMaterial->normalTex, 256, 256);
		}
	}
}
