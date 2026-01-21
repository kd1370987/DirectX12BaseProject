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

	DirectX::XMFLOAT3 GetMaxCounts() const { return m_maxCounts; }
	DirectX::XMFLOAT3 GetCurrentCounts() const { return m_currentCounts; }

	/// <summary>
	/// CPU ハンドル取得
	/// </summary>
	/// <param name="a_number">生成時のインデックス</param>
	const D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT a_number) const;

	/// <summary>
	/// GPU ハンドル取得
	/// </summary>
	/// <param name="a_number">生成時のインデックス</param>
	const D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT a_number) const;

	UINT RegisterCBV(
		ID3D12Resource* a_resource,
		size_t a_size,
		D3D12_CONSTANT_BUFFER_VIEW_DESC& a_cbvDesc
	);
	/*DescriptorHandle RegisterCBV(
		ID3D12Resource* a_resource,
		size_t a_size,
		D3D12_CONSTANT_BUFFER_VIEW_DESC& a_cbvDesc
	);*/
	DescriptorHandle RegisterSRV(ID3D12Resource* a_resource);

	DescriptorHandle AllocateSRVRange(UINT a_count);

	DescriptorHandle AllocateSRVRange(std::vector<ID3D12Resource*> a_resource);

	DescriptorHandle RegisterUAV(ID3D12Resource* a_resource);

	D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiCPUHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiGPUHandle();
	

private:

	UINT m_incrementSize = 0;							// 移動距離
	D3D12_DESCRIPTOR_HEAP_TYPE m_type{};				// ディスクリプタヒープのタイプ
	ComPtr<ID3D12DescriptorHeap> m_cpHeap = nullptr;	// ディスクリプタヒープ本体

	ID3D12Device* m_pDevice = nullptr;					// デバイスのポインタ

	// CBV・SRV・UAVのカウント
	DirectX::XMFLOAT3 m_maxCounts = {};					// ディスクリプタヒープに乗せれる上限
	DirectX::XMFLOAT3 m_currentCounts = {};				// 今何番目か

	CBV_SRV_UAVInitInfo m_initInfo = {};		// 初期設定
};