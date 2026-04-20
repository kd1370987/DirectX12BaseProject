#pragma once
namespace Engine::ECS
{
	class EntitySerializeField
	{
	public:

		// 保存
		void Serialize(std::string a_path);

		// 読み込み
		void Deserialize(std::string a_path);

	private:
		std::string m_path;
	};
}