#pragma once

#include "../UIBase.h"

namespace App::Object
{
	/// <summary>
	/// 戦闘時に画面中央へ表示する照準(レティクル)HUD。外枠のほう。
	///
	/// 見た目だけでなく、ミサイルのターゲット収集範囲も兼ねる。
	/// 描いている画像に内接する円を判定円として、毎フレーム プレイヤーの
	/// MissileLockComponent へ書き込む。判定を別に持つと
	/// 「枠の内側なのに溜まらない」ズレが起きるので、描いている見た目を基準にする。
	///
	/// 内側の AimReticleHUD は銃のロックオン(LockOnTargetComponent)用で別枠。
	/// </summary>
	class CombatReticleHUD : public UIBase
	{
	public:

		// 初期化処理 : レティクルテクスチャの読み込み
		void Init(Engine::GameObject::ObjectContext& a_context) override;

		// 更新処理 : ミサイルの収集円をプレイヤーへ渡す
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "CombatReticleHUD"; }

		// インスペクター
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// 収集円の半径(px)を求める。
		// 表示サイズに内接する円なので、画像を大きくすれば収集範囲も広がる。
		// 倍率で詰めたい場合はプレイヤーの MissileLockComponent::reticleScale を使う
		// (この HUD 側に設定を足すと、既存シーンのバイナリ配置が崩れるため)
		float CalcCollectRadius() const;
	};
}
