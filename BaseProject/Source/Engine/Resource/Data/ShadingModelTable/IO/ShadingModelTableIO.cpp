#include "ShadingModelTableIO.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	ShadingModelTable ShadingModelTableIO::LoadFromFile(const std::string& a_path)
	{
		auto _fileDir = FileUtility::GetDirFromPath(a_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_path);
		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "smtble");
		ShadingModelTable _smTable = {};
		_smTable.Archive(_ar);
		return _smTable;
	}
	void ShadingModelTableIO::Create(const std::string& a_path, const std::string& a_name)
	{
		// ディレクトリ
		static std::string _dir = "Asset/ShadingModelTables/";
		auto _basePath = _dir + a_path + "/" + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			// すでに作成されていた場合
			ENGINE_LOG("すでに作成済みのテーブルです : %s", _basePath.c_str());
			return;
		}


		// アセットデータベースに場所を作る
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath, "ShadingModelTable");

		// リソースマネージャーに登録
		ShadingModelTable _sma(a_name);
		auto _saveFileDir = FileUtility::GetDirFromPath(_basePath);
		auto _saveFileName = FileUtility::GetFileNameWithoutExtension(_basePath);
		Persistence::Archive _ar(Persistence::Archive::Mode::Save,_saveFileDir,_saveFileName,"smtble");
		_sma.Archive(_ar);

		ResourceManager::Instance().AddResourceAndGUID(std::move(_sma), _guid);
	}
}
