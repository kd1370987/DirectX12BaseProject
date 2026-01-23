#pragma once

class OffScreen
{
public:

	bool CreatePostProcessResource(ID3D12Resource& a_backBuffer);

	bool CreateScreenVertex();

	bool CreateScreenPipeline();

	void SetRenderTarget(
		ID3D12GraphicsCommandList* a_pCmdList,
		D3D12_CPU_DESCRIPTOR_HANDLE& a_depthHandle
	);

	ID3D12Resource* Get()
	{
		return m_offScreenResource.Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle()
	{
		return m_postProcessRTVHeap->GetCPUDescriptorHandleForHeapStart();
	}



	ComPtr<ID3D12Resource> m_offScreenResource;

	ComPtr<ID3D12DescriptorHeap> m_postProcessRTVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_postProcessSRVHeap = nullptr;

	ComPtr<ID3D12Resource> m_screenVB;
	D3D12_VERTEX_BUFFER_VIEW m_screenVBView;

	ComPtr<ID3D12RootSignature> m_screenRootSignature;
	ComPtr<ID3D12PipelineState> m_screenPipelineDefault;
};