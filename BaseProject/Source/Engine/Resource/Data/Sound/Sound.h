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
		/// <param name="a_is3D">
		/// 3Dサウンドとして作成するか。
		/// false で作ったインスタンスに Play3D / SetPos / Apply3D は使えない
		/// (DirectXTK が例外を投げるため、こちら側で弾いている)
		/// </param>
		bool Init(const ResourceRef<Sound>& a_resourceRef, bool a_is3D = false);

		//==================================================================
		// 操作
		//==================================================================
		// ---- 再生 ----
		void Play(bool a_isLoop = false);									// 2D再生
		void Play3D(const DXSM::Vector3& a_pos, bool a_isLoop = false);		// 3D再生 : 再生座標が必要になる

		void Apply3D();														// Emitterの情報を適応

		// ---- 停止 ----
		void Stop();			// 停止
		void Pause();			// 一時停止
		void Resume();			// 再開

		// ---- 設定 ----

		/// <summary>
		/// ボリューム設定 : 1.0f が 100%
		/// </summary>
		/// <remarks>
		/// ここへ渡すのは「呼び出し側の都合だけで決めた音量」。
		/// 実際に鳴るのは これ × グループ音量 × マスター音量 になる。
		/// 設定画面の値を掛けてから渡す必要はない
		/// </remarks>
		void SetVolume(float a_vol);

		void SetPos(const DXSM::Vector3& a_pos);	// 3Dサウンド座標設定
		void SetCurveDistanceScaler(float a_val);	// 減衰倍率設定 : 1 = 通常 ... FLT_MIN～FLT_MAX

		//==================================================================
		// グループ
		//==================================================================

		// どのグループの音か。オプションの音量はこの単位で掛かる
		void SetGroup(Audio::ESoundGroup a_group);
		Audio::ESoundGroup GetGroup() const { return m_group; }

		/// <summary>
		/// グループ音量・マスター音量を掛け直して送り直す
		/// </summary>
		/// <remarks>
		/// 設定が変わったときに AudioManager がまとめて呼ぶ。
		/// 鳴らしている側は自分では気付けない(更新が止まっているシーンもある)ため
		/// </remarks>
		void RefreshVolume();

		// ---- 取得 ----
		bool IsPlay();			// 再生中か否か
		bool IsPause();			// 一時停止中か否か
		bool Is3D() const { return m_is3D; }	// 3Dサウンドとして作られたか

	private:

		// サウンド再生用インスタンス
		std::unique_ptr<DirectX::SoundEffectInstance> m_upSoundInstance = nullptr;

		// 自分の元データ
		ResourceRef<Sound> m_soundRef = {};

		// エミッター : 3Dサウンドリソースの定義など
		DirectX::AudioEmitter m_emitter;

		bool m_is3D = false;

		// 呼び出し側が指定した音量。実際に送るときはグループとマスターを掛ける
		float m_volume = 1.0f;

		// どのグループの音か
		Audio::ESoundGroup m_group = Audio::ESoundGroup::Se;
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