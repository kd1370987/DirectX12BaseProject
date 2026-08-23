#pragma once

namespace Engine::Audio
{
	/// <summary>
	/// 3Dサウンドの「聞き手」の情報。
	/// 誰がリスナーなのかはエンジンは知らないので、持っている側(プレイヤー等)が
	/// 毎フレーム SubmitListener で送り込む。
	/// </summary>
	struct ListenerData
	{
		DXSM::Vector3 pos      = { 0.0f, 0.0f, 0.0f };	// ワールド座標
		DXSM::Vector3 front    = { 0.0f, 0.0f, 1.0f };	// 前方 : このエンジンは左手系で +Z が前
		DXSM::Vector3 up       = { 0.0f, 1.0f, 0.0f };	// 上方向
		DXSM::Vector3 velocity = { 0.0f, 0.0f, 0.0f };	// 速度(m/秒)。ドップラーに使う
	};

	/// <summary>
	/// 音関係を扱うシングルトンクラス、サウンドはここで読込要求が来るとロードしてインスタンスのみを返す
	/// </summary>
	class AudioManager
	{
	public:
		//----------------------------------------------------------------------------------------------------
		// 初期化・解放
		//----------------------------------------------------------------------------------------------------
		bool Init();

		/// <summary>
		/// 発行済みのサウンドインスタンスをすべて停止・破棄する
		/// DirectX::SoundEffectInstance は生成元の SoundEffect と AudioEngine を
		/// 参照しているため、それらより先にここで破棄しきる必要がある
		/// </summary>
		void ReleaseInstances();

		/// <summary>
		/// 全解放 : サウンドインスタンス → オーディオエンジン の順で破棄する
		/// </summary>
		void Release();

		//----------------------------------------------------------------------------------------------------
		// 更新
		//----------------------------------------------------------------------------------------------------

		/// <summary>
		/// 毎フレーム呼ぶこと
		/// 再生し終わったワンショットボイスの回収と、
		/// オーディオデバイスのロスト検出・復帰をここで行う
		/// </summary>
		void Update();

		//----------------------------------------------------------------------------------------------------
		// アクセサ
		//----------------------------------------------------------------------------------------------------
		DirectX::AudioEngine* RefAudioEngine() { return m_upAudioEngine.get(); }
		DirectX::AudioListener& RefAudioListner() { return m_listener; }

		//----------------------------------------------------------------------------------------------------
		// リスナー
		//----------------------------------------------------------------------------------------------------
		/// <summary>
		/// 聞き手の情報を更新する。リスナーを持つ側から毎フレーム呼ぶこと。
		/// 3D再生(Apply3D)は必ずここで設定された最新のリスナーを見る。
		/// 送らない間は初期値(原点で +Z 向き)のままなので、定位がおかしくなる。
		/// </summary>
		void SubmitListener(const ListenerData& a_data);

		const Resource::SoundInstance* GetInstance(const Handle<Resource::SoundInstance>& a_handle) const;
		Resource::SoundInstance* RefInstance(const Handle<Resource::SoundInstance>& a_handle);

		//----------------------------------------------------------------------------------------------------
		// サウンドインスタンスの発行
		//----------------------------------------------------------------------------------------------------
		/// <param name="a_is3D">
		/// true で3Dサウンド用インスタンスを発行する。
		/// Play3D / SetPos / Apply3D はこれを true にしたインスタンスでしか使えない
		/// </param>
		/// <param name="a_group">
		/// 音のグループ。オプションの音量はこの単位で掛かる。
		/// 省略すると効果音(Se)扱い
		/// </param>
		Handle<Resource::SoundInstance> RequestSoundInstance(
			const std::string& a_filePath, bool a_is3D = false,
			ESoundGroup a_group = ESoundGroup::Se);
		Handle<Resource::SoundInstance> RequestSoundInstance(
			const Engine::GUID& a_guid, bool a_is3D = false,
			ESoundGroup a_group = ESoundGroup::Se);

		//----------------------------------------------------------------------------------
		// 音量設定
		//----------------------------------------------------------------------------------

		/// <summary>
		/// マスター音量 : 全部の音へ掛かる
		/// </summary>
		/// <remarks>
		/// 変えた瞬間に鳴っているもの全部へ送り直す。
		/// 更新が止まっているシーンの音(ポーズ中のゲームBGMなど)にも効かせるため
		/// </remarks>
		void SetMasterVolume(float a_volume);
		float GetMasterVolume() const { return m_masterVolume; }

		// グループ音量 : そのグループの音へ掛かる
		void SetGroupVolume(ESoundGroup a_group, float a_volume);
		float GetGroupVolume(ESoundGroup a_group) const;

		/// <summary>
		/// そのグループの音へ掛ける倍率(マスター込み)
		/// </summary>
		/// <remarks>SoundInstance が実際に送る音量を出すのに使う</remarks>
		float CalcVolumeScale(ESoundGroup a_group) const;

		/// <summary>
		/// 発行したサウンドインスタンスを停止して破棄する
		/// プールはアプリ寿命なので、発行した側(コンポーネント等)が必ず返却すること
		/// </summary>
		/// <param name="a_handle">RequestSoundInstance が返したハンドル : 無効なら何もしない</param>
		void ReleaseSoundInstance(const Handle<Resource::SoundInstance>& a_handle);

	private:

		// 鳴っているものすべてへ音量を送り直す
		void RefreshAllVolume();

	private:

		// ---- メンバの宣言順が破棄順を決めるので入れ替えないこと ----
		// 破棄は宣言の逆順。SoundInstance が AudioEngine を参照しているため、
		// オーディオエンジンは必ず一番上(= 最後に破棄される位置)に置く。

		// オーディオエンジン
		std::unique_ptr<DirectX::AudioEngine> m_upAudioEngine = nullptr;

		// 3Dサウンドリスナー
		DirectX::AudioListener m_listener;

		// 現在再生中のサウンド管理リスト
		Pool::ItemPool<Resource::SoundInstance> m_soundInstancePool;

		//----------------------------------------------------------------------------------
		// 音量
		//
		// 保存はオプション側(AudioOption)。ここは実行中の値を持つだけで、
		// 起動時とエディターでの変更時にオプションから流し込まれる
		//----------------------------------------------------------------------------------
		float m_masterVolume = 1.0f;
		std::array<float, SOUND_GROUP_COUNT> m_groupVolumeArray = {};

	// シングルトン
	private:

		AudioManager();
		~AudioManager();
		NON_COPYABLE_NON_MOVABLE(AudioManager);

	public:

		static AudioManager& Instance()
		{
			static AudioManager _instance;
			return _instance;
		}

	};
}