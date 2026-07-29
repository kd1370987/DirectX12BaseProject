#include "MeshIO.h"

#include "../../../Common/ScopedResourceBuild.h"

namespace Engine::Resource
{
	Mesh MeshIO::LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
	{
		// コンテキストを渡されていればそれに積み、なければその場でバッチを開く
		ResourceBuildScope _scope(a_pContext);

		Mesh _mesh = {};
		_mesh.Load(_scope.GetContext(), a_path);

		return _mesh;
	}
}
