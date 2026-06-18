#pragma once

#include "../BaseLoader.h";

namespace Engine::Resource
{
	class ParticlesAsset;

	class ParticlesAssetLoader : public BaseLoader<ParticlesAsset, ParticlesAssetLoader>
	{
	public:

		// 読み込み
		static Handle<ParticlesAsset> Load(const Engine::GUID& a_guid);

		// リクエスト
		static std::pair<Engine::GUID, Handle<ParticlesAsset>> Create(const std::string& a_path, const std::string& a_name);
	};
}