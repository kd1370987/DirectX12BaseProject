#pragma once
namespace Engine::Graphics
{
	// 前方宣言
	class GraphicsEngine;
	class RenderContext;

	// 描画フェーズ
	enum class EDrawPhase : UINT
	{
		Setup,				// リソース関連準備
		Shadow,				// 影生成
		Geometry,			// オブジェクト描画
		Lighting,			// ライティング
		PostProcess,		// ポストプロセス
		HistoryUpdate,		// 過去データ更新
		UI,					// UI描画
		Present,			// バックバッファに描画
		Count
	};

	// 描画用パスノード
	struct RenderPassNode
	{
		// 初期情報
		std::string name;										// パス名
		std::vector<Engine::Resource::ID> read = {};			// 入力データ
		std::vector<Engine::Resource::ID> write = {};			// 出力データ
		std::vector<AccessResource> resourceAccessVec = {};	// 開始・終了時のリソース設定

		// コンパイル後データ
		uint8_t passIndex = 255;		// ソートキー用インデックス

		// 実行関数
		std::function<void(GraphicsEngine*, RenderContext*)> executeFunc;
	};
}