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

		// 体の並び。頭(リーダーの直後)から尾の順。ボイドを切り離す攻撃が使う
		const std::vector<Engine::ECS::Entity>* pPlatoonLeaders = nullptr;
		float wormLength = 0.0f;												// 頭から尾までの長さ(1次元。m)
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
