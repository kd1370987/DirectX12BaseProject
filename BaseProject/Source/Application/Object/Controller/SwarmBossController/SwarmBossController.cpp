#include "SwarmBossController.h"

// App
#include "../../../ECS/World/APPWorld.h"

#include "../../../Components/Transform/LocalTransformComponent.h"
#include "../../../Components/Force/MovementComponent.h"
#include "../../../Components/Force/VelocityComponent.h"

// エンジン
#include "Engine/ECS/Internal/SystemContext.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
namespace App::Object
{
	void SwarmBossController::PostDeserialize(Engine::GameObject::ObjectContext& a_context)
	{}
	void SwarmBossController::Awake(Engine::GameObject::ObjectContext & a_context)
	{
		// リーダー生成
		CreateLeader(a_context);

	}
	void SwarmBossController::Start(Engine::GameObject::ObjectContext& a_context)
	{}
	void SwarmBossController::Update(Engine::GameObject::ObjectContext& a_context)
	{

	}
	bool SwarmBossController::CreateLeader(Engine::GameObject::ObjectContext& a_context)
	{
		// リーダー生成
		auto* _pPrefab = a_context.pServices->pResourceManager->Get(m_leaderPrefabHandle);
		if (!_pPrefab) 
		{
			ENGINE_ERRLOG(false,"リーダー生成用のプレハブが設定されていません");
			return false;
		}

		// プレハブのインスタンス取得
		auto _instanceVec = _pPrefab->BuildInstanceData(a_context.pWorld);
		if (_instanceVec.empty()) 
		{
			ENGINE_ERRLOG(false, "リーダー生成用のプレハブの取得に失敗");
			return false;
		}

		// リーダーに必須なコンポーネントを付与 : すでにあればスキップ
		auto _ltID = a_context.pWorld->GetCompTypeID<LocalTransformComponent>();
		auto _mID = a_context.pWorld->GetCompTypeID<MovementComponent>();
		auto _vID = a_context.pWorld->GetCompTypeID<VelocityComponent>();

		// リーダー生成

	}
	bool SwarmBossController::CreatePlatoonLeaders(Engine::GameObject::ObjectContext& a_context)
	{
		// プレハブ取得
		auto* _pPrefab = a_context.pServices->pResourceManager->Get(m_platoonPrefab);
		if (!_pPrefab)
		{
			ENGINE_ERRLOG(false, "小隊長生成用のプレハブが設定されていません");
			return false;
		}

		// プレハブのインスタンス取得
		auto _instanceVec = _pPrefab->BuildInstanceData(a_context.pWorld);
		if (_instanceVec.empty())
		{
			ENGINE_ERRLOG(false, "小隊長生成用のプレハブの取得に失敗");
			return false;
		}

		// 必須なコンポーネントを付与 : すでにあればスキップ
		auto _ltID = a_context.pWorld->GetCompTypeID<LocalTransformComponent>();
		auto _mID = a_context.pWorld->GetCompTypeID<MovementComponent>();
		auto _vID = a_context.pWorld->GetCompTypeID<VelocityComponent>();

		// 最大数分小隊長を生成
		for (uint32_t _i = 0; _i < m_maxPlatoonLeader; ++_i)
		{
			// 初期化時に一つ前の小隊長のIDをコンポーネントに覚えさせる
			// 初めの小隊長はリーダーのEntityIDを覚えさせる
		}
	}
}