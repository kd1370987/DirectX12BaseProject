#include "AudioOption.h"

#include "../../Audio/AudioManager.h"

namespace
{
	// グループの並びに合わせた見出し
	constexpr const char* GROUP_LABEL[Engine::Audio::SOUND_GROUP_COUNT] =
	{
		"SE",
		"BGM",
		"UI",
		"Voice",
	};
}

void Engine::Option::ProjectOptions::AudioOption::Apply() const
{
	auto& _audioManager = Engine::Audio::AudioManager::Instance();

	_audioManager.SetMasterVolume(masterVolume);

	for (size_t _i = 0; _i < groupVolumeArray.size(); ++_i)
	{
		_audioManager.SetGroupVolume(static_cast<Engine::Audio::ESoundGroup>(_i), groupVolumeArray[_i]);
	}
}

void Engine::Option::ProjectOptions::AudioOption::DrawEdit()
{
	bool _isChanged = false;

	ImGui::SeparatorText("Master");

	if (ImGui::SliderFloat("Master", &masterVolume, 0.0f, 1.0f)) _isChanged = true;
	ImGui::TextDisabled("全部の音へ掛かる");

	ImGui::SeparatorText("Group");
	ImGui::TextDisabled("鳴らしている側を触らずに、そのグループだけ上下できる");

	for (size_t _i = 0; _i < groupVolumeArray.size(); ++_i)
	{
		ImGui::PushID(static_cast<int>(_i));

		if (ImGui::SliderFloat(GROUP_LABEL[_i], &groupVolumeArray[_i], 0.0f, 1.0f)) _isChanged = true;

		ImGui::PopID();
	}

	// 動かした瞬間に効かせる。鳴っている音にもその場で送り直される
	if (_isChanged) Apply();
}

void Engine::Option::ProjectOptions::AudioOption::Archive(Persistence::Archive& a_archive)
{
	a_archive.Field("masterVolume", masterVolume);

	// グループは並び順で保存する。
	// ※ ESoundGroup へ足すときは Count の手前へ(既存の値がずれる)
	for (size_t _i = 0; _i < groupVolumeArray.size(); ++_i)
	{
		a_archive.Field("groupVolume" + std::to_string(_i), groupVolumeArray[_i]);
	}
}
