#pragma once

struct GLTFModel;
namespace Engine::Resource
{
	struct Model;
}

namespace Serialize
{
	void TinyGLTF(Engine::Resource::Model& a_dst,std::shared_ptr<GLTFModel> a_src,const std::string& a_fileDir);
}