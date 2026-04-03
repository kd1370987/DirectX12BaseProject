#pragma once

#include "../ISystem/ISystem.h"

namespace Engine::ECS
{
	template<typename System>
	class SystemBase : public ISystem
	{
	public:
		static constexpr ESystemType s_type = System::s_type;

		void Update(World& a_world, float a_dt) final
		{
			static_cast<System*>(this)->Run(a_world, a_dt);
		}
	};
}
