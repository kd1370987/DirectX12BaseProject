#pragma once

namespace Engine::Pool
{
	template<typename T>
	class ItemPool
	{
	public:
		ItemPool() = default;
		~ItemPool() = default;

		
		void Reserve(size_t a_capacity);

	private:

		// 実体データ
		std::vector<T> m_data;
		std::vector<uint32_t> m_generations;
		std::vector<uint32_t> m_freeIndices;

	};
}