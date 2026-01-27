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
		return m_offScreenRT.Ref();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle();


	//ComPtr<ID3D12DescriptorHeap> m_postProcessSRVHeap = nullptr;

	ComPtr<ID3D12Resource> m_screenVB;
	D3D12_VERTEX_BUFFER_VIEW m_screenVBView;

	ComPtr<ID3D12RootSignature> m_screenRootSignature;
	ComPtr<ID3D12PipelineState> m_screenPipelineDefault;

	// レンダーターゲット
	RenderTarget m_offScreenRT;
	RTVHandle m_rtvHandle;			// RTV
	Storage::Range m_srvRange;			// SRV
};