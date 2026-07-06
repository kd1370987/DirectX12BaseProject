#pragma once

#include "RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderPassRegistry;


	// =========================================================
	// リソースバリア（コンパイル時に計算済みのもの）
	// =========================================================
	struct RGBarrier
	{
		D3D12::GPUResource* pResource		= nullptr;
		D3D12_RESOURCE_STATES before		= D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES after			= D3D12_RESOURCE_STATE_COMMON;

		bool isUAVBarrier					= false;
	};
	
	// =========================================================
	// コンパイル済みパス : 実行時に必要なデータを全てキャッシュ
	// =========================================================
	struct CompiledPass
	{
		RenderPassNode* pNode = nullptr;

		// このパスの「実行直前」に張るバリアのリスト
		std::vector<RGBarrier> preBarriers = {};

		// RTV・DSVチェンジ用（D3D12のCPUハンドルを直接持つ）
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles = {};
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = { 0 };

		// RTV・DSVクリア用（クリアカラー等を保持）
		std::vector<size_t> clearRtvIndices = {};
		bool isDepthClear = false;
	};

	class RGResourceManager;

	class RenderGraph
	{
	public:

		RenderGraph();
		~RenderGraph();

		void Init(RenderPassRegistry* a_pRenderPassRegister);
		void Release();

		// パス追加後、依存関係を整理してバリアを計算する
		void Compile();

		// 毎フレーム呼ばれる実行関数
		void Execute(GraphicsEngine* a_pGE, RenderContext* a_pCtx);

		// パスの使用するフォーマットを取得
		std::vector<DXGI_FORMAT> GetPassRTVFormats(uint8_t a_passIndex);
		DXGI_FORMAT GetPassDSVFormat(uint8_t a_passIndex);

		// リソースマネージャーにアクセス
		const RGResourceManager* GetRGResourceManager() const;
		RGResourceManager* RefRGResourceManager();

		// リソースアクセス
		D3D12::GPUResource* GetPassResource(uint8_t a_passIndex, const std::string& a_name) const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetPassSRV(uint8_t a_passIndex, const std::string& a_name) const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetPassUAV(uint8_t a_passIndex, const std::string& a_name) const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetPassDSV(uint8_t a_passIndex, const std::string& a_name) const;

		UINT GetTemporalIndex() const { return m_tempralIndex; } // テンポラルインデックス取得

		RenderPassNode* GetPass(UINT a_passHash);
	private:
		// テンポラルインデックス更新 : フレーム用テンポラル
		void Swap();

		// コンパイル時の内部処理
		void CreateResource();					// パスからリソースを生成
		void TopologicalSort();					// パスの依存関係を解決
		void ResolveResourceHandles();			// 文字列からRGResourceHandleへ変換
		void ComputeBarriersAndVersions();		// バリア計算とバージョンUP

	private:

		UINT m_tempralIndex = 0;

		// パスデータ格納
		std::map<EDrawPhase, std::vector<RenderPassNode*>> m_pPassNodeMap;
		std::vector<RenderPassNode*> m_sortedPasses = {};

		// コンパイル後の実行用データ
		std::vector<CompiledPass> m_compiledPasses = {};

		// リソース管理
		std::unique_ptr<RGResourceManager> m_upRGResourceManager = nullptr;
	};
}