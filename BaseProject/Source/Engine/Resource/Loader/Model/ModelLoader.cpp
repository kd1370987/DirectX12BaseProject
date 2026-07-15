#include "ModelLoader.h"

#include "../../Data/Model/IO/ModelIO.h"

namespace Engine::Resource
{
	Model ModelLoader::Load(const std::string& a_path)
	{
		return ModelIO::Import(a_path);
	}
}