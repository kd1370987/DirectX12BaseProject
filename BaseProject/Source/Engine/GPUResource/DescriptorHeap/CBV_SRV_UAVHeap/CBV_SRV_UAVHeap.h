#pragma once

class CBV_SRV_UAVHeap
{
public:

	/// <summary>
	/// ディスクリプタヒープ作成
	/// </summary>
	/// <param name="a_type">作成する種類</param>
	/// <param name="a_numDescriptors">ディスクリプタに乗せれる上限</param>
	/// <param name="a_flags">シェーダから見えるかどうか</param>
	/// <param name="a_mask">アダプタ数によって変化</param>
	/// <returns>成功 = true</returns>
	bool Create(
		ID3D12Device* a_pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE a_type,
		DirectX::XMFLOAT3 a_maxCounts = { 100, 100, 100 },
		D3D12_DESCRIPTOR_HEAP_FLAGS a_flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		UINT a_mask = 0
	);

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


	

private:

	UINT m_incrementSize = 0;							// 移動距離
	D3D12_DESCRIPTOR_HEAP_TYPE m_type{};				// ディスクリプタヒープのタイプ
	ComPtr<ID3D12DescriptorHeap> m_cpHeap = nullptr;	// ディスクリプタヒープ本体

	ID3D12Device* m_pDevice = nullptr;					// デバイスのポインタ

	// CBV・SRV・UAVのカウント
	DirectX::XMFLOAT3 m_maxCounts = {};					// ディスクリプタヒープに乗せれる上限
	DirectX::XMFLOAT3 m_currentCounts = {};				// 今何番目か
};