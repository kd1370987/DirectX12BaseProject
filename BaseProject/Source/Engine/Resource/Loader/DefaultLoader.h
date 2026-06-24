#pragma once

#include "Model/ModelLoader.h"
#include "Texture/TextureLoader.h"
#include "StateMachineAsset/StateMachineAssetLoader.h"

namespace Engine::Resource
{
	// ロード処理の中間用クラス
	template<typename T>
	struct DefaultLoader
	{
		static T LoadFromFile(const std::string& a_path)
		{
			// 特殊化されていない型で Load<T> が呼ばれたらコンパイルを止める
			static_assert(sizeof(T) == 0, "特殊化されていない型のLoaderが呼ばれました。DefaultLoaderを特殊化してください。");
			return T();
		}
	};

	// -----------------------------------------------------
	// テンプレート特殊化
	// -----------------------------------------------------
	// モデル
	template<>
	struct DefaultLoader<Model>
	{
		static Model LoadFromFile(const std::string& a_path)
		{
			// ModelLoaderクラスの専用処理
			return ModelLoader::Load(a_path);
		}
	};
	// テクスチャ
	template<>
	struct DefaultLoader<Texture>
	{
		static Texture LoadFromFile(const std::string& a_path)
		{
			return TextureLoader::LoadFromFile(a_path);
		}
	};
	// ステートマシン
	template<>
	struct DefaultLoader<StateMachineAsset>
	{
		static StateMachineAsset LoadFromFile(const std::string& a_path)
		{
			return StateMachineAssetLoader::LoadFromFile(a_path);
		}
	};

	
}