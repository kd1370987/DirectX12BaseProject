#pragma once

#include "Model/ModelLoader.h"
#include "Texture/TextureLoader.h"
#include "StateMachineAsset/StateMachineAssetLoader.h"
#include "Shader/ShaderLoader.h"
#include "Particles/ParticlesLoader.h"
#include "Material/MaterialLoader.h"
#include "Mesh/MeshLoader.h"
#include "Animation/AnimationLoader.h"

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
	// シェーダー
	template<>
	struct DefaultLoader<Shader>
	{
		static Shader LoadFromFile(const std::string& a_path)
		{
			return ShaderLoader::LoadShaderFromFile(a_path);
		}
	};
	// ライブラリシェーダー
	template<>
	struct DefaultLoader<ShaderLibrary>
	{
		static ShaderLibrary LoadFromFile(const std::string& a_path)
		{
			return ShaderLoader::LoadShaderLibraryFromFile(a_path);
		}
	};
	// パーティクル
	template<>
	struct DefaultLoader<ParticlesAsset>
	{
		static ParticlesAsset LoadFromFile(const std::string& a_path)
		{
			return ParticlesAssetLoader::LoadFromFile(a_path);
		}
	};
	// マテリアル
	template<>
	struct DefaultLoader<Material>
	{
		static Material LoadFromFile(const std::string& a_path)
		{
			return MaterialLoader::LoadFromFile(a_path);
		}
	};
	// メッシュ
	template<>
	struct DefaultLoader<Mesh>
	{
		static Mesh LoadFromFile(const std::string& a_path)
		{
			return MeshLoader::LoadFromFile(a_path);
		}
	};
	// アニメーション
	template<>
	struct DefaultLoader<AnimationData>
	{
		static AnimationData LoadFromFile(const std::string& a_path)
		{
			return AnimationLoader::LoadFromFile(a_path);
		}
	};
}