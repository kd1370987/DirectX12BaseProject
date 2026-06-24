#pragma once
namespace Engine::Resource
{
	class ModelLoader
	{
	public:
		static Model Load(const std::string& a_path);
	};
}