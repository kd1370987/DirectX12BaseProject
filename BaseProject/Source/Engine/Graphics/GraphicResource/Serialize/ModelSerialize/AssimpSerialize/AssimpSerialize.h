#pragma once

struct AssimpModel;

namespace Serialize
{
	void Assimp(Engine::Resource::Model& a_dst,std::shared_ptr<AssimpModel> a_src,const std::string& a_fileDir);
}