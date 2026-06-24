#include "ModelLoader.h"

namespace Engine::Resource
{
	Model ModelLoader::Load(const std::string& a_path)
	{
		Model _model = {};
		_model.Import(a_path);
		return _model;
	}
}