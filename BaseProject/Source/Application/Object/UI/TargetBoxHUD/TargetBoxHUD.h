#pragma once

#include "../UIBase.h"

namespace App::Object
{
	/// <summary>
	/// 敵(EnemyTag)へ重ねて表示するターゲットボックスHUD。
	///
	/// アクティブカメラのエンティティから ProjMatComponent / WorldMatrixComponent を引き、
	/// EnemyTag を持つ全エンティティのワールド座標をスクリーン座標へ射影して、
	/// 1体につき1枚ずつUIを積む。
	///
	/// 位置は毎フレーム敵から作り直すため、UIBase の PixelPos は使わない。
	/// (サイズ・色・回転・ピボットなどの見た目は UIBase 側の値を全ボックス共通で使う)
	/// </summary>
	class TargetBoxHUD : public UIBase
	{
	public:

		// 初期化処理 : ターゲットボックス用テクスチャの読み込み
		void Init(Engine::GameObject::ObjectContext& a_context) override;

		// 更新処理 : 敵のスクリーン座標を集める
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

		// 敵のワールド座標に足すYオフセット。
		// 敵の原点が足元にあるモデルで、胴体あたりへボックスを合わせたいときに使う。
		float m_worldOffsetY = 0.0f;

		// このフレームに描くボックスのスクリーン座標(px, 左上原点)。
		// Update で作って Draw で消費する。
		std::vector<DXSM::Vector2> m_targetScreenPosVec = {};
	};
}
