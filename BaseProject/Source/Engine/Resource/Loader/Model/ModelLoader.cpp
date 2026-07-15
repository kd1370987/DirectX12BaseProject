#include "ModelLoader.h"

#include "../../Data/Model/IO/ModelIO.h"

namespace Engine::Resource
{
	Model ModelLoader::Load(const std::string& a_path)
	{
		//Model _model = {};
		//_model.Import(a_path);
		//return _model;

		Model _model = std::move(ModelIO::Import(a_path));
		_model.TestParse();
		return _model;
	}
}