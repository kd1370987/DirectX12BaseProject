#include "MeshLoader.h"

namespace Engine::Resource
{
	Mesh MeshLoader::LoadFromFile(const std::string& a_path)
	{
		Mesh _mesh = {};
		_mesh.Load(a_path);

		return _mesh;
	}
}
