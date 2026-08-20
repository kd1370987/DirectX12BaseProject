#include "EffectAssetIO.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	EffectAsset EffectAssetIO::LoadFromFile(const std::string& a_path)
	{
		auto _fileDir = Engine::File::GetDirFromPath(a_path);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_path);

		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "effect");

		EffectAsset _effect = {};
		_effect.Archive(_ar);

		// GUID しか入っていないので、参照アセットをここで引き当てる
		_effect.ResolveReferences();

		return _effect;
	}

	void EffectAssetIO::Create(const std::string& a_path, const std::string& a_name)
	{
		// ディレクトリ
		static std::string _dir = "Asset/Effect/";
		auto _basePath = _dir + a_path + "/" + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_LOG("すでに作成済みのエフェクトです : %s", _basePath.c_str());
			return;
		}

		// アセットデータベースに場所を作る
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath, "EffectAsset");

		// 空の状態で書き出す(パーツは後からインスペクターで足す)
		EffectAsset _effect(a_name);
		_effect.Save(_basePath);

		// リソースマネージャーに登録
		ResourceManager::Instance().AddResourceAndGUID(std::move(_effect), _guid);
	}
}
