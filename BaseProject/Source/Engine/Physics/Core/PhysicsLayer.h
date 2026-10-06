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
		inline constexpr uint32_t GROUP_BITS	= 7;
		inline constexpr uint32_t GROUP_MASK	= (1u << GROUP_BITS) - 1u;	// 0x7F
		inline constexpr uint32_t MOVING_BIT	= 1u << 7;
		inline constexpr uint32_t MASK_SHIFT	= 8;

		// group / mask に使えるビット(アプリのレイヤーはこの範囲に収めること)
		inline constexpr uint32_t ALL_GROUPS	= GROUP_MASK;

		constexpr JPH::ObjectLayer Make(uint32_t a_group, uint32_t a_mask, bool a_isMoving) noexcept
		{
			return static_cast<JPH::ObjectLayer>(
				(a_group & GROUP_MASK) |
				(a_isMoving ? MOVING_BIT : 0u) |
				((a_mask & GROUP_MASK) << MASK_SHIFT));
		}

		constexpr uint32_t GetGroup(JPH::ObjectLayer a_layer) noexcept { return a_layer & GROUP_MASK; }
		constexpr uint32_t GetMask(JPH::ObjectLayer a_layer) noexcept { return (a_layer >> MASK_SHIFT) & GROUP_MASK; }
		constexpr bool IsMoving(JPH::ObjectLayer a_layer) noexcept { return (a_layer & MOVING_BIT) != 0; }

		/// 値がビット幅に収まっているか(収まらない分は Make で黙って落ちる)
		constexpr bool IsInRange(uint32_t a_bits) noexcept { return (a_bits & ~GROUP_MASK) == 0; }
	}

	// ブロードフェーズの分け方 : 動かないもの / 動くもの の2本
	namespace EBroadPhaseLayer
	{
		inline constexpr JPH::BroadPhaseLayer STATIC{ 0 };
		inline constexpr JPH::BroadPhaseLayer DYNAMIC{ 1 };

		inline constexpr JPH::uint NUM_LAYERS = 2;
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
		uint32_t m_queryMask = Layer::ALL_GROUPS;
	};
}
