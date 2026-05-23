#include "obmdlImporter.h"

#include "../../../../../Utility/BinaryHelper/BinaryHelper.h"

namespace Engine::Resource
{
	bool Load(const std::string& a_filePath, ModelData& a_outData)
	{
		// ファイルオープン
		std::ifstream _ifs(a_filePath, std::ios::binary);
		if (_ifs.is_open())
		{
			Editor::MainEditor::Instance().ErrorLog("fileOpenError : %s",a_filePath);
			return false;
		}

		a_outData = {};

		// 保存時と全く同じ順序で読み込む

		// モデル名
		a_outData.name = BinaryHelper::ReadString(_ifs);

		// 参照データGUID
	}
}