#include "RenderingPipelineAssetIO.h"

#include "../RenderingPipelineMetaRegistry.h"
#include "../StandardPipeline/StandardPipeline.h"
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

		// 形式はJSON固定。
		//
		// バイナリは「書いた順にそのまま並べるだけ」でキーを持たないので、
		// パスにパラメータを1つ足しただけで、それ以降の読み出しが全部ずれる。
		// パスの種類ぶんだけ増改築が起きる構成データなので、この形式とは相性が悪い。
		// (Auto にすると .ob が残っている間そちらを読みに行くことがある)
		Persistence::Archive _arch(
			Persistence::Archive::Mode::Load, _fileDir, _fileName, RenderingPipelineAsset::kExtension,
			Persistence::Archive::ArchiveFormat::Json);
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

		RenderingPipelineAsset _asset = {};
		_asset.SetMetaRegistry(a_pRegistry);
		_asset.SetName(a_name);

		// 中身は標準構成(既存の描画と同じ流れ)から始める。
		// 空で作ると何も映らないので、まず絵が出る状態を土台にして、
		// そこから要らないパスを外していく形にしている
		if (a_pRegistry) BuildStandardPipeline(_asset, *a_pRegistry);

		_asset.Save(_basePath);

		// リソースマネージャーに登録
		Resource::ResourceManager::Instance().AddResourceAndGUID(std::move(_asset), _guid);
	}
}
