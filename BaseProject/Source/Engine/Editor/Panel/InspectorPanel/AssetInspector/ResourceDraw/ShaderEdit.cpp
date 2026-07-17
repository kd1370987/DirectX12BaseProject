#include "ShaderEdit.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// シェーダーの詳細表示
	//-----------------------------------------------------------------------------------------
	void ShaderEdit(EditorContext& a_editContext, Resource::Shader* a_pShader)
	{
		if (!a_pShader) { return; }

		auto _guid = a_editContext.pAssetProp->guid;
		auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);

		ImGui::Text("Stage    : %s", std::string(magic_enum::enum_name(a_pShader->GetStage())).c_str());
		ImGui::Text("FilePath : %s", _filePath.c_str());

		// バイトコードのサイズ : 未コンパイルなら0
		const auto& _byteCode = a_pShader->GetByteCode();
		ImGui::Text("ByteCode : %zu byte", _byteCode.BytecodeLength);

		if (_byteCode.BytecodeLength == 0)
		{
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "(Shader is not compiled)");
		}
	}
}
