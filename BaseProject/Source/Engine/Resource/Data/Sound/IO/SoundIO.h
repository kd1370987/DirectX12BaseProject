#pragma once
namespace Engine::Resource
{
	class SoundIO
	{
	public:

		// a_pAudioEngine : SoundEffect を作る先。nullptr なら空のサウンドを返す
		static Sound Load(const std::string& a_filePath, DirectX::AudioEngine* a_pAudioEngine);
	};
}