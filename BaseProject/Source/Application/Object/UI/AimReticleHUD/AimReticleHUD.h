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
	/// 書き込む。基準になるのは UIBase のアンカー(PixelPos / PixelSize)。
	/// 飾りは何枚でも生やせるので、そのどれかではなくアンカーを唯一の基準にしてある
	/// (見た目に合わせたいときは、アンカーの大きさを飾りに合わせること)。
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

		// 判定半径をアンカーの大きさ(PixelSize)から作るか。
		// true  : PixelSize の半分 × radiusScale
		// false : lockRadius をそのまま使う(絵に余白がある時などに手で詰める)
		bool  m_isUseTextureSize = true;

		float m_radiusScale = 1.0f;		// 表示サイズから作るときの倍率
		float m_lockRadius  = 80.0f;	// 手で指定するときの半径(px)
	};
}
