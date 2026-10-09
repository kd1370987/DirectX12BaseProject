#include "SoundIO.h"

namespace Engine::Resource
{
	Sound Engine::Resource::SoundIO::Load(const std::string& a_filePath, DirectX::AudioEngine* a_pAudioEngine)
	{
		auto* _pAudioEngine = a_pAudioEngine;
		if (!_pAudioEngine) return Sound();

		auto _wFilePath = Core::String::ToWideString(a_filePath);
		Sound _sound(std::move(DirectX::SoundEffect(_pAudioEngine, _wFilePath.c_str())));

		return _sound;
	}
}
