#pragma once

namespace Engine::Audio
{
	class AudioManager;
}

namespace Engine::Resource
{
	//==========================================================
	// 音の流れの区切り
	//
	// ブースターで言えば
	//   Start : 点火した瞬間の「ボッ」        (単発)
	//   Loop  : 噴射している間の「ゴーーー」  (鳴りっぱなし)
	//   End   : 止めた瞬間の「シュゥ」        (単発)
	//
	// ※ 値は保存されるので、増やすときは必ず Count の手前に足すこと
	//==========================================================
	enum class EAudioPhase : uint32_t
	{
		Start,
		Loop,
		End,

		Count
	};
	inline constexpr size_t AUDIO_PHASE_COUNT = static_cast<size_t>(EAudioPhase::Count);

	// 表示・ログ用のフェーズ名
	const char* ToString(EAudioPhase a_phase);

	//==========================================================
	// 1フェーズ分の定義
	//
	// アセット側(全員で共有する設計図)のデータ。
	// 音を設定していないフェーズは soundGUID が空のままで、
	// 鳴らす側はそのフェーズを丸ごと飛ばす
	//==========================================================
	struct SoundPart
	{
		// 音源データ
		Engine::GUID soundGUID = Engine::DefaultGUID;

		// 鳴らし方
		float vol = 1.0f;
		bool is3DSound = false;

		// 音が設定されているか : false なら鳴らす側は何もしない
		bool IsValid() const { return soundGUID != Engine::DefaultGUID; }

		void Archive(Persistence::Archive& a_ar);
	};

	//==========================================================
	// 実体側 : 鳴らすものが1つずつ持つ
	//
	// アセットはロード結果を全員で共有するので、
	// 再生用インスタンスをアセットに持たせると
	// 同じビヘイビアを使うエンティティ同士で音を奪い合う。
	// そのため実体はここに分けて、使う側(コンポーネント等)が持つ
	//==========================================================
	struct AudioBehaviorInstance
	{
		// フェーズごとの再生用インスタンス。
		// 音が設定されていないフェーズは無効ハンドルのまま
		Handle<SoundInstance> handles[AUDIO_PHASE_COUNT] = {};

		// 3D指定のパートを鳴らす位置
		Math::Vector3 pos = { 0.0f, 0.0f, 0.0f };

		// 始動してから終了するまでの間か。
		// Start / Loop で立ち、End で下りる。
		// End を「動いていたときだけ」1回鳴らすための判定に使う
		// (止まっている間も毎フレーム End が呼ばれる作りにできるようにするため)
		bool isActive = false;

		Handle<SoundInstance> GetHandle(EAudioPhase a_phase) const
		{
			return handles[static_cast<size_t>(a_phase)];
		}

		//------------------------------------------------------
		// 発行済みインスタンスに対しての操作
		//
		// どれもアセットの中身を見ないので、
		// アセットが読めていない・破棄された後でも呼べる
		//------------------------------------------------------

		// 全フェーズを止める(Endも鳴らさない)
		void StopAll(Engine::Audio::AudioManager& a_audioManager);

		// 3D再生の位置を更新する。鳴っている音にも即時反映される
		void SetPos(Engine::Audio::AudioManager& a_audioManager, const Math::Vector3& a_pos);

		// 発行済みインスタンスをすべて返却して空にする
		void Release(Engine::Audio::AudioManager& a_audioManager);
	};

	//==========================================================
	// 音の組み合わせを表現
	//
	// ブースターの射出音と、継続発射中の音、終了時の音という
	// 音の流れやデータを一つのアセットとして管理
	//
	// 鳴らす側はどのファイルが何番目かを知らなくてよく、
	// 「始動した」「続いている」「終わった」だけを伝える
	//==========================================================
	class AudioBehavior
	{
	public:
		AudioBehavior() = default;
		explicit AudioBehavior(const std::string& a_name) : m_name(a_name) {}
		~AudioBehavior() = default;
		NON_COPYABLE_MOVABLE(AudioBehavior);

		//------------------------------------------------------
		// 定義へのアクセス
		//------------------------------------------------------
		const SoundPart& GetPart(EAudioPhase a_phase) const { return m_parts[static_cast<size_t>(a_phase)]; }
		SoundPart& RefPart(EAudioPhase a_phase) { return m_parts[static_cast<size_t>(a_phase)]; }

		// そのフェーズに音が設定されているか
		bool HasPart(EAudioPhase a_phase) const { return GetPart(a_phase).IsValid(); }

		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& a_name) { m_name = a_name; }

		//------------------------------------------------------
		// 保存・読み込み
		//------------------------------------------------------
		void Archive(Persistence::Archive& a_ar);

		// 拡張子なしのベースパスへ保存する
		void Save(const std::string& a_baseFilePath);

		//------------------------------------------------------
		// 実行
		//
		// 鳴らす対象は使う側が持っている実体(AudioBehaviorInstance)。
		// 音が設定されていないフェーズは何もしないので、
		// 呼ぶ側で「設定されているか」を確かめる必要はない
		//------------------------------------------------------

		/// <summary>
		/// 定義に沿って再生用インスタンスを発行する
		/// </summary>
		/// <remarks>
		/// すでに発行済みなら一度返してから作り直すので、二重発行にはならない。
		/// 音が設定されていないフェーズのハンドルは無効のままになる
		/// </remarks>
		void CreateInstance(Engine::Audio::AudioManager& a_audioManager, AudioBehaviorInstance& a_inst) const;

		// 始動時 : 頭から鳴らし直す(単発想定)
		void Start(Engine::Audio::AudioManager& a_audioManager, AudioBehaviorInstance& a_inst) const;

		// 継続中 : 止まっていたら鳴らす。毎フレーム呼んでよい
		void Loop(Engine::Audio::AudioManager& a_audioManager, AudioBehaviorInstance& a_inst) const;

		// 終了時 : 継続音を止めて終了音を鳴らす。
		// 始動していなければ何もしないので、
		// 止まっている状態で毎フレーム呼んでも終了音は鳴り続けない
		void End(Engine::Audio::AudioManager& a_audioManager, AudioBehaviorInstance& a_inst) const;

	private:

		// 指定フェーズを鳴らす。設定されていなければ false
		bool PlayPart(
			Engine::Audio::AudioManager& a_audioManager,
			AudioBehaviorInstance& a_inst,
			EAudioPhase a_phase,
			bool a_isLoop
		) const;

	private:

		// 識別子
		std::string m_name;

		// フェーズごとの定義
		SoundPart m_parts[AUDIO_PHASE_COUNT] = {};
	};
}
