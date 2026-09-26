#include "MaterialEdit.h"

#include "../../../../../Helper/EditorField.inl"

#include "../../AssetLink.h"

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
			auto _filePath = a_editContext.pServices->pAssetDatabase->GetFilePathFromGUID(_guid);
			auto _fileDir = Engine::File::GetDirFromPath(_filePath);
			auto _fileName = Engine::File::GetFileNameWithoutExtension(_filePath);
			Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "mtrl");
			a_pMaterial->Archive(_ar);
		}

		ImGui::InputText("name", &a_pMaterial->name);
		ImGui::Separator();
		Editor::FlagsField("AlphaMode", a_pMaterial->alphaMode);

		ImGui::Separator();

		// 各テクスチャの描画
		if (ImGui::CollapsingHeader("Albedo"))
		{
			Editor::AssetField<Resource::Texture>(
				*a_editContext.pServices,
				"Change AlbedTex",
				"Texture",
				a_pMaterial->baseColorTexGUID,
				a_pMaterial->baseColorTex
			);
			DrawAssetLink(&a_editContext, "Texture :", a_pMaterial->baseColorTexGUID);
			ImGui::DragFloat4("AlbedScale", a_pMaterial->baseColor.Data(), 0.01f, 0.0f);
			Editor::Image(*a_editContext.pServices, a_pMaterial->baseColorTex, 256, 256);
		}
		if (ImGui::CollapsingHeader("Metallic / Roughness"))
		{
			Editor::AssetField<Resource::Texture>(
				*a_editContext.pServices,
				"Change MetaricRoughnessTex",
				"Texture",
				a_pMaterial->metaRoughTexGUID,
				a_pMaterial->metaRoughTex
			);
			DrawAssetLink(&a_editContext, "Texture :", a_pMaterial->metaRoughTexGUID);
			ImGui::DragFloat("MetallicScale", &a_pMaterial->metallic, 0.01f, 0.0f);
			ImGui::DragFloat("RoughnessScale", &a_pMaterial->roughness, 0.01f, 0.0f);
			Editor::Image(*a_editContext.pServices, a_pMaterial->metaRoughTex, 256, 256);
		}
		if (ImGui::CollapsingHeader("Emissive"))
		{
			Editor::AssetField<Resource::Texture>(
				*a_editContext.pServices,
				"Change EmissiveTex",
				"Texture",
				a_pMaterial->emissiveTexGUID,
				a_pMaterial->emissiveTex
			);
			DrawAssetLink(&a_editContext, "Texture :", a_pMaterial->emissiveTexGUID);
			ImGui::DragFloat3("EmissiveScale", &a_pMaterial->emissive.x, 0.01f, 0.0f);
			Editor::Image(*a_editContext.pServices, a_pMaterial->emissiveTex, 256, 256);
		}
		if (ImGui::CollapsingHeader("Normal"))
		{
			Editor::AssetField<Resource::Texture>(
				*a_editContext.pServices,
				"Change NormalTex",
				"Texture",
				a_pMaterial->normalTexGUID,
				a_pMaterial->normalTex
			);
			DrawAssetLink(&a_editContext, "Texture :", a_pMaterial->normalTexGUID);
			Editor::Image(*a_editContext.pServices, a_pMaterial->normalTex, 256, 256);
		}
	}
}
