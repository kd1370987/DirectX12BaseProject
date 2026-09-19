#pragma once

namespace Engine::Physics
{
	//======================================================================================
	// PhysicsWorld に登録したボディの札
	//
	// コンポーネント(trivially copyable)に持たせるので Jolt の型は使わず、
	// JPH::BodyID の中身(uint32)をそのまま入れる。無効値も Jolt と同じ 0xFFFFFFFF。
	// 保存はしない(登録は Start で毎回やり直す)。
	//======================================================================================
	struct BodyHandle
	{
		static constexpr uint32_t kInvalid = 0xFFFFFFFFu;

		uint32_t id = kInvalid;

		bool IsValid() const noexcept { return id != kInvalid; }
	};
}
