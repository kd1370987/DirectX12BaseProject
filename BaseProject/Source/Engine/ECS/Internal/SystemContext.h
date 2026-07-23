#pragma once

namespace Engine
{
	class MainEngine;
}
namespace Engine::Resource
{
	class ResourceManager;
}
namespace Engine::Input
{
	class InputManager;
}
namespace Engine::Editor
{
	class MainEditor;
}
namespace Engine::Raytracing
{
	class RayEngine;
}

namespace Engine::ECS
{
	class World;

	/// <summary>
	/// エンジン側のアプリケーションを立ち上げたら、終了まで解放されないクラス群
	///
	/// ここに足してよいのは「アプリ寿命」のものだけ。
	/// ワールド寿命のものは World::AddResource / GetResource を使うこと。
	/// </summary>
	struct EngineServices
	{
		MainEngine*					pMainEngine			= nullptr;
		Resource::ResourceManager*	pResourceManager	= nullptr;
		Input::InputManager*		pInputManager		= nullptr;
		Editor::MainEditor*			pMainEditor			= nullptr;
		Raytracing::RayEngine*		pRayEngine			= nullptr;
	};

	/// <summary>
	/// システム関数の引数に渡す構造体
	/// 主にシステム内で参照される、使われるデータを共通で送るためのもの
	///
	/// システムのラムダはこれ以外の状態を持たない(無捕獲)こと。
	/// 捕獲するとシーンをまたいで状態が残り、追いにくい不具合になる。
	/// </summary>
	struct SystemContext
	{
		World*			pWorld		= nullptr;		// ワールドの参照
		EngineServices* pServices	= nullptr;		// アプリ寿命
		float			dt			= 0.0f;			// デルタタイム
	};
}
