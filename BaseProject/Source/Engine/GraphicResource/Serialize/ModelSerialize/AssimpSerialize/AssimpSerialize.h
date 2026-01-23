#pragma once

class Model;
class AssimpModel;

namespace Serialize
{
	void Assimp(Model& a_dst,std::shared_ptr<AssimpModel> a_src,const std::string& a_fileDir);
}