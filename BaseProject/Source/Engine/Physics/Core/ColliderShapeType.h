#pragma once

namespace Engine::Physics
{
	//======================================================================================
	// コライダーの形状の種類(アプリの ColliderComponent::shapeType)
	//
	// シーン・プレハブに保存されている値なので、並びと名前は変えないこと。
	// ボディの形はこれで決まる : Mesh は判定メッシュ(COL ノード)、それ以外は描画メッシュのAABBの箱
	// (Sphere/Box/Capsule の寸法は持っていない。PhysicsWorld::EModelBodyShape を参照)
	//======================================================================================
	enum class EShapeType : uint32_t
	{
		Sphere,
		Box,
		Capsule,
		Mesh,
	};
}
