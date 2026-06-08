#pragma once

#include "../RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderGraph;
	class RenderContext;

	// 中間データ
	struct TempData
	{
		D3D12::GraphicsPipelineDesc desc;
		uint8_t* pOutIndex;
	};

	// ラスタライザ用パスビルダー
	class RGRasterPassBuilder
	{
	public:

		RGRasterPassBuilder(RenderPassNode* a_pNode, RenderGraph* a_pRG) : m_pNode(a_pNode),m_pRG(a_pRG) {}
		~RGRasterPassBuilder() = default;

		// PSOの作成
		D3D12::GraphicsPipelineDesc& CreatePSODesc(const std::string& a_name,uint8_t& a_outIndex);

		// 最後に呼ぶ
		void ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager);

		// ---- パスを通しての共通設定 ----
		// ルートシグネチャセット
		bool SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, const std::string& a_shaderPath);

		// 依存関係構築
		void Read(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);
		void Write(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);

		// ---- ヘルパー ----
		void SetVS(D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_vsPath, const D3D12_INPUT_LAYOUT_DESC& a_desc);
		void SetPS(D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_psPath);

	private:

		RenderGraph* m_pRG = nullptr;
		RenderPassNode* m_pNode = nullptr;

		// ルートシグネチャはパスで一つ
		ID3D12RootSignature* m_pRootSig = nullptr;

		// PSOはパス内に複数所持
		std::vector<TempData> m_tempPSODescVec = {};

		// パスで共通の出力
		std::vector<DXGI_FORMAT> m_rtvFormatVec;
		
	};

	// コンピュート用パスビルダー
	class RGComputePassBuilder
	{
	public:

		RGComputePassBuilder(RenderPassNode* a_pNode, RenderGraph* a_pRG) : m_pNode(a_pNode),m_pRG(a_pRG) {}
		~RGComputePassBuilder() = default;

		void ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager);

		// ルートシグネチャセット
		ID3D12RootSignature* SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, const std::string& a_shaderPath);

		// 依存関係構築
		void Read(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);
		void Write(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);

		void SetShader(const std::string& a_csPath, const std::string& a_name, uint8_t& a_outIndex);

	private:

		RenderGraph* m_pRG = nullptr;
		RenderPassNode* m_pNode = nullptr;

		ID3D12RootSignature* m_pRootSig = nullptr;
		D3D12::ComputePipelineDesc m_desc = {};
		uint8_t* m_pOutIndex = nullptr;
	};

	class RGGlobalsPassBuilder
	{
	public:
		RGGlobalsPassBuilder(RenderPassNode* a_pNode, RenderGraph* a_pRG) : m_pNode(a_pNode), m_pRG(a_pRG) {}
		~RGGlobalsPassBuilder() = default;

		// 依存関係構築
		void Read(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);
		void Write(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);

	private:
		RenderGraph* m_pRG = nullptr;
		RenderPassNode* m_pNode = nullptr;
	};
}
