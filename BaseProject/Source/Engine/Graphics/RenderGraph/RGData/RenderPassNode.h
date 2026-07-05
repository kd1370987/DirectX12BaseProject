#pragma once

#include "../../ShadingPipelineBuilder/ShadingPipelineBuilder.h"

namespace Engine::Graphics
{
	// 前方宣言
	class GraphicsEngine;
	class RenderContext;

	enum class RenderQueueType
	{
		Shadow,
		Simple,
		Opaque,
		AnimationOpaque,
		Transparent,
		AnimationTransparent,
		Bloom,
		Lighting,
		Debug
	};

	enum class RenderQueueType2D
	{
		ScreenUI,
		WorldUI
	};

	enum class LoadOp
	{
		Load,			// 前のパスで書かれていた
		Clear,			// 初回使用
		DontCare		// これ以上後ろで使われることがない
	};

	enum class StoreOp
	{
		Store,
		DontCare		// これ以上後ろで使われることがない
	};

	enum class AccessType
	{
		None,
		SRV,
		RTV,
		Depth_Read,
		Depth_Write,
		UAV,
		CopySrc,
		CopyDst
	};

	struct AccessResource
	{
		Engine::Resource::ID id = Engine::Resource::Limits::INVALID_ID;

		AccessType type = AccessType::None;
		LoadOp load = LoadOp::Clear;
		StoreOp store = StoreOp::Store;
	};

	struct PassDesc
	{
		// パスの識別名
		std::string name = "none";

		UINT rootSigID = UINT_MAX;		// パスが使用するルートシグネチャID
		Engine::Handle<Engine::D3D12::PipelineState> psoID;			// パスが使用するパイプラインステートID

		// 依存関係・トポロジカルソート用
		std::vector<Engine::Resource::ID> readResource = {};		// 入力(SRV)
		std::vector<Engine::Resource::ID> writeResource = {};	// 出力(RTV,UAV)

		// レンダーパス開始・終了時のAPI設定
		std::vector<AccessResource> resourceAccessVec = {};
	};

	// 描画フェーズ
	enum class EDrawPhase : UINT
	{
		Setup,				// リソース関連準備
		Shadow,				// 影生成
		Geometry,			// オブジェクト描画
		Raytracing,
		NotSort,			// ピンポンバッファなどの影響でソートできないフェーズを差し込む
		Lighting,			// ライティング
		Particle,			// パーティクル
		PostProcess,		// ポストプロセス
		HistoryUpdate,		// 過去データ更新
		UI,					// UI描画
		Present,			// バックバッファに描画
		Count
	};

	// リソースの要求書
	struct ResourceRequest
	{
		// リソース情報
		std::string		resName		= "";					// リソース名
		AccessType		type		= AccessType::None;		// アクセスタイプ
		DXGI_FORMAT		format		= DXGI_FORMAT_UNKNOWN;	// フォーマット
		float			texScale	= 1.0f;					// テクスチャスケール

		// バリア情報
		LoadOp	load	= LoadOp::Clear;		// バッファのクリアをするかどうか
		StoreOp	store	= StoreOp::Store;		// これ以降使うかどうか

		bool isTemporal = false;
	};


	// 描画用パスノード
	struct RenderPassNode
	{
		// 初期情報
		std::string name;										// パス名
		UINT nameHash;
		EDrawPhase phase;										// パスが所属するフェーズ

		std::vector<Engine::Resource::ID> read = {};			// 入力データ
		std::vector<Engine::Resource::ID> write = {};			// 出力データ
		std::vector<AccessResource> resourceAccessVec = {};		// 開始・終了時のリソース設定

		std::vector<ResourceRequest> readRequests;
		std::vector<ResourceRequest> writeRequests;

		// シェーディングパイプライン
		ShadingPipelineBuilder pipelineBuilder;

		// コンパイル後データ
		uint8_t passIndex = 255;		// ソートキー用インデックス
		std::map<std::string, uint8_t> psoIndexMap; // PSOのインデックスマップ

		uint8_t GetPSOIndex(const std::string& a_name) const
		{
			auto _it = psoIndexMap.find(a_name);
			if (_it != psoIndexMap.end()) return _it->second;
			return 255;
		};

		// 実行関数
		std::function<void(GraphicsEngine*, RenderContext*, uint8_t)> executeFunc;
	};
}