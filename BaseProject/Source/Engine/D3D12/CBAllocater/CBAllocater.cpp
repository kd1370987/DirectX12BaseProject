#include "CBAllocater.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void CBAllocater::Release()
{
	ResetUse();
	m_pDevice = nullptr;

	// グラフィックス用解放
	if (m_spResource && m_pMappedData)
	{
		m_spResource->Unmap(0,nullptr);
	}
	m_pMappedData = nullptr;
	m_spResource.Reset();

	// コンピュート用解放
	if (m_spComputeResource && m_pComputeMappedData)
	{
		m_spComputeResource->Unmap(0, nullptr);
	}
	m_pComputeMappedData = nullptr;
	m_spComputeResource.Reset();
}

void CBAllocater::RootCBVCreate(Engine::D3D12::Device* a_device, size_t a_memSize)
{
	m_pDevice = a_device;
	
	UINT64 _sizeAligned = ((a_memSize + 0xff) & ~0xff);

	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_sizeAligned);			// リソースの設定

	// リソースを生成
	auto _hr = m_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_spResource.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "定数バッファリソースの生成に失敗\n");
		return;
	}
	if (m_spResource) m_spResource->SetName(L"CBAllocator_Graphics");	// リーク調査用

	// 定数バッファをマッピング
	_hr = m_spResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedData));

	m_capacity = _sizeAligned;
	m_usedCount = 0;

	CreateCompute(a_memSize);
}

void CBAllocater::BindAndAttachDataRootCBV(Engine::D3D12::GraphicsCommandList* a_pCmdList, int a_descIndex, const void* a_data, size_t a_size)
{
	size_t _dataSize = (a_size + 0xff) & ~0xff; // 256バイトアライメント

	int _useValue = static_cast<int>(_dataSize / 0x100);
	if ((m_usedCount + _useValue) * 256 > m_capacity)
	{
		// ヒープに登録できる数を超えた
		assert(0 && "アップロードヒープの上限を迎えました");
		return;
	}

	// アドレス位置
	int _top = m_usedCount;

	// データ転送
	std::memcpy(&m_pMappedData[_top].buff, &a_data, a_size);

	// コマンドリストにセット
	a_pCmdList->SetGraphicsRootConstantBufferView(
		a_descIndex,
		m_spResource->GetGPUVirtualAddress() + ((UINT64)_top * 0x100)
	);

	m_usedCount += _useValue;
}

void CBAllocater::CreateCompute(size_t a_memSize)
{
	UINT64 _sizeAligned = ((a_memSize + 0xff) & ~0xff);

	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_sizeAligned);			// リソースの設定

	// リソースを生成
	auto _hr = m_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_spComputeResource.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "定数バッファリソースの生成に失敗\n");
		return;
	}
	if (m_spComputeResource) m_spComputeResource->SetName(L"CBAllocator_Compute");	// リーク調査用

	// 定数バッファをマッピング
	_hr = m_spComputeResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pComputeMappedData));

	m_computeCapacity = _sizeAligned;
	m_useComputeCount = 0;
}
