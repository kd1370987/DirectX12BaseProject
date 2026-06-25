#include "AnimationLoader.h"

namespace Engine::Resource
{

	AnimationData AnimationLoader::LoadFromFile(const std::string& a_path)
	{
		AnimationData _animData = {};
		_animData.Load(a_path);
		return _animData;
	}
}
