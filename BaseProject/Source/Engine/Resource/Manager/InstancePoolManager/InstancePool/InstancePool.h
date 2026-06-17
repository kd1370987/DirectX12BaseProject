#pragma once
namespace Engine::Resource
{
	template<typename T>
	class InstancePool
	{
	public:

		InstancePool() = default;
		~InstancePool() = default;


		void Init(UINT a_maxCount);

	private:
		std::vector<T> m_data;
		FreeRange m_freeRange;
	};
}