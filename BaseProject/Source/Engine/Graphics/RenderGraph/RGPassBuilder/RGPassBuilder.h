#pragma once

#include "../RGData/RenderPassNode.h"
#include "../../../D3D12/Builder/PipelineBuilder/MeshPipelineBuilder/MeshPipelineBuilder.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderContext;

	// 中間データ
	struct TempData
	{
		D3D12::GraphicsPipelineDesc desc;
		uint8_t* pOutIndex;
	};

	struct TempMSData
	{
		D3D12::MeshPipelineBuilder desc;
		uint8_t* pOutIndex;
	};

	// ラスタライザ用パスビルダー
	class RGRasterPassBuilder
	{
	public:

		RGRasterPassBuilder(RenderPassNode* a_pNode) : m_pNode(a_pNode) {}
		~RGRasterPassBuilder() = default;

		// =========================================================
		// 読み込み系
		// =========================================================
		void ReadSRV(const std::string& a_texName);
		void ReadDepth(const std::string& a_texName);
		void ReadHistorySRV(const std::string& a_texName);
		// =========================================================
		// 書き込み系
		// =========================================================
		// 基本はクリア
		void WriteRTV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear, // 基本はクリアして
			StoreOp a_storeOp = StoreOp::Store, // 基本は保存する
			float a_texScale = 1.0f
		);
		void WriteDepth(
			const std::string& a_texName,
			DXGI_FORMAT a_format = DXGI_FORMAT_D32_FLOAT,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);
		void WriteTemporalRTV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);

		// PSOの作成
		D3D12::GraphicsPipelineDesc& CreatePSODesc(const std::string& a_name,uint8_t& a_outIndex);

		// 最後に呼ぶ
		void ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager);

		// ---- パスを通しての共通設定 ----
		// ルートシグネチャセット
		//bool SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, const std::string& a_shaderPath);
		ID3D12RootSignature* SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, ID3DBlob* a_pBlob);

		// ---- ヘルパー ----
		ID3DBlob* SetVS(D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_vsPath, const D3D12_INPUT_LAYOUT_DESC& a_desc);
		void SetPS(D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_psPath);

	private:

		RenderPassNode* m_pNode = nullptr;

		// ルートシグネチャはパスで一つ
		ID3D12RootSignature* m_pRootSig = nullptr;

		// PSOはパス内に複数所持
		std::vector<TempData> m_tempPSODescVec = {};

		// パスで共通の出力
		std::vector<DXGI_FORMAT> m_rtvFormatVec;
		
	};

	// メッシュシェーダー用パスビルダー
	class RGMeshShaderPassBuilder
	{
	public:

		RGMeshShaderPassBuilder(RenderPassNode* a_pNode) : m_pNode(a_pNode) {}
		~RGMeshShaderPassBuilder() = default;

		// =========================================================
		// 読み込み系
		// =========================================================
		void ReadSRV(const std::string& a_texName);
		void ReadDepth(const std::string& a_texName);
		void ReadHistorySRV(const std::string& a_texName);
		// =========================================================
		// 書き込み系
		// =========================================================
		// 基本はクリア
		void WriteRTV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear, // 基本はクリアして
			StoreOp a_storeOp = StoreOp::Store, // 基本は保存する
			float a_texScale = 1.0f
		);
		void WriteDepth(
			const std::string& a_texName,
			DXGI_FORMAT a_format = DXGI_FORMAT_D32_FLOAT,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);
		void WriteTemporalRTV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);

		// PSOの作成用構造体
		D3D12::MeshPipelineBuilder& CreatePSODesc(const std::string& a_name, uint8_t& a_outIndex);

		// ---- パスを通しての共通設定 ----
		ID3D12RootSignature* SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, const std::string& a_shaderPath);
		void SetRootSignature(ID3D12RootSignature* a_pRootSig);

		// シェーダーセット
		void SetMS(D3D12::MeshPipelineBuilder& a_pso, const std::string& a_msPath);
		void SetPS(D3D12::MeshPipelineBuilder& a_pso, const std::string& a_psPath);

		// PSOの作成
		void ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager);

	private:

		RenderPassNode* m_pNode = nullptr;

		// ルートシグネチャはパスで一つ
		ID3D12RootSignature* m_pRootSig = nullptr;

		// PSOはパス内に複数所持
		std::vector<TempMSData> m_tempMSPSODescVec = {};

		// パスで共通の出力
		std::vector<DXGI_FORMAT> m_rtvFormatVec;

	};

	// コンピュート用パスビルダー
	class RGComputePassBuilder
	{
	public:

		RGComputePassBuilder(RenderPassNode* a_pNode) : m_pNode(a_pNode){}
		~RGComputePassBuilder() = default;

		void ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager);

		// ルートシグネチャセット
		ID3D12RootSignature* SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, ID3DBlob* a_pBlob);

		// 依存関係構築
		// =========================================================
		// 読み込み系
		// =========================================================
		void ReadSRV(const std::string& a_texName);
		void ReadHistorySRV(const std::string& a_texName);

		// =========================================================
		// 書き込み系
		// =========================================================
		void WriteUAV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,	// 基本はクリアして
			StoreOp a_storeOp = StoreOp::Store, // 基本は保存する
			float a_texScale = 1.0f
		);
		void WriteTemporalUAV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,
			StoreOp a_storeOp = StoreOp::Store,
			float a_texScale = 1.0f
		);

		ID3DBlob* SetShader(const std::string& a_csPath, const std::string& a_name, uint8_t& a_outIndex);

	private:

		RenderPassNode* m_pNode = nullptr;

		ID3D12RootSignature* m_pRootSig = nullptr;
		D3D12::ComputePipelineDesc m_desc = {};
		uint8_t* m_pOutIndex = nullptr;
	};

	class RGGlobalsPassBuilder
	{
	public:
		RGGlobalsPassBuilder(RenderPassNode* a_pNode) : m_pNode(a_pNode){}
		~RGGlobalsPassBuilder() = default;

		// 依存関係構築
		// =========================================================
		// 読み込み系
		// =========================================================
		void CopySrc(const std::string& a_texName);

		// =========================================================
		// 書き込み系
		// =========================================================
		void CopyDst(const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,	// 基本はクリアして
			StoreOp a_storeOp = StoreOp::Store, // 基本は保存する
			float a_texScale = 1.0f);
		void ReadSRV(const std::string& a_texName);
		void WriteUAV(
			const std::string& a_texName,
			DXGI_FORMAT a_format,
			LoadOp a_loadOp = LoadOp::Clear,	// 基本はクリアして
			StoreOp a_storeOp = StoreOp::Store, // 基本は保存する
			float a_texScale = 1.0f
		);

	private:
		RenderPassNode* m_pNode = nullptr;
	};
}
