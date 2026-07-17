#pragma once

#include "../RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderContext;
	class RGPassBuilderBase;

	// =========================================================
	// SRVディスクリプタテーブルの宣言スコープ
	//
	// SrvTable(rootIdx).Add("A").Add("B") のように書くと、
	// 宣言した順にそのままシェーダのレジスタ順（t0, t1, ...）になる。
	// 実行時にはコンパイル済みの連続配列をそのまま渡すだけになる
	// =========================================================
	class RGSrvTableScope
	{
	public:

		RGSrvTableScope(RGPassBuilderBase* a_pBuilder, size_t a_bindIndex)
			: m_pBuilder(a_pBuilder), m_bindIndex(a_bindIndex) {}

		// 通常のSRV
		RGSrvTableScope& Add(const std::string& a_texName);
		// ヒストリーバッファ（テンポラルフラグ付き）
		RGSrvTableScope& AddHistory(const std::string& a_texName);

		// 直前に追加した要素のトークン
		RGResourceRef LastRef() const { return m_lastRef; }

	private:

		RGResourceRef Push(const RGAccessDecl& a_decl);

		RGPassBuilderBase*	m_pBuilder	= nullptr;
		size_t				m_bindIndex	= 0;		// ノードの binds への添字
		RGResourceRef		m_lastRef	= {};
	};

	// =========================================================
	// 全ビルダー共通のリソース宣言部
	// ここで宣言したものは全てビルド時に確定する静的データになる
	// =========================================================
	class RGPassBuilderBase
	{
	public:

		RGPassBuilderBase(RenderPassNode* a_pNode, ERGPipelineType a_pipelineType)
			: m_pNode(a_pNode)
		{
			m_pNode->pipelineType = a_pipelineType;
		}
		~RGPassBuilderBase() = default;

		// =========================================================
		// 読み込み宣言
		// =========================================================
		RGResourceRef ReadSRV(const std::string& a_texName);
		RGResourceRef ReadHistorySRV(const std::string& a_texName);
		RGResourceRef ReadDepth(const std::string& a_texName);
		RGResourceRef CopySrc(const std::string& a_texName);

		// =========================================================
		// 書き込み宣言
		// =========================================================
		RGResourceRef WriteRTV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,		// 基本はクリアして
			StoreOp a_storeOp = StoreOp::Store,		// 基本は保存する
			float a_texScale = 1.0f
		);
		RGResourceRef WriteDepth(
			const std::string& a_texName,
			DXGI_FORMAT a_format = DXGI_FORMAT_D32_FLOAT,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);
		RGResourceRef WriteUAV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);
		RGResourceRef WriteTemporalRTV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);
		RGResourceRef WriteTemporalUAV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);
		RGResourceRef CopyDst(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);

		// =========================================================
		// 宣言的バインド
		// ここで宣言したものは、コンパイル時にCPUディスクリプタまで解決され、
		// 実行時はグラフが executeFunc の前に自動でバインドする。
		// executeFunc 側でリソース名を書き直す必要はもう無い
		// =========================================================

		// 連続SRVを1つのディスクリプタテーブルとしてバインド
		RGSrvTableScope SrvTable(UINT a_rootIndex);

		// SRV 1枚を単独のルートパラメータへ
		RGResourceRef BindSRV(UINT a_rootIndex, const std::string& a_texName);

		// UAV 1枚を単独のルートパラメータへ（宣言と同時に書き込み要求も出す）
		RGResourceRef BindUAV(
			UINT a_rootIndex,
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);

		// =========================================================
		// コピー宣言 : グラフが直接実行するので executeFunc は不要になる
		// =========================================================
		void Copy(
			const std::string& a_srcName,
			const std::string& a_dstName,
			DXGI_FORMAT a_dstFormat,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);

		// =========================================================
		// パス共通の静的設定
		// =========================================================
		// グラフがパス実行前に張るヒープ
		void SetHeapMode(ERGHeapMode a_mode) { m_pNode->heapMode = a_mode; }

		// グラフがパス実行前に自動セットするPSOを指定する
		void SetPassPSO(uint8_t a_psoIndex) { m_pNode->psoIndex = a_psoIndex; }

		// 偶数/奇数フレームのみ実行する（ピンポンバッファ用）
		// ランタイムでの早期リターンをやめ、グラフ側の静的な実行条件にする
		void SetFrameParity(ERGFrameParity a_parity) { m_pNode->frameParity = a_parity; }

		RenderPassNode* RefNode() { return m_pNode; }

	protected:

		friend class RGSrvTableScope;

		// アクセス宣言を積んでトークンを返す
		RGResourceRef PushAccess(const RGAccessDecl& a_decl);

		// ルートシグネチャを記録（グラフが実行前にセットする）
		void StoreRootSignature(ID3D12RootSignature* a_pRootSig) { m_pNode->pRootSig = a_pRootSig; }

		RenderPassNode* m_pNode = nullptr;
	};

	// PSO作成用の中間データ
	struct TempData
	{
		D3D12::GraphicsPipelineDesc desc;
		uint8_t* pOutIndex;
	};

	// =========================================================
	// ラスタライザ用パスビルダー
	// =========================================================
	class RGRasterPassBuilder : public RGPassBuilderBase
	{
	public:

		RGRasterPassBuilder(RenderPassNode* a_pNode)
			: RGPassBuilderBase(a_pNode, ERGPipelineType::Graphics) {}
		~RGRasterPassBuilder() = default;

		// PSOの作成
		D3D12::GraphicsPipelineDesc& CreatePSODesc(const std::string& a_name, uint8_t& a_outIndex);

		// 最後に呼ぶ : 出力フォーマットを解決してPSOを確定させる
		void ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager);

		// ルートシグネチャセット
		ID3D12RootSignature* SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, ID3DBlob* a_pBlob);

		// ---- ヘルパー ----
		ID3DBlob* SetVS(D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_vsPath, const D3D12_INPUT_LAYOUT_DESC& a_desc);
		void SetPS(D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_psPath);

	private:

		// PSOはパス内に複数所持
		std::vector<TempData> m_tempPSODescVec = {};
	};

	// =========================================================
	// メッシュシェーダー用パスビルダー
	// =========================================================
	class RGMeshShaderPassBuilder : public RGPassBuilderBase
	{
	public:

		RGMeshShaderPassBuilder(RenderPassNode* a_pNode)
			: RGPassBuilderBase(a_pNode, ERGPipelineType::Graphics) {}
		~RGMeshShaderPassBuilder() = default;

		ID3D12RootSignature* SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, ID3DBlob* a_pBlob);
		void SetRootSignature(ID3D12RootSignature* a_pRootSig) { StoreRootSignature(a_pRootSig); }
	};

	// =========================================================
	// コンピュート用パスビルダー
	// =========================================================
	class RGComputePassBuilder : public RGPassBuilderBase
	{
	public:

		RGComputePassBuilder(RenderPassNode* a_pNode)
			: RGPassBuilderBase(a_pNode, ERGPipelineType::Compute) {}
		~RGComputePassBuilder() = default;

		void ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager);

		// ルートシグネチャセット
		ID3D12RootSignature* SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, ID3DBlob* a_pBlob);

		ID3DBlob* SetShader(const std::string& a_csPath, const std::string& a_name, uint8_t& a_outIndex);

	private:

		D3D12::ComputePipelineDesc m_desc = {};
		uint8_t* m_pOutIndex = nullptr;
	};

	// =========================================================
	// 汎用パスビルダー（レイトレ・コピーなど、自前でPSOを管理するパス用）
	// =========================================================
	class RGGlobalsPassBuilder : public RGPassBuilderBase
	{
	public:

		RGGlobalsPassBuilder(RenderPassNode* a_pNode)
			: RGPassBuilderBase(a_pNode, ERGPipelineType::Compute) {}
		~RGGlobalsPassBuilder() = default;
	};
}
