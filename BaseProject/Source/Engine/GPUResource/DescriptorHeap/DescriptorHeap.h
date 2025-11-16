#pragma once

struct DescriptorHandle
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU{};
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU{};
};

class DescriptorHeap
{
public:

	DescriptorHeap() = default;
	~DescriptorHeap() = default;



	bool Create(
		D3D12_DESCRIPTOR_HEAP_TYPE a_type,
		UINT a_numDescriptors = 512,
		D3D12_DESCRIPTOR_HEAP_FLAGS a_flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		UINT a_mask = 0
	);

	ID3D12DescriptorHeap* GetHeap();					// ディスクリプタヒープ本体を取得

	/// <summary>
	/// レンダーターゲット / デプスステンシルビュー用登録処理
	/// </summary>
	/// <returns>登録したハンドルを返す</returns>
	DescriptorHandle RegisterCPUOnly();
	/// <summary>
	/// シェーダーリソースビュー用登録処理
	/// </summary>
	/// <param name="a_resource"></param>
	/// <returns>登録したハンドルを返す</returns>
	DescriptorHandle RegisterSRV(ID3D12Resource* a_resource);

	/// <summary>
	/// アンオーダーアクセスビュー用登録処理
	/// </summary>
	/// <param name="a_resource">登録リソース</param>
	/// <returns>登録したハンドルを返す</returns>
	DescriptorHandle RegisterUAV(ID3D12Resource* a_resource);

	/// <summary>
	/// サンプラー用登録処理
	/// </summary>
	/// <returns>登録したハンドルを返す</returns>
	DescriptorHandle RegisterSampler();						


private:

	UINT m_incrementSize = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE m_type{};
	ComPtr<ID3D12DescriptorHeap> m_pHeap = nullptr;		// ディスクリプタヒープ本体
	
	UINT m_maxSize = 0;
	//std::vector<DescriptorHandle> m_handles;			// 登録されているハンドル

	size_t m_currentIndex = 0;
};