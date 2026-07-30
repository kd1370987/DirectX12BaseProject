#include "SoundIO.h"

#include "../../../../Audio/AudioManager.h"

namespace Engine::Resource
{
	Sound Engine::Resource::SoundIO::Load(const std::string& a_filePath)
	{
		auto* _pAudioEngine = Audio::AudioManager::Instance().RefAudioEngine();
		if (!_pAudioEngine) return Sound();

		auto _wFilePath = StringUtility::ToWideString(a_filePath);
		Sound _sound(std::move(DirectX::SoundEffect(_pAudioEngine, _wFilePath.c_str())));

		return _sound;
	}
}
