#pragma once

#include "Engine/Utility/Math/Vector/Vector3.h"

namespace JPH
{
	class PhysicsSystem;
	class BroadPhaseLayerInterface;
	class ObjectVsBroadPhaseLayerFilter;
	class ObjectLayerPairFilter;
}

namespace Engine::Physics
{
	class PhysicsEngine;

	// PhysicsWorld を作るときの大きさ。
	// PhysicsSystem::Init で先に確保されるので、プレビュー用のワールドは小さくしておく
	struct PhysicsWorldDesc
	{
		uint32_t maxBodies				= 65536;	// ボイド4000体 + 地形 + 弾 を余裕で収める
		uint32_t maxBodyPairs			= 65536;
		uint32_t maxContactConstraints	= 10240;
		Math::Vector3 gravity			= { 0.0f, -9.8f, 0.0f };

		// エフェクトエディターのプレビュー用
		static PhysicsWorldDesc MakePreview() noexcept
		{
			PhysicsWorldDesc _desc;
			_desc.maxBodies = 1024;
			_desc.maxBodyPairs = 1024;
			_desc.maxContactConstraints = 1024;
			return _desc;
		}
	};

	//======================================================================================
	// シーン(ECSワールド)ごとの物理空間
	//
	// CollisionWorld と同じく World のリソースとして持つ(CreateSceneWorld で足す)。
	// システムからは a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>() で引く。
	//
	// Jolt の型は外へ出さない。ボディの出し入れやクエリはここに口を足していく
	//======================================================================================
	class PhysicsWorld
	{
	public:

		// a_pEngine : Jolt 全体の持ち主(借り物)。このワールドより長生きすること
		PhysicsWorld(PhysicsEngine* a_pEngine, const PhysicsWorldDesc& a_desc);
		~PhysicsWorld();
		NON_COPYABLE_NON_MOVABLE(PhysicsWorld);

		// 1ステップ進める。判定クエリ(Physics フェーズ)の前に呼ぶ
		void Update(float a_dt);

		// 登録されているボディの数(確認用)
		uint32_t GetBodyCount() const;

		bool IsValid() const { return m_upPhysicsSystem != nullptr; }

	private:

		PhysicsEngine* m_pEngine = nullptr;

		// PhysicsSystem はレイヤー定義を参照で持つので、こちらを先に宣言する
		// (メンバーは宣言の逆順に壊れる = PhysicsSystem が先に消える)
		std::unique_ptr<JPH::BroadPhaseLayerInterface> m_upBroadPhaseLayerInterface;
		std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_upObjectVsBroadPhaseLayerFilter;
		std::unique_ptr<JPH::ObjectLayerPairFilter> m_upObjectLayerPairFilter;

		std::unique_ptr<JPH::PhysicsSystem> m_upPhysicsSystem;
	};
}
