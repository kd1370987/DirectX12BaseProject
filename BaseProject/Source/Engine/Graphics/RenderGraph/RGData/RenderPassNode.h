#pragma once

#include "../../ShadingPipelineBuilder/ShadingPipelineBuilder.h"

namespace Engine::Graphics
{
	// 前方宣言
	class GraphicsEngine;
	class RenderContext;
	class RGPassResources;

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

	// パスが使うパイプライン種別。自動バインドがグラフィックス系とコンピュート系の
	// どちらのルートAPIを叩くかをこれで決める
	enum class ERGPipelineType : uint8_t
	{
		Graphics,
		Compute
	};

	// パス実行前にグラフが張るディスクリプタヒープ
	enum class ERGHeapMode : uint8_t
	{
		None,					// グラフは触らない（executeFunc側で張る）
		Default,				// BindHeap()
		BindlessWithSampler		// BindCopyHeapAndSumplerBindLess()
	};

	// フレームの偶奇でしか動かないパスの指定（ピンポンバッファ用）
	enum class ERGFrameParity : uint8_t
	{
		Always,
		Even,
		Odd
	};

	// =========================================================
	// ビルド時にビルダーが返すトークン
	// ノードの accesses 配列への添字そのもの。実行時はこれで O(1) 参照する
	// =========================================================
	struct RGResourceRef
	{
		static constexpr uint16_t kInvalid = 0xFFFF;

		uint16_t index = kInvalid;

		bool IsValid() const { return index != kInvalid; }
	};

	// =========================================================
	// リソースアクセス宣言 : ビルド時に確定する静的データ
	// =========================================================
	struct RGAccessDecl
	{
		std::string	resName		= "";					// リソース名（文字列を使うのはコンパイル時まで）
		AccessType	type		= AccessType::None;		// アクセスタイプ
		DXGI_FORMAT	format		= DXGI_FORMAT_UNKNOWN;	// フォーマット
		float		texScale	= 1.0f;					// テクスチャスケール

		LoadOp		load		= LoadOp::Clear;		// バッファをクリアするかどうか
		StoreOp		store		= StoreOp::Store;		// これ以降使うかどうか

		bool		isTemporal	= false;				// ヒストリーバッファか
		bool		isWrite		= false;				// Write要求か（Readならfalse）

		// ---- コンパイル後に解決される ----
		RGResourceHandle handle = {};
	};

	// =========================================================
	// バインド宣言 : どのルートパラメータへ何を割り当てるかをビルド時に確定させる
	// =========================================================
	enum class ERGBindType : uint8_t
	{
		SrvTable,		// 連続したSRV群を1つのディスクリプタテーブルとしてバインド
		Srv,			// SRV 1枚を単独のルートパラメータへ
		Uav				// UAV 1枚を単独のルートパラメータへ
	};

	struct RGBindDecl
	{
		ERGBindType	type		= ERGBindType::SrvTable;
		UINT		rootIndex	= 0;
		uint16_t	firstAccess	= 0;	// accesses への開始添字
		uint16_t	count		= 1;	// 連続数（SrvTable以外は必ず1）
	};

	// =========================================================
	// コピー宣言 : src -> dst のコピーはグラフが直接実行する（executeFunc不要）
	// =========================================================
	struct RGCopyDecl
	{
		uint16_t srcAccess = 0;		// accesses への添字
		uint16_t dstAccess = 0;
	};

	// =========================================================
	// 描画用パスノード
	// 「ビルド時に決まるもの」を全てここに静的データとして持たせる
	// =========================================================
	struct RenderPassNode
	{
		static constexpr uint8_t kInvalidPSOIndex = 255;

		// ---- 識別 ----
		std::string	name;
		UINT		nameHash = 0;
		EDrawPhase	phase = EDrawPhase::Setup;

		// ---- 静的宣言（ビルド時に確定） ----
		std::vector<RGAccessDecl>	accesses	= {};	// リソースアクセス（宣言順）
		std::vector<RGBindDecl>		binds		= {};	// ルートパラメータへのバインド
		std::vector<RGCopyDecl>		copies		= {};	// グラフが実行するコピー

		ERGPipelineType	pipelineType	= ERGPipelineType::Graphics;
		ERGHeapMode		heapMode		= ERGHeapMode::None;
		ERGFrameParity	frameParity		= ERGFrameParity::Always;

		// グラフがパス実行前にセットするルートシグネチャ・PSO
		// nullptr / 未設定なら executeFunc 側で自前セットする契約
		ID3D12RootSignature*	pRootSig	= nullptr;
		uint8_t					psoIndex	= kInvalidPSOIndex;

		// シェーディングパイプライン
		ShadingPipelineBuilder pipelineBuilder;

		// ---- コンパイル後データ ----
		uint8_t passIndex = 255;						// ソートキー用インデックス
		std::map<std::string, uint8_t> psoIndexMap;		// PSOのインデックスマップ

		uint8_t GetPSOIndex(const std::string& a_name) const
		{
			auto _it = psoIndexMap.find(a_name);
			if (_it != psoIndexMap.end()) return _it->second;
			return kInvalidPSOIndex;
		};

		// ---- 実行関数 ----
		// 静的に宣言しきれない部分（定数バッファ更新・Dispatch/Draw）だけをここに書く。
		// 宣言だけで完結するパス（コピーパス等）は nullptr で良い
		std::function<void(GraphicsEngine*, RenderContext*, const RGPassResources&)> executeFunc;
	};
}
