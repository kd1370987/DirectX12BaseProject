#include "ModelImporter.h"

#include "tinyGLTF/tinyGLTF.h"

Engine::Resource::Model Engine::Resource::ImportModel(const std::string& a_filePath)
{
	//-------------------------------------
	// 拡張子を取得
	//-------------------------------------
	std::string _ext = FileUtility::GetFilePathExtension(a_filePath);

	//-------------------------------------
	// 独自形式があるのかチェック
	//-------------------------------------
	auto _originExt = FileUtility::FindExtensionInDirectory(
		FileUtility::GetDirFromPath(a_filePath),		// 親ディレクトリパス取得
		".originalBin"
	);
	if (_originExt.size() > 0)
	{
		assert(0 && "独自形式の読み込み");
	}
	//-------------------------------------
	// TinyGLTFを使用
	//-------------------------------------
	else if(_ext == "gltf" || _ext == "glb")
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

	return Engine::Resource::Model();
}
