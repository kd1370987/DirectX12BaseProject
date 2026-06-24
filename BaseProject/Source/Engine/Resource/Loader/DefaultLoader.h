#pragma once

#include "Model/ModelLoader.h"

namespace Engine::Resource
{
	// ロード処理の中間用クラス
	template<typename T>
	struct DefaultLoader
	{
		static T LoadFromFile(const std::string& a_path)
		{
			// 特殊化されていない型で Load<T> が呼ばれたらコンパイルを止める！
			static_assert(sizeof(T) == 0, "特殊化されていない型のLoaderが呼ばれました。DefaultLoaderを特殊化してください。");
			return T();
		}
	};

	// テンプレート特殊化

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
}