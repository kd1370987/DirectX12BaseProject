#pragma once

#include "../../Engine/GameObject/BaseObject/BaseObject.h"

namespace Engine::Audio { class AudioManager; }

namespace App::Object
{
	//======================================================================================
	// シーケンスが持つBGM
	//
	// 進行役(TitleSequence / HomeSequence / ResultSequence / SceneSequence / PauseSequence)が
	// 1つずつ持つ。鳴らす・止める・フェードイン・音量の絞りをまとめてある。
	//
	// ・鳴らし始めるのは Update
	//     Init はシーンの読み込み(Archive)より先に走るので、そこではまだ
	//     どの曲を鳴らすか決まっていない。
	//
	// ・シーンが変わればその曲は止まる
	//     借りたインスタンスを持ち主の Release で返すため。
	//     タイトルとホームで同じ曲を指定しても、切り替わりで頭から鳴り直す。
	//     繋げたいなら「今鳴っているBGM」をアプリ寿命で1つ持つ作りが要る。
	//
	// ・ポーズ中の絞り(ダッキング)は全体で1つ
	//     ポーズ画面はゲームのシーンへ重ねて出るので、下のゲームの進行役へは
	//     手が届かない(ObjectContext が運ぶのは自分のシーンの分だけ)。
	//     しかも重ねている間、下のシーンは更新されない = 下のBGMは自分では音量を
	//     送り直せない。そのため生存中のBGMを静的な一覧で持ち、
	//     絞りを変えた瞬間にこちらから送り込む。
	//     ポーズ自身のBGMは対象から外すこと(SetDuckTarget(false))。
	//======================================================================================
	class SequenceBgm
	{
	public:

		// 生存中の一覧へ載せる/外すので、アドレスが変わる操作は禁止する
		SequenceBgm();
		~SequenceBgm();
		NON_COPYABLE_NON_MOVABLE(SequenceBgm);

		/// <summary>
		/// 毎フレーム呼ぶ : 初回で鳴らし始め、フェードと音量を進める
		/// </summary>
		void Update(Engine::GameObject::ObjectContext& a_context);

		/// <summary>
		/// 借りたインスタンスを返す : 持ち主の Release から必ず呼ぶこと
		/// </summary>
		/// <remarks>プールはアプリ寿命なので、返さないとシーンを跨いで溜まり続ける</remarks>
		void Release(Engine::GameObject::ObjectContext& a_context);

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar);

		// インスペクター
		void DrawInspector(Engine::GameObject::ObjectContext& a_context);

		/// <summary>
		/// 全体の絞りを受けるか
		/// </summary>
		/// <remarks>ポーズ自身のBGMは false にする。自分で絞ってしまうため</remarks>
		void SetDuckTarget(bool a_isDuckTarget);

		// 鳴らしているか
		bool IsPlaying() const { return m_isStarted; }

		//----------------------------------------------------------------------------------
		// 全体の絞り(ダッキング)
		//----------------------------------------------------------------------------------

		/// <summary>
		/// 鳴っているBGMをまとめて小さくする
		/// </summary>
		/// <param name="a_scale">掛ける倍率(1で素のまま)</param>
		/// <remarks>
		/// 呼んだ瞬間に生存中のBGMへ送り込む。更新が止まっているシーンのBGMにも効かせるため。
		/// 立てた側が必ず 1.0f へ戻すこと。戻し忘れると、
		/// 次に絞りを解除するまで小さいままになる
		/// </remarks>
		static void SetGlobalDuck(float a_scale);
		static float GetGlobalDuck();

	private:

		// 今送るべき音量を出す
		float CalcVolume() const;

		// 今の音量を送る(変わっていなければ何もしない)
		void ApplyVolume();

	private:

		//-------------------------------------------------------------------
		// 設定(保存される)
		//-------------------------------------------------------------------
		Engine::GUID m_guid = {};		// 鳴らす曲。未設定なら何もしない
		float m_volume = 0.5f;			// 音量
		float m_fadeInTime = 1.0f;		// 鳴り始めに音量を上げきるまでの時間(秒)。0で即時
		bool m_isLoop = true;			// 繰り返すか
		bool m_isDuckTarget = true;		// 全体の絞りを受けるか

		//-------------------------------------------------------------------
		// 状態(保存しない)
		//-------------------------------------------------------------------
		Engine::Handle<Engine::Resource::SoundInstance> m_handle = {};

		// 音量を送る先。更新が止まっている間も送れるよう、Update で受け取ったものを覚えておく
		Engine::Audio::AudioManager* m_pAudioManager = nullptr;

		float m_fadeTime = 0.0f;		// フェードインの経過(秒)
		bool m_isStarted = false;		// 鳴らし始めたか
		bool m_isFailed = false;		// 発行に失敗した : 毎フレーム読み直さないための印

		// 最後に送った音量。変わったときだけ送る
		float m_appliedVolume = -1.0f;
	};
}
