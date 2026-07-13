#pragma once

#include "../Builder/RootSignatureBuilder/RootSignatureBuilder.h"

#include "../Builder/PipelineBuilder/RenderPipelineBuilder/RenderPipelineBuilder.h"

namespace Engine::D3D12
{
	struct RenderPipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;

		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_AS AS;
		CD3DX12_PIPELINE_STATE_STREAM_MS MS;

		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendState;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencilState;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;

		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
	};

	class PipelineStateManager
	{
	public:
		PipelineStateManager() = default;
		~PipelineStateManager() = default;

		// 初期化
		void Init(D3D12::Device* a_pDevice);

		// 解放
		void Release();

		// 初期値からリクエスト
		ID3D12RootSignature* Request(const D3D12::RootSignatureDesc& a_desc);
		ID3D12RootSignature* Request(const std::string& a_shaderPath);
		ID3D12RootSignature* Request(ID3DBlob* a_pShaderBlob);

		ID3D12PipelineState* Request(const D3D12::GraphicsPipelineDesc& a_desc);
		ID3D12PipelineState* Request(const D3D12::ComputePipelineDesc& a_desc);

		ID3D12PipelineState* Request(const D3D12::RenderPipelineBuilder& a_builder);

		// パイプラインステートのハンドル管理
		Handle<ID3D12PipelineState> RequestHandle(const D3D12::GraphicsPipelineDesc& a_desc);
		Handle<ID3D12PipelineState> RequestHandle(const D3D12::ComputePipelineDesc& a_desc);

		Handle<ID3D12PipelineState> RequestHandle(const D3D12::RenderPipelineBuilder& a_builder);


		ID3D12PipelineState* GetPSO(Handle<ID3D12PipelineState> a_handle);
		ID3D12PipelineState* GetPSO(uint8_t a_rawIdx8bit);

	private:
		// 構造体からハッシュ値を求める
		uint64_t CalcHash(const void* a_pData,size_t a_size);
		uint64_t CalcHash(const D3D12::RootSignatureDesc& a_desc);

	private:

		D3D12::Device* m_pDevice = nullptr;

		std::unordered_map<uint64_t, ComPtr<ID3D12RootSignature>> m_rootSigMap;

		std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>> m_psoMap;
		Storage::HandlePool<ID3D12PipelineState> m_psoHandlePool = {};
		std::vector<ID3D12PipelineState*> m_pPsoVec = {};

	};
}