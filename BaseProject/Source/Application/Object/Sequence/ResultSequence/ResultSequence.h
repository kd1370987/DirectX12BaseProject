#pragma once

#include "../../../../Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	/// <summary>
	/// リザルト画面の進行役 : シーンに一つ置く
	/// </summary>
	/// <remarks>
	/// タイトルと同じ作りで、持っているのは「どのボタンを押したらどこへ行くか」だけ。
	///
	///   ・シーン上の UIButton を GUID で探し、押されたらタイトルへ戻す処理を差し込む
	///   ・リザルトの間はカーソルの中央固定を切る(固定したままだと狙えない)
	///
	/// 出す数字(スコア・タイム)は GlobalGameContext に入っていて、
	/// 並べるのは ScoreHUD の仕事。ここは触らない。
	/// ゲームシーンから持ってきた記録はリザルトを抜けても消さない
	/// (消すのはゲームシーンの入り口 = SceneSequence)。
	/// </remarks>
	class ResultSequence : public Engine::GameObject::BaseObject
	{
	public:

		// 更新処理 : ボタンへの差し込みと、カーソル固定の解除
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// 解放処理 : カーソル固定を設定値へ戻す
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "ResultSequence"; }

		// インスペクター : 対象ボタンと遷移先の設定
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// ボタンへ押下時の処理を差し込む(まだ見つからなければ何もしない)
		void TryBindButton(Engine::GameObject::ObjectContext& a_context);

		// タイトルへ戻る
		void RequestBackToTitle();

	private:

		//-------------------------------------------------------------------
		// 設定(保存される)
		//-------------------------------------------------------------------
		// 押したらタイトルへ戻るボタン(同じシーンに置いた UIButton のGUID)
		Engine::GUID m_homeButtonGUID = {};

		// 戻り先(タイトル画面)
		Engine::GUID m_titleSceneGUID = {};

		// リザルトの間はカーソルの中央固定を切るか
		bool m_isReleaseCursorLock = true;

		//-------------------------------------------------------------------
		// 状態(保存しない)
		//-------------------------------------------------------------------
		bool m_isBound = false;			// 差し込み済みか
		bool m_isSceneRequested = false;	// 二重に遷移要求を出さないための印
	};
}
