#include "ShaderEdit.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// シェーダーの詳細表示
	//-----------------------------------------------------------------------------------------
	void ShaderEdit(EditorContext& a_editContext, Resource::Shader* a_pShader)
	{
		if (!a_pShader) { return; }

		auto _guid = a_editContext.pAssetProp->guid;
		auto _filePath = a_editContext.pServices->pAssetDatabase->GetFilePathFromGUID(_guid);

		Engine::Editor::Value("Stage", "%s", std::string(magic_enum::enum_name(a_pShader->GetStage())).c_str());
		Engine::Editor::Value("FilePath", "%s", _filePath.c_str());

		// バイトコードのサイズ : 未コンパイルなら0
		const auto& _byteCode = a_pShader->GetByteCode();
		Engine::Editor::Value("ByteCode", "%zu byte", _byteCode.BytecodeLength);

		if (_byteCode.BytecodeLength == 0)
		{
			Engine::Editor::WarningText("(Shader is not compiled)");
		}
	}
}
