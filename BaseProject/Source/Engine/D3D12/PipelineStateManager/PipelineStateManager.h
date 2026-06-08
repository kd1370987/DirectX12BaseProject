#pragma once

#include "../Builder/RootSignatureBuilder/RootSignatureBuilder.h"

namespace Engine::D3D12
{
	class PipelineStateManager
	{
	public:
		PipelineStateManager() = default;
		~PipelineStateManager() = default;

		// 初期化
		void Init(ID3D12Device* a_pDevice);

		// 解放
		void Release();

		// 初期値からリクエスト
		ID3D12RootSignature* Request(const D3D12::RootSignatureDesc& a_desc);
		ID3D12RootSignature* Request(const std::string& a_shaderPath);
		ID3D12PipelineState* Request(const D3D12::GraphicsPipelineDesc& a_desc);
		ID3D12PipelineState* Request(const D3D12::ComputePipelineDesc& a_desc);

		// パイプラインステートのハンドル管理
		Resource::Handle<ID3D12PipelineState> RequestHandle(const D3D12::GraphicsPipelineDesc& a_desc);
		Resource::Handle<ID3D12PipelineState> RequestHandle(const D3D12::ComputePipelineDesc& a_desc);
		ID3D12PipelineState* GetPSO(Resource::Handle<ID3D12PipelineState> a_handle);
		ID3D12PipelineState* GetPSO(uint8_t a_rawIdx8bit);

	private:
		// 構造体からハッシュ値を求める
		uint64_t CalcHash(const void* a_pData,size_t a_size);
		uint64_t CalcHash(const D3D12::RootSignatureDesc& a_desc);

	private:

		ID3D12Device* m_pDevice = nullptr;

		std::unordered_map<uint64_t, ComPtr<ID3D12RootSignature>> m_rootSigMap;

		std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>> m_psoMap;
		Storage::HandleStorage<ID3D12PipelineState> m_psoHandleStorage = {};
		std::vector<ID3D12PipelineState*> m_pPsoVec = {};

	};
}