#include "RenderingPipelineAssetIO.h"

#include "../RenderingPipelineMetaRegistry.h"
#include "../../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics::Pipeline
{
	// アセットの置き場所 : 他のアセットと同じく Asset/ 以下に種類ごとのフォルダを掘る
	static const std::string kAssetDir = "Asset/RenderingPipeline/";

	RenderingPipelineAsset RenderingPipelineAssetIO::LoadFromFile(const std::string& a_path, PassMetaRegistry* a_pRegistry)
	{
		RenderingPipelineAsset _asset = {};

		// レジストリが無いとパスの実体を作り直せない。
		// 中身が空のまま返すと「読めているのにパスが1つも無い」状態になって原因が追いにくいので出しておく
		if (!a_pRegistry)
		{
			ENGINE_WARNING("[RenderingPipelineAssetIO] PassMetaRegistry が無いため読み込めません : %s", a_path.c_str());
			return _asset;
		}

		auto _fileDir = Engine::File::GetDirFromPath(a_path);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_path);

		_asset.SetMetaRegistry(a_pRegistry);
		_asset.SetName(_fileName);

		Persistence::Archive _arch(
			Persistence::Archive::Mode::Load, _fileDir, _fileName, RenderingPipelineAsset::kExtension);
		_asset.Archive(_arch);

		return _asset;
	}

	void RenderingPipelineAssetIO::Create(const std::string& a_path, const std::string& a_name, PassMetaRegistry* a_pRegistry)
	{
		auto _basePath = kAssetDir + a_path + "/" + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_LOG("すでに作成済みのパイプラインです : %s", _basePath.c_str());
			return;
		}

		// アセットデータベースに場所を作る
		auto _guid = Resource::AssetDatabase::Instance().AddMetaData(_basePath, "RenderingPipelineAsset");

		// 空の状態で書き出す(パスは後からノードエディタで足す)
		RenderingPipelineAsset _asset = {};
		_asset.SetMetaRegistry(a_pRegistry);
		_asset.SetName(a_name);
		_asset.Save(_basePath);

		// リソースマネージャーに登録
		Resource::ResourceManager::Instance().AddResourceAndGUID(std::move(_asset), _guid);
	}
}
