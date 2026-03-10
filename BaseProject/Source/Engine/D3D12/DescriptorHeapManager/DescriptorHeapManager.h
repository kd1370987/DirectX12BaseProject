#pragma once

namespace Engine::D3D12
{
	class RTVAllocator;
	class DSVAllocator;
	class SRVAllocator;
}

class DescriptorHeapManager
{
public:
	
	// 初期化と解放
	bool Init();
	void Release();

	//==========================================================================================
	// 
	// CBV_SRV_UAV
	// 
	//==========================================================================================
	// ヒープの生ポインタ取得
	ID3D12DescriptorHeap* GetCBV_SRV_UAVHeap() const;

	//------------------------------------------------------------------------------------------
	// CBV
	//------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------
	// SRV
	//------------------------------------------------------------------------------------------
	// 一括でSRVを確保
	std::vector<Engine::Resource::Handle<SRV>> AllocateSRVRange(std::vector<SRVViewInit> a_viewInitVec);

	// SRVのCPUハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(Engine::Resource::Handle<SRV> a_range);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(Engine::Resource::Handle<SRV> a_range);

	//------------------------------------------------------------------------------------------
	// UAV
	//------------------------------------------------------------------------------------------



	//==========================================================================================
	// 
	// ImGui
	// 
	//==========================================================================================

	// ImGui初期設定用
	ID3D12DescriptorHeap* GetImGuiHeap() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiCPUHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiGPUHandle();

	/// 一括でSRVを確保
	std::vector<Engine::Resource::Handle<SRV>> AllocateImGuiSRVRange(std::vector<SRVViewInit> a_viewInitVec);

	// ImGuiのSRVハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiSRVCPUHandle(Engine::Resource::Handle<SRV> a_range);
	D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiSRVGPUHandle(Engine::Resource::Handle<SRV> a_range);

	//==========================================================================================
	// 
	// DSV
	// 
	//==========================================================================================
	// 深度ステンシルビューを作成しハンドルを取得
	Engine::Resource::Handle<DSV> AllocateDSV(
		ID3D12Resource* a_resource,
		D3D12_DEPTH_STENCIL_VIEW_DESC* a_pDSVDesc
	);

	// 深度ステンシルビュー破棄
	void RemoveDSV(Engine::Resource::Handle<DSV> a_handle);

	// CPUハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle(Engine::Resource::Handle<DSV> a_handle);

	//==========================================================================================
	// 
	// RTV
	// 
	//==========================================================================================
	// レンダーターゲット生成しハンドル取得
	Engine::Resource::Handle<RTV> AllocateRTV(
		ID3D12Resource* a_resource,
		D3D12_RENDER_TARGET_VIEW_DESC* a_pRtvDesc
	);

	// レンダーターゲット破棄
	void RemoveRTV(Engine::Resource::Handle<RTV> a_handle);

	// CPUハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(Engine::Resource::Handle<RTV> a_handle);

private:

	// ヒープ本体
	Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_cbv_srv_uavHeap;
	Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>			m_dsvHeap;
	Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>			m_rtvHeap;
	Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_imguiHeap;

	// ヒープアロケーター
	std::unique_ptr<Engine::D3D12::RTVAllocator> m_upRTVAllocator = nullptr;
	std::unique_ptr<Engine::D3D12::DSVAllocator> m_upDSVAllocator = nullptr;
	std::unique_ptr<Engine::D3D12::SRVAllocator> m_upSRVAllocator = nullptr;
	std::unique_ptr<Engine::D3D12::SRVAllocator> m_upImGuiSRVAllocator = nullptr;

// シングルトン
private:
	DescriptorHeapManager();
	~DescriptorHeapManager();

	// コピー禁止
	DescriptorHeapManager(const DescriptorHeapManager&) = delete;
	void operator=(const DescriptorHeapManager&) = delete;

public:

	static DescriptorHeapManager& Instance()
	{
		static DescriptorHeapManager instance;
		return instance;
	}
};