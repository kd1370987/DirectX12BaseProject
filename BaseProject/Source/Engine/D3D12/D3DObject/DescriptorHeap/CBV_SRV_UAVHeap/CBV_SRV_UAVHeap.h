#pragma once

struct CBV_SRV_UAVInitInfo
{
	ID3D12Device* pDevice = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE type;
	D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	UINT maxCBVCount = 100;
	UINT maxSRVCount = 100;
	UINT maxUAVCount = 100;
	UINT mask = 0;
	UINT useImGuiSRVCount = 128;
};

class CBV_SRV_UAVHeap
{
public:
	//==========================================================================================
	// 
	// CBV_SRV_UAV
	// 
	//==========================================================================================
	/// <summary>
	/// ディスクリプタヒープ作成
	/// </summary>
	/// <returns>成功 = true</returns>
	bool Create(const CBV_SRV_UAVInitInfo& a_info);

	/// <summary>
	/// ディスクリプタヒープ取得
	/// </summary>
	/// <returns>ディスクリプタヒープポインタ</returns>
	ID3D12DescriptorHeap* GetHeap();

	//==========================================================================================
	// 
	// CBV
	// 
	//==========================================================================================
	/*UINT RegisterCBV(
		ID3D12Resource* a_resource,
		size_t a_size,
		D3D12_CONSTANT_BUFFER_VIEW_DESC& a_cbvDesc
	);*/
	
	//==========================================================================================
	// 
	// SRV
	// 
	//==========================================================================================
	
	/// <summary>
	/// SRVを一括で確保する
	/// </summary>
	/// <param name="a_intVec">リソースとビューの設定の配列</param>
	/// <returns>保存した情報</returns>
	Storage::Range AllocateSRVRange(std::vector<SRVViewInit> a_intVec);

	/// <summary>
	/// SRVが使える領域のハンドルを取得
	/// </summary>
	/// <param name="a_handle">指定インデックス</param>
	/// <returns>CPUハンドル</returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(Storage::Range a_handle);

	/// <summary>
	/// SRVが使える領域のハンドルを取得
	/// </summary>
	/// <param name="a_handle">指定インデックス</param>
	/// <returns>GPUハンドル</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(Storage::Range a_handle);

	/// <summary>
	/// ImGuiが使えるSRV領域のハンドルを取得
	/// </summary>
	/// <returns>CPUハンドル</returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiCPUHandle();

	/// <summary>
	/// ImGuiが使えるSRV領域のハンドルを取得
	/// </summary>
	/// <returns>GPUハンドル</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiGPUHandle();

	//==========================================================================================
	// 
	// UAV
	// 
	//==========================================================================================
	
	UAVHandle AllocateUAV(const std::vector<UAVViewInit>& a_initVec);

	D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCPUHandle(const UAVHandle& a_handel);
	D3D12_GPU_DESCRIPTOR_HANDLE GetUAVGPUHandle(const UAVHandle& a_handel);

private:

	UINT m_incrementSize = 0;							// 移動距離
	D3D12_DESCRIPTOR_HEAP_TYPE m_type{};				// ディスクリプタヒープのタイプ
	ComPtr<ID3D12DescriptorHeap> m_cpHeap = nullptr;	// ディスクリプタヒープ本体

	ID3D12Device* m_pDevice = nullptr;					// デバイスのポインタ

	CBV_SRV_UAVInitInfo m_initInfo = {};		// 初期設定

	FreeRange m_srvRange;
	FreeRange m_uavRange;
};