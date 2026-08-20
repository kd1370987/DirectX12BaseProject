#pragma once

#include "../Data/Model/IO/ModelIO.h"
#include "../Data/Texture/IO/TextureIO.h"
#include "../Data/AnimatorAsset/IO/AnimatorAssetIO.h"
#include "../Data/ActionStateMachineAsset/IO/ActionStateMachineAssetIO.h"
#include "../Data/Shader/IO/ShaderIO.h"
#include "../Data/Particles/IO/ParticlesIO.h"
#include "../Data/Material/IO/MaterialIO.h"
#include "../Data/Mesh/IO/MeshIO.h"
#include "../Data/Animation/IO/AnimationIO.h"
#include "../Data/ShadingModelTable/IO/ShadingModelTableIO.h"
#include "../Data/Prefab/Prefab.h"
#include "../Data/Sound/IO/SoundIO.h"
#include "../Data/AudioBehavior/IO/AudioBehaviorIO.h"
#include "../Data/EffectAsset/IO/EffectAssetIO.h"
namespace Engine::Resource
{
	// ロード処理の中間用クラス
	//
	// LoadFromFile は必ずビルドコンテキストを受け取る形にしてある。
	// GPUリソースを作らないアセットは無視してよいが、
	// 作るアセット(Model/Mesh/Texture)は必ずコンテキスト側のコマンドリストへ積むこと。
	template<typename T>
	struct DefaultLoader
	{
		static T LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			// 特殊化されていない型で LoadImmediate<T> が呼ばれたらコンパイルを止める
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
		static Model LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			// ModelLoaderクラスの専用処理
			return ModelIO::Import(a_path, a_pContext);
		}
	};
	// テクスチャ
	template<>
	struct DefaultLoader<Texture>
	{
		static Texture LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return TextureIO::LoadFromFile(a_path);
		}
	};
	// アニメーター
	template<>
	struct DefaultLoader<AnimatorAsset>
	{
		static AnimatorAsset LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return AnimatorAssetIO::LoadFromFile(a_path);
		}
	};
	// ゲームプレイ用ステートマシン
	template<>
	struct DefaultLoader<ActionStateMachineAsset>
	{
		static ActionStateMachineAsset LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return ActionStateMachineAssetIO::LoadFromFile(a_path);
		}
	};
	// シェーダー
	template<>
	struct DefaultLoader<Shader>
	{
		static Shader LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return ShaderIO::LoadShaderFromFile(a_path);
		}
	};
	// パーティクル
	template<>
	struct DefaultLoader<ParticlesAsset>
	{
		static ParticlesAsset LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return ParticlesAssetIO::LoadFromFile(a_path);
		}
	};
	// マテリアル
	template<>
	struct DefaultLoader<Material>
	{
		static Material LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return MaterialIO::LoadFromFile(a_path);
		}
	};
	// メッシュ
	template<>
	struct DefaultLoader<Mesh>
	{
		static Mesh LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return MeshIO::LoadFromFile(a_path, a_pContext);
		}
	};
	// アニメーション
	template<>
	struct DefaultLoader<AnimationData>
	{
		static AnimationData LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return AnimationIO::LoadFromFile(a_path);
		}
	};
	// シェーディングモデル
	template<>
	struct DefaultLoader<ShadingModelTable>
	{
		static ShadingModelTable LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return ShadingModelTableIO::LoadFromFile(a_path);
		}
	};
	// プレハブ
	template<>
	struct DefaultLoader<Prefab>
	{
		static Prefab LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return Prefab::LoadFromFile(a_path);
		}
	};
	// サウンド
	template<>
	struct DefaultLoader<Sound>
	{
		static Sound LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return SoundIO::Load(a_path);
		}
	};
	// オーディオビヘイビア
	template<>
	struct DefaultLoader<AudioBehavior>
	{
		static AudioBehavior LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return AudioBehaviorIO::LoadFromFile(a_path);
		}
	};
	// エフェクト
	template<>
	struct DefaultLoader<EffectAsset>
	{
		static EffectAsset LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
		{
			return EffectAssetIO::LoadFromFile(a_path);
		}
	};
}
