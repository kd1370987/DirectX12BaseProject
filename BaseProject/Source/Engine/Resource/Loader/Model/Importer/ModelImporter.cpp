#include "ModelImporter.h"

#include "tinyGLTF/tinyGLTF.h"



namespace Engine::Resource
{
	Engine::Resource::ModelData Engine::Resource::ImportModel(const std::string& a_filePath)
	{
		// 拡張子とファイルディレクトリ取得
		std::string _ext = FileUtility::GetFilePathExtension(a_filePath);
		std::string _dir = FileUtility::GetDirFromPath(a_filePath);

		// 独自形式があるのかチェック
		auto _originExt = FileUtility::FindExtensionInDirectory(_dir, ".obmdl");

		//-------------------------------------
		// 独自形式読み込み
		//-------------------------------------
		if (_originExt.size() > 0)
		{
			assert(0 && "独自形式の読み込み");
		}
		//-------------------------------------
		// TinyGLTFを使用
		//-------------------------------------
		else if (_ext == "gltf")
		{
			return GLTF::Import(a_filePath);
		}
		//-------------------------------------
		// Assimpを使用
		//-------------------------------------
		else
		{
			assert(0 && "Assimpは未対応");
		}

		return Engine::Resource::ModelData();
	}
}