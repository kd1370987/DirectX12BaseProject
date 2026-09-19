#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

//==========================================================================================
// Jolt のレイヤーの詰め方
//
// エンジンはアプリの Layer(StaticObject / PlayerProjectile ...)を知らないので、
// ビット列のまま受け取って JPH::ObjectLayer(16bit)へ詰める。
//
//   bit  0- 6 : group  … 自分が属するレイヤー(アプリの ColliderComponent::layer)
//   bit  7    : moving … 動くボディか(ブロードフェーズの振り分けに使う)
//   bit  8-14 : mask   … 当たりに行く相手(アプリの ColliderComponent::collideLayer)
//   bit 15    : 未使用(0xFFFF は Jolt の無効値なので使わない)
//
// 以前の自作判定にあった「layer が 0 なら弾かずに通す」は引き継がない。
// group が 0 のボディはどのクエリにも当たらない。アセット上 layer=0 は1件も無く、
// あの特例は静的登録で layer を入れ忘れていた時代の保険だったため。
//==========================================================================================
namespace Engine::Physics
{
	namespace Layer
	{
		inline constexpr uint32_t kGroupBits	= 7;
		inline constexpr uint32_t kGroupMask	= (1u << kGroupBits) - 1u;	// 0x7F
		inline constexpr uint32_t kMovingBit	= 1u << 7;
		inline constexpr uint32_t kMaskShift	= 8;

		// group / mask に使えるビット(アプリのレイヤーはこの範囲に収めること)
		inline constexpr uint32_t kAllGroups	= kGroupMask;

		constexpr JPH::ObjectLayer Make(uint32_t a_group, uint32_t a_mask, bool a_isMoving) noexcept
		{
			return static_cast<JPH::ObjectLayer>(
				(a_group & kGroupMask) |
				(a_isMoving ? kMovingBit : 0u) |
				((a_mask & kGroupMask) << kMaskShift));
		}

		constexpr uint32_t GetGroup(JPH::ObjectLayer a_layer) noexcept { return a_layer & kGroupMask; }
		constexpr uint32_t GetMask(JPH::ObjectLayer a_layer) noexcept { return (a_layer >> kMaskShift) & kGroupMask; }
		constexpr bool IsMoving(JPH::ObjectLayer a_layer) noexcept { return (a_layer & kMovingBit) != 0; }

		// 値がビット幅に収まっているか(収まらない分は Make で黙って落ちる)
		constexpr bool IsInRange(uint32_t a_bits) noexcept { return (a_bits & ~kGroupMask) == 0; }
	}

	// ブロードフェーズの分け方 : 動かないもの / 動くもの の2本
	namespace EBroadPhaseLayer
	{
		inline constexpr JPH::BroadPhaseLayer Static{ 0 };
		inline constexpr JPH::BroadPhaseLayer Dynamic{ 1 };

		inline constexpr JPH::uint NumLayers = 2;
	}

	//--------------------------------------------------------------------------------------
	// クエリ用のレイヤーフィルター
	//
	// 以前の自作判定と同じく「クエリ側のマスク」で相手を選ぶ。
	// ボディ側の mask(相手から見て当たりに行きたいか)は見ない。
	// シミュレーション同士の組み合わせは PysicsObjectLayerPairFilter が両方向で見る
	//--------------------------------------------------------------------------------------
	class LayerMaskQueryFilter final : public JPH::ObjectLayerFilter
	{
	public:
		explicit LayerMaskQueryFilter(uint32_t a_queryMask) noexcept : m_queryMask(a_queryMask) {}

		bool ShouldCollide(JPH::ObjectLayer a_layer) const override
		{
			return (Layer::GetGroup(a_layer) & m_queryMask) != 0;
		}

	private:
		uint32_t m_queryMask = Layer::kAllGroups;
	};
}
