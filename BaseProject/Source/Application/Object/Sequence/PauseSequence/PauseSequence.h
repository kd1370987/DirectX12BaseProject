#pragma once

#include "../../../../Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	/// <summary>
	/// ポーズ画面の進行役 : ポーズ画面のシーンに一つ置く
	/// </summary>
	/// <remarks>
	/// ポーズ画面はゲームのシーンへ**重ねて**出す(Push)。切り替えではないので
	/// 後ろのシーンは消えず、閉じればそのまま続きから動き出す。
	///
	///   重ねるのは  : ゲーム側の SceneSequence(ポーズ画面のシーンを設定しておく)
	///   閉じるのは  : ここ(Pop)
	///
	/// 更新されるのは一番上のシーンだけなので、重ねている間、後ろのゲームは
	/// 描画だけが続いて止まっている(SceneManager::SetUpdateTopSceneOnly)。
	/// 「止める」ための旗はどこにも要らない。
	///
	/// ・ボタン(UIButton)はシーンへ置いて GUID で引く。押されて何をするかはここが
	///   差し込むので、ボタン側はポーズの知識を持たない(タイトル・ホームと同じ作り)。
	/// ・ポーズ中はカーソルの中央固定を切る(固定したままだとボタンを狙えない)。
	/// ・ゲームをやめて別のシーンへ移るときは、**先にポーズを外してから**差し替える。
	///   重ねたまま差し替えると、入れ替わるのは後ろのゲームの方で、
	///   ポーズ画面が乗りっぱなしになる。
	/// </remarks>
	class PauseSequence : public Engine::GameObject::BaseObject
	{
	public:

		// 更新処理 : ボタンへの差し込みと、閉じる入力の受け取り
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// 解放処理 : カーソル固定を設定値へ戻す
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "PauseSequence"; }

		// インスペクター : 対象ボタンと行き先の設定
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// ボタンへ押下時の処理を差し込む(まだ見つからなければ何もしない)
		void TryBindButtons(Engine::GameObject::ObjectContext& a_context);

		// ポーズを閉じて後ろのゲームへ戻る
		void RequestResume();

		// ポーズを閉じてから、ゲームのシーンを行き先へ差し替える
		void RequestExitScene();

	private:

		//-------------------------------------------------------------------
		// 設定(保存される)
		//-------------------------------------------------------------------
		// 押したらゲームへ戻るボタン(同じシーンに置いた UIButton のGUID)
		Engine::GUID m_resumeButtonGUID = {};

		// 押したらゲームをやめて別のシーンへ移るボタン(任意)
		Engine::GUID m_exitButtonGUID = {};

		// やめたときの行き先(ホームやタイトル)。未設定ならボタンを押しても移らない
		Engine::GUID m_exitSceneGUID = {};

		// 閉じるのにも使う入力アクション名。開くのと同じキーにしておく
		std::string m_pauseActionName = "Pause";

		// ポーズの間はカーソルの中央固定を切るか。
		// 固定したままだとカーソルが毎フレーム中央へ戻され、ボタンを狙えない
		bool m_isReleaseCursorLock = true;

		//-------------------------------------------------------------------
		// 状態(保存しない)
		//-------------------------------------------------------------------
		// 差し込み済みか。ボタンは同じシーンに居るので普通は初回で見つかるが、
		// 読み込み順に依存しないよう、見つかるまで毎フレーム試す
		bool m_isBound = false;

		// 二重に要求を出さないための印
		bool m_isClosing = false;
	};
}
