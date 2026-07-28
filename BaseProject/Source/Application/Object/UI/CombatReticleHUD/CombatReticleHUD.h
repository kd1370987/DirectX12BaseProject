#pragma once

#include "../UIBase.h"

namespace App::Object
{
	/// <summary>
	/// 戦闘時に画面中央へ表示する照準(レティクル)HUD。
	/// UI描画パイプラインの動作確認用テストオブジェクト。
	/// </summary>
	class CombatReticleHUD : public UIBase
	{
	public:

		// 初期化処理 : レティクルテクスチャの読み込み
		void Init(Engine::GameObject::ObjectContext& a_context) override;

		// 更新処理
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// 描画処理 : UI描画命令の発行
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "CombatReticleHUD"; }

		// インスペクターの編集UI
		void DrawInspector() override;

		// シーンビュー上のスクリーンハンドルで位置を編集する
		bool DrawGizmo(const Engine::GameObject::ObjectGizmoContext& a_ctx) override;

	private:

		// レティクルのテクスチャ参照(ResourceRefで所有し、GCで解放されないようにする)
		Engine::ResourceRef<Engine::Resource::Texture> m_reticleTexRef = {};

		// 描画するスクリーン座標(ピクセル : 画面左上が原点、Xは右・Yは下方向が正。矩形の中心を指す)
		DXSM::Vector2 m_posPixel = { 960.0f, 540.0f };

		// レティクルの表示サイズ(ピクセル : 幅・高さ)
		DXSM::Vector2 m_sizePixel = { 128.0f, 128.0f };
	};
}
