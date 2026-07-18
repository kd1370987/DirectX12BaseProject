#include "AssetInspector.h"

#include "ResourceDraw/ResourceDraw.h"

void Engine::Editor::Inspector::AssetInspector(EditorContext& a_editContext)
{
	// アセットが選択チェック
	if (!a_editContext.pAssetProp)
	{
		ImGui::Text("No selected Asset");
		return;
	}

	// アセットが選択されている場合
	// メタ情報
	ImGui::Text("GUID : %s",a_editContext.pAssetProp->guid.String().c_str());
	ImGui::Separator();
	ImGui::Text("FilePath");
	for (auto& _ext : a_editContext.pAssetProp->extensionsVec)
	{
		auto _filePath = a_editContext.pAssetProp->filePath + a_editContext.pAssetProp->fileName + _ext;
		ImGui::Text("%s", _filePath.c_str());
	}
	ImGui::Separator();

	// タイプごとのアセット描画
	const auto& _type = a_editContext.pAssetProp->type;
	if (_type == "Model")
	{
		ModelDraw(a_editContext);
	}
	else if (_type == "Mesh")
	{
		MeshDraw(a_editContext);
	}
	else if (_type == "Material")
	{
		MaterialDraw(a_editContext);
	}
	else if (_type == "Animation")
	{
		AnimationDraw(a_editContext);
	}
	else if (_type == "AnimatorAsset")
	{
		AnimatorDraw(a_editContext);
	}
	else if (_type == "ActionStateMachineAsset")
	{
		ActionStateMachineDraw(a_editContext);
	}
	else if (_type == "Texture")
	{
		TextureDraw(a_editContext);
	}
	else if (_type == "Shader")
	{
		ShaderDraw(a_editContext);
	}
	else if (_type == "ParticlesAsset")
	{
		ParticleDraw(a_editContext);
	}
	else if (_type == "ShadingModelTable")
	{
		ShadingModelTableDraw(a_editContext);
	}
}
