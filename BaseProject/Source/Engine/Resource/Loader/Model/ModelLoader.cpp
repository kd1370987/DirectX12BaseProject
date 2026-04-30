#include "ModelLoader.h"
#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
#include "../../../Raytracing/RaytracingEngine/RaytracingEngine.h"
namespace Engine::Resource
{
	std::unordered_map<Engine::GUID, Engine::Resource::Handle<Engine::Resource::Model>>
		Engine::Resource::ModelLoader::m_cache;

	Handle<Model> ModelLoader::Load(const Engine::GUID& a_guid)
	{
		// すでに読み込み済みならそのハンドルを返す
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return _it->second;
		}

		// なければロード
		auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		Model _model = {};
		_model.Import(_path);

		//リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(_model);

		// キャッシュに登録
		m_cache.emplace(a_guid, _handle);

		// レイトレにとりあえず保存
		Engine::Raytracing::RayEngine::Instance().RegistModel(DXSM::Matrix::Identity, _handle);

		return _handle;
	}
	Handle<Model> ModelLoader::Request(const std::string& a_path)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			return Load(_guid);
		}

		// GUIDがなければ
		// フォルダ以下をもう一度探す処理を入れる予定だが、とりあえず、エラー値を返す
		return Handle<Model>();
	}
	const std::unordered_map<Engine::GUID, Handle<Model>>& ModelLoader::GetAllCache()
	{
		return m_cache;
	}
}