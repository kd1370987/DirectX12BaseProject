#pragma once
namespace Engine::Resource
{
	template<typename T>
	class InstancePool
	{
	public:

	private:
		std::vector<T> m_data;
		Storage::HandleStorage m_handleStorage;
	};
}