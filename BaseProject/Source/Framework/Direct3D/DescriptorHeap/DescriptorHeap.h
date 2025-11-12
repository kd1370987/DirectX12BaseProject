#pragma once

class Texture2D;

class DescriptorHandle
{
public:
	D3D12_CPU_DESCRIPTOR_HANDLE HandoleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE HandoleGPU;
};

class DescriptorHeap
{
public:

	DescriptorHeap();									// コンストラクタで生成
	ID3D12DescriptorHeap* GetHeap();					// ディスクリプタヒープ本体を取得
	DescriptorHandle* Register(Texture2D* a_texture);	// テクスチャを登録してハンドルを取得

private:

	bool m_isValid = false;		// 生成に成功したどうか
	UINT m_incrementSize = 0;
	ComPtr<ID3D12DescriptorHeap> m_pHeap = nullptr;		// ディスクリプタヒープ本体
	std::vector<DescriptorHandle*> m_pHandles;			// 登録されているハンドル
};