#include "AnimationIO.h"

namespace Engine::Resource
{

	AnimationData AnimationIO::LoadFromFile(const std::string& a_path)
	{
		AnimationData _animData = {};
		_animData.Load(a_path);
		return _animData;
	}
}
