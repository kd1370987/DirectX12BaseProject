#pragma once

namespace Engine::GameObject
{
	struct ObjectContext;
}

namespace Engine::Persistence
{
	class Archive;
}

namespace App::Object
{
	class SwarmBossStateMachine;

	/// <summary>
	/// ステートに渡すもの
	///
	/// ステートはコントローラーを直接見ない。要るものはここに載せて毎フレーム組む
	/// </summary>
	struct SwarmBossStateContext
	{
		Engine::GameObject::ObjectContext* pObject = nullptr;	// dt / ワールド
		SwarmBossStateMachine* pMachine = nullptr;				// 切り替え要求の出し先

		Engine::ECS::Entity leaderEntity = Engine::ECS::Limits::INVALID_ENTITY;	// 入力を書き込む相手
		Math::Vector3 spawnPos = {};											// 行動範囲の中心(ワールド)
	};

	class IState
	{
	public:
		virtual ~IState() = default;

		virtual void Enter(SwarmBossStateContext& a_context) = 0;
		virtual void Update(SwarmBossStateContext& a_context) = 0;
		virtual void Exit(SwarmBossStateContext& a_context) = 0;

		//------------------------------------------------------------------------------------------
		// 調整値の保存 / 表示。持たないステートは何もしない
		//------------------------------------------------------------------------------------------
		virtual void Archive(Engine::Persistence::Archive& a_ar) {}
		virtual void DrawInspector() {}
	};
}
