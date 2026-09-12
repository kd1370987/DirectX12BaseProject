#include "ParticlesIO.h"

#include "../../../Data/Particles/ParticlesAsset.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"

namespace Engine::Resource
{
	ParticlesAsset ParticlesAssetIO::LoadFromFile(const std::string& a_path)
	{
		ParticlesAsset _pa = {};
		_pa.Load(a_path);
		return _pa;
	}
	void ParticlesAssetIO::Create(const std::string& a_path, const std::string& a_name)
	{
		// ディレクトリ
		static std::string _dir = "Asset/ParticlesAsset/";
		auto _basePath = _dir + a_path +"/" + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			// すでに作成されていた場合
			ENGINE_LOG("すでに作成済みのパーティクルです : %s",_basePath.c_str());
			return;
		}


		// 書き出すだけでよい。
		// メタファイルとGUIDは、AssetDatabase の監視が新しいファイルを見つけて用意する。
		// 自分のGUIDは空のまま書き出すが、読み込み時に引き直すので問題ない
		ParticlesAsset _sma = {};
		_sma.Create(a_name);
		_sma.Save(_basePath);
	}
}