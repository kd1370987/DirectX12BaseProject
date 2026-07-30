#pragma once

namespace Engine::Resource
{
	class Sound;

	/// <summary>
	/// サウンドデータに対して使用するときに発行されるインスタンス
	/// 使いたいオブジェクトが保持するデータ
	/// </summary>
	class SoundInstance
	{
	public:
		SoundInstance() = default;
		~SoundInstance();
		NON_COPYABLE_MOVABLE(SoundInstance);

		//==================================================================
		// インスタンス作成
		//==================================================================
		bool Init(const ResourceRef<Sound>& a_resourceRef, bool a_is3D = false);

		//==================================================================
		// 操作
		//==================================================================
		// ---- 再生 ----
		void Play(bool a_isLoop = false);									// 2D再生
		void Play(const DXSM::Vector3& a_pos, bool a_isLoop = false);		// 3D再生 : 再生座標が必要になる

		void Apply3D();														// Emitterの情報を適応

		// ---- 停止 ----
		void Stop();			// 停止
		void Pause();			// 一時停止
		void Resume();			// 再開

		// ---- 設定 ----
		void SetVolume(float a_vol);				// ボリューム設定 : 1.0f が 100%
		void SetPos(const DXSM::Vector3& a_pos);	// 3Dサウンド座標設定
		void SetCurveDistanceScaler(float a_val);	// 減衰倍率設定 : 1 = 通常 ... FLT_MIN～FLT_MAX

		// ---- 取得 ----
		bool IsPlay();			// 再生中か否か
		bool IsPause();			// 一時停止中か否か

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

		// DirectXサウンドエフェクトのポインタ取得
		DirectX::SoundEffect* RefSoundEffect() { return m_upSoundEffect.get(); }

	private:

		std::unique_ptr<DirectX::SoundEffect> m_upSoundEffect;
	};
}