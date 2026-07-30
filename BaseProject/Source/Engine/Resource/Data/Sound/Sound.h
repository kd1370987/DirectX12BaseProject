#pragma once

namespace Engine::Resource
{
	class Sound;

	class SoundInstance
	{
	public:
		SoundInstance(DirectX::SoundEffectInstance&& a_instance) { m_upSoundInstance = std::make_unique<DirectX::SoundEffectInstance>(std::move(a_instance)); }
		~SoundInstance() = default;
		NON_COPYABLE_MOVABLE(SoundInstance);

	private:

		// サウンド再生用インスタンス
		std::unique_ptr<DirectX::SoundEffectInstance> m_upSoundInstance = nullptr;

		// 自分の元データ
		ResourceRef<Sound> m_soundRef = {};

		// エミッター : 3Dサウンドリソースの定義など
		DirectX::AudioEmitter m_emitter;

		bool m_is3D = false;
	};

	/// <summary>
	/// サウンドデータ
	/// .wavなどに対して一対一で保持するデータ
	/// 直接使うのではなくて、インスタンスとして確保して使う
	/// </summary>
	class Sound
	{
	public:

		Sound() = default;
		Sound(DirectX::SoundEffect&& a_effect) { m_upSoundEffect = std::make_unique<DirectX::SoundEffect>(std::move(a_effect)); }
		~Sound() = default;
		NON_COPYABLE_MOVABLE(Sound);

	private:

		std::unique_ptr<DirectX::SoundEffect> m_upSoundEffect;
	};
}