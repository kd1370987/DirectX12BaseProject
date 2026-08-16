#pragma once

#include "../UIBase.h"

namespace App::Object
{
	/// <summary>
	/// 画面中央の内側レティクル(オートエイム用)。
	///
	/// CombatReticleHUD が「戦闘中の外枠」なのに対して、こちらは
	/// 「この円の内側に入った敵だけがロック対象になる」判定そのものを表す UI。
	///
	/// 判定の中心と半径は、この UI が毎フレーム プレイヤーの LockOnTargetComponent へ
	/// 書き込む。判定を別に持つと「枠の内側なのにロックされない」ズレが起きるので、
	/// 実際に描いている見た目を唯一の基準にする。
	/// </summary>
	class AimReticleHUD : public UIBase
	{
	public:

		// 初期化処理 : レティクルテクスチャの読み込み
		void Init(Engine::GameObject::ObjectContext& a_context) override;

		// 更新処理 : 判定の中心と半径をプレイヤーへ渡す
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "AimReticleHUD"; }

		// インスペクター
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// 判定半径(px)を求める
		float CalcLockRadius() const;

	private:

		// 判定半径を表示サイズから作るか。
		// true  : 描いている画像の半分 × radiusScale(見た目と判定が必ず一致する)
		// false : lockRadius をそのまま使う(画像に余白がある時などに手で詰める)
		bool  m_isUseTextureSize = true;

		float m_radiusScale = 1.0f;		// 表示サイズから作るときの倍率
		float m_lockRadius  = 80.0f;	// 手で指定するときの半径(px)
	};
}
