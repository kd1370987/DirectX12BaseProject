#include "AudioBehaviorIO.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	AudioBehavior AudioBehaviorIO::LoadFromFile(const std::string& a_path)
	{
		auto _fileDir = Engine::File::GetDirFromPath(a_path);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_path);

		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "audbhv");

		AudioBehavior _behavior = {};
		_behavior.Archive(_ar);
		return _behavior;
	}

	void AudioBehaviorIO::Create(const std::string& a_path, const std::string& a_name)
	{
		// ディレクトリ
		static std::string _dir = "Asset/AudioBehavior/";
		auto _basePath = _dir + a_path + "/" + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_LOG("すでに作成済みのオーディオビヘイビアです : %s", _basePath.c_str());
			return;
		}

		// アセットデータベースに場所を作る
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath, "AudioBehavior");

		// 空の状態で書き出す(音は後からインスペクターで割り当てる)
		AudioBehavior _behavior(a_name);
		_behavior.Save(_basePath);

		// リソースマネージャーに登録
		ResourceManager::Instance().AddResourceAndGUID(std::move(_behavior), _guid);
	}
}
