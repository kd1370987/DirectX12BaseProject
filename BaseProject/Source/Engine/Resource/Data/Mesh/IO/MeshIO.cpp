#include "MeshIO.h"

namespace Engine::Resource
{
	Mesh MeshIO::LoadFromFile(const std::string& a_path)
	{
		Mesh _mesh = {};
		_mesh.Load(a_path);

		return _mesh;
	}
}
