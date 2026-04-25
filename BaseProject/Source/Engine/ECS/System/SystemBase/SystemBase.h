#pragma once

#include "../ISystem/ISystem.h"

namespace Engine::ECS
{
	template<typename System>
	class SystemBase : public ISystem
	{
	public:
		static constexpr ESystemType s_type = System::s_type;

		virtual void Init(World& a_world) override = 0;
	};
}
