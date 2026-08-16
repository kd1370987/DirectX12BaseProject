#pragma once

#include "../UIBase.h"

namespace App::Object
{
	/// <summary>
	/// 敵へ重ねて表示するターゲットボックスHUD。
	///
	/// 出す相手は LockOnTargetSystem がプレイヤーの LockOnTargetComponent へ書いた結果。
	///   レティクル内の敵           … 通常テクスチャ(黄色の枠)
	///   そのうち画面中央に最も近い … ロックテクスチャ(赤い枠)
	///
	/// スクリーン座標はロック判定側が計算済みのものをそのまま使う。ここで射影をやり直すと、
	/// 条件のわずかな差で「枠は出ているのにロックされない」といったズレが起きるため。
	///
	/// 位置は毎フレーム作り直すので、UIBase の PixelPos は使わない。
	/// (サイズ・色・回転・ピボットなどの見た目は UIBase 側の値を全ボックス共通で使う)
	/// </summary>
	class TargetBoxHUD : public UIBase
	{
	public:

		// 初期化処理 : ターゲットボックス用テクスチャの読み込み
		void Init(Engine::GameObject::ObjectContext& a_context) override;

		// 更新処理 : プレイヤーのロック結果からスクリーン座標を集める
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// 描画処理 : 集めた座標ぶんだけUIを積む
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "TargetBoxHUD"; }

		// インスペクター
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

		// 位置は敵から毎フレーム作るので、座標編集用のギズモは出さない
		bool DrawGizmo(
			const Engine::GameObject::ObjectGizmoContext& a_ctx,
			Engine::GameObject::ObjectContext& a_context) override {
			return false;
		}

	private:

		// ロック中の相手に使うテクスチャ(赤い枠)。
		// 通常の枠(黄色)は UIBase の m_texRef / m_texGUID を使う。
		Engine::ResourceRef<Engine::Resource::Texture> m_lockTexRef = {};
		Engine::GUID m_lockTexGUID = {};

		// ロック枠の拡大率(通常枠のピクセルサイズに掛ける)
		float m_lockSizeScale = 1.0f;

		// このフレームに描くボックスのスクリーン座標(px, 左上原点)。
		// Update で作って Draw で消費する。
		std::vector<DXSM::Vector2> m_targetScreenPosVec = {};

		// ロック中の相手のスクリーン座標(px)。isLocked が false のフレームは描かない
		DXSM::Vector2 m_lockedScreenPos = {};
		bool m_isLocked = false;
	};
}
