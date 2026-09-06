#pragma once
//==========================================================================================
//
// RenderingPipelineEditor (Engine::Editor::Inspector)
//
// レンダリングパイプラインアセットの編集UI。
//
// 以前は RenderingPipelineAsset がこの中身をまるごと持っていたが、
// ランタイム側のアセットが ImNodes とエディターの選択状態を抱える形になっていた。
// ここへ移して、アセット側は「グラフを1つ持つ」だけに戻してある。
//
// ImNodes の作法は NodeGraphEditor が持つので、ここは何を描くかだけを書く。
//
// アセットは GUID で持って毎フレーム引き直す。
// 実体ポインタを抱えると、アンロードや作り直しで切れる
//
//==========================================================================================
#include "../../../../../ImGui/ImNode/Core/NodeGraphEditor.h"

namespace Engine::Graphics::Pipeline
{
	class Pass;
	class RenderingPipelineAsset;
}

namespace Engine::Editor::Inspector
{
	class RenderingPipelineEditor : public NodeGraphEditor
	{
	public:

		explicit RenderingPipelineEditor(const Engine::GUID& a_assetGUID)
			: m_assetGUID(a_assetGUID)
		{}

		~RenderingPipelineEditor() override = default;

		// このエディターが開いているアセット。
		// 選択が変わったかどうかの判定に使う
		const Engine::GUID& GetAssetGUID() const { return m_assetGUID; }

	protected:

		bool OnBeginDraw(EditorContext& a_editContext) override;
		void OnDrawHeader(EditorContext& a_editContext) override;
		void OnDrawNodes(EditorContext& a_editContext) override;
		void OnPostDraw(EditorContext& a_editContext) override;

		void OnApplyNodePositions() override;
		void OnSyncNodePositions() override;

	private:

		//----------------------------------------------------------------------------------
		// ヘッダー(グラフの外)
		//----------------------------------------------------------------------------------
		void DrawToolbar(EditorContext& a_editContext);	// 保存・追加・コンパイル・既定構成
		void DrawAddPass();								// パス追加ボタン + ポップアップ
		void DrawValidation();							// 検証結果の一覧
		void DrawSelectedPassDetail();					// 選択中パスの詳細(EditUpdate)

		//----------------------------------------------------------------------------------
		// グラフの中
		//----------------------------------------------------------------------------------
		void DrawNode(Graphics::Pipeline::Pass& a_pass);	// ノード1つ分の枠とピン

		//----------------------------------------------------------------------------------
		// 操作
		//----------------------------------------------------------------------------------
		void HandleCreateLink();						// 線が引かれたときの処理
		void HandleDeleteSelection();					// Delete キーでの削除
		void HandlePendingDeletePass();					// ノード内ボタンで予約された削除

		// パスを追加して、決まったノード座標を ImNodes 側へ反映する
		void AddPassFromEditor(ID<Graphics::Pipeline::Pass> a_typeID);

		// 編集対象。OnBeginDraw で引き直したものをそのフレームの間だけ持つ
		Graphics::Pipeline::RenderingPipelineAsset* m_pAsset = nullptr;

		// 開いているアセット : 同一性はこちら
		Engine::GUID m_assetGUID = {};

		// このフレーム内で削除予約されたパス。
		// パス配列を回している最中に消すとイテレータが壊れるので、後でまとめて消す
		Engine::GUID m_pendingDeletePass = {};
	};
}
