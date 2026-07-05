#include "Material.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"

//#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "Engine/Resource/Loader/Texture/TextureLoader.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"

#include "../../../Editor/EditorUI/EditorUI.h"

void Engine::Resource::Material::Release()
{

}

void Engine::Resource::Material::SetTexture2D(
	const std::string& a_fileDir,
	const std::string& a_baseColorTexFileName,
	const std::string& a_metallicRoughnessTexFileName,
	const std::string& a_emissiveTexFileName,
	const std::string& a_normalTexFileName
)
{
	baseColorTexGUID	= AssetDatabase::Instance().GetGUIDFromFilePath(a_fileDir + a_baseColorTexFileName);
	metaRoughTexGUID	= AssetDatabase::Instance().GetGUIDFromFilePath(a_fileDir + a_metallicRoughnessTexFileName);
	emissiveTexGUID		= AssetDatabase::Instance().GetGUIDFromFilePath(a_fileDir + a_emissiveTexFileName);
	normalTexGUID		= AssetDatabase::Instance().GetGUIDFromFilePath(a_fileDir + a_normalTexFileName);

	baseColorTex	= TextureLoader::LoadTexture(baseColorTexGUID, TexColor::WHITE);
	metaRoughTex	= TextureLoader::LoadTexture(metaRoughTexGUID, TexColor::ORM);
	emissiveTex		= TextureLoader::LoadTexture(emissiveTexGUID, TexColor::BLACK);
	normalTex		= TextureLoader::LoadTexture(normalTexGUID, TexColor::NORMAL);
}

void Engine::Resource::Material::Archive(Persistence::Archive& a_ar)
{
	a_ar.StringField("MaterialName", name);
	a_ar.Field("AlphaMode", alphaMode);

	// 参照テクスチャGUID
	a_ar.Field("BaseColorTexGUID", baseColorTexGUID);
	a_ar.Field("MetaRoughTexGUID", metaRoughTexGUID);
	a_ar.Field("EmissiveTexGUID", emissiveTexGUID);
	a_ar.Field("NormalTexGUID", normalTexGUID);

	// スケール値
	a_ar.Field("BaseColor", baseColor);
	a_ar.Field("Metallic", metallic);
	a_ar.Field("Roughness", roughness);
	a_ar.Field("Emissive", emissive);

	// シェーディングモデル
	a_ar.Field("shedingModelGUID", shedingModelGUID);
}

void Engine::Resource::Material::Edit(const Engine::GUID& a_guid)
{
	if (ImGui::Button("Save"))
	{
		auto _filePath = AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		auto _fileDir = FileUtility::GetDirFromPath(_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(_filePath);
		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "mtrl");
		Archive(_ar);
	}

	ImGui::InputText("name", &name);
	ImGui::Separator();

	// Flagsではなく通常のEnumComboにする（関数の実装に合わせて変更してください）
	// Editor::DrawEnumCombo("AlphaMode", alphaMode); 
	Editor::DrawEnumFlagsCombo("AlphaMode", alphaMode); // ※取り急ぎそのままにしていますが、要確認

	// シェーディングモデル
	Editor::UI::DrawAssetSelectCombo<ShadingModelTable>(
		"Change Shading Model",
		"ShadingModelTable",
		shedingModelGUID,
		shadingModelHandle
	);

	ImGui::Separator();

	// Selectable ではなく CollapsingHeader を使う！
	if (ImGui::CollapsingHeader("Albedo"))
	{
		Editor::UI::DrawAssetSelectCombo<Texture>(
			"Change AlbedTex",
			"Texture",
			baseColorTexGUID,
			baseColorTex
		);
		ImGui::DragFloat4("AlbedScale", &baseColor.x, 0.01f, 0.0f);
		// 1920x1080 は大きすぎるので適度なサイズに制限
		Editor::UI::DrawTexture(baseColorTex, 256, 256);
	}
	if (ImGui::CollapsingHeader("Metallic / Roughness"))
	{
		Editor::UI::DrawAssetSelectCombo<Texture>(
			"Change MetaricRoughnessTex",
			"Texture",
			metaRoughTexGUID,
			metaRoughTex
		);
		ImGui::DragFloat("MetallicScale", &metallic, 0.01f, 0.0f);
		ImGui::DragFloat("RoughnessScale", &roughness, 0.01f, 0.0f);
		Editor::UI::DrawTexture(metaRoughTex, 256, 256);
	}
	if (ImGui::CollapsingHeader("Emissive"))
	{
		Editor::UI::DrawAssetSelectCombo<Texture>(
			"Change EmissiveTex",
			"Texture",
			emissiveTexGUID,
			emissiveTex
		);
		ImGui::DragFloat3("EmissiveScale", &emissive.x, 0.01f, 0.0f);
		Editor::UI::DrawTexture(emissiveTex, 256, 256);
	}
	if (ImGui::CollapsingHeader("Normal"))
	{
		Editor::UI::DrawAssetSelectCombo<Texture>(
			"Change NormalTex",
			"Texture",
			normalTexGUID,
			normalTex
		);
		Editor::UI::DrawTexture(normalTex, 256, 256);
	}
}