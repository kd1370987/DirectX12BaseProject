#pragma once

class RTVHeap
{
public:

	/// <summary>
	/// レンダーターゲットビューの作成・登録
	/// </summary>
	/// <param name="a_resource">登録リソース</param>
	/// <param name="a_pRtvDesc">ビュー設定</param>
	/// <returns>登録したインデックス</returns>
	RTVHandle RegisterRTV(ID3D12Resource* a_resource,D3D12_RENDER_TARGET_VIEW_DESC* a_pRtvDesc);

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
		UINT a_numDescriptors = 100,
		D3D12_DESCRIPTOR_HEAP_FLAGS a_flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		UINT a_mask = 0
	);

	/// <summary>
	/// ディスクリプタヒープ取得
	/// </summary>
	/// <returns>ディスクリプタヒープポインタ</returns>
	ID3D12DescriptorHeap* GetHeap();

	/// <summary>
	/// CPU ハンドル取得
	/// </summary>
	/// <param name="a_number">生成時のインデックス</param>
	const D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(RTVHandle a_handleIndex) const;

protected:

	// デバイスのポインタ
	ID3D12Device* m_pDevice = nullptr;

	UINT m_incrementSize = 0;									// 移動距
	ComPtr<ID3D12DescriptorHeap> m_cpHeap = nullptr;			// ディスクリプタヒープ本体

	// 使用インデックスの待ち行列
	std::queue<RTVHandle> m_indexQueue;
};