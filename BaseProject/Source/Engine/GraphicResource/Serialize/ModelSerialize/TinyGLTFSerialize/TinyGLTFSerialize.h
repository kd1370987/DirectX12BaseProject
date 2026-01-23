#pragma once

class Model;
class GLTFModel;

namespace Serialize
{
	void TinyGLTF(Model& a_dst,std::shared_ptr<GLTFModel> a_src,const std::string& a_fileDir);
}