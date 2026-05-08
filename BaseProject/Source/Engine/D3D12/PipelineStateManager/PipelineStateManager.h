#pragma once
namespace Engine::D3D12
{
	class PipelineStateManager
	{
	public:
		PipelineStateManager() = default;
		~PipelineStateManager() = default;

		// 初期化
		void Init(ID3D12Device* a_pDevice);

		// 初期値からリクエスト
		ID3D12RootSignature* Request(const D3D12_ROOT_SIGNATURE_DESC& a_desc);
		ID3D12PipelineState* Request(const D3D12::GraphicsPipelineDesc& a_desc);



	private:
		// 構造体からハッシュ値を求める
		uint64_t CalcHash(const void* a_pData,size_t a_size);

	private:

		ID3D12Device* m_pDevice = nullptr;

		std::unordered_map<uint64_t, ComPtr<ID3D12RootSignature>> m_rootSigMap;
		std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>> m_psoMap;
	};
}