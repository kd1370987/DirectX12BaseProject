#include "ShaderTable.h"

#include "../RaytracingWorld/RaytracingWorld.h"
#include "../RayPSO/RayPSO.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../Resource/Manager/TextureManager/TextureManager.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void Engine::Raytracing::ShaderTable::Init(
	const Engine::Raytracing::RayWorld& a_rayWorld,
	Engine::Raytracing::RayPSO& a_rayPSO
)
{
	// シェーダーID取得
	void* _raygenID = a_rayPSO.GetShaderID("RayGen");
	void* _missID = a_rayPSO.GetShaderID("Miss");
	void* _hitID = a_rayPSO.GetShaderID("HitGroup");

	assert(_raygenID);
	assert(_missID);
	assert(_hitID);

	// インスタンス配列
	auto& _instanceVec = a_rayWorld.GetInstnace();

	// シェーダーサイズ計算
	//CalucShaderTableSize(_instanceVec.size());
	CalucShaderTableSize(1000);

	// 仕様書
	D3D12_RESOURCE_DESC _desc = {};
	_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	_desc.Width = m_tableSize;
	_desc.Height = 1;
	_desc.DepthOrArraySize = 1;
	_desc.MipLevels = 1;
	_desc.SampleDesc.Count = 1;
	_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// ヒープ作成
	auto _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// シェーダーテーブル作成
	auto* _pDevice = D3D12Wrapper::Instance().GetDevice5();
	auto _hr = _pDevice->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_cpShaderTable)
	);
	if (FAILED(_hr))
	{
		assert(0 && "シェーダーテーブル作成に失敗");
		return;
	}

	// 更新
	m_cpShaderTable->Map(0, nullptr, (void**)&m_pShaderTableData);

	memcpy(m_pShaderTableData + m_rayGenOffset, _raygenID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	memcpy(m_pShaderTableData + m_missOffset, _missID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// ヒットシェーダーのデータ
	for (size_t _i = 0; _i < _instanceVec.size(); ++_i)
	{
		// インスタンス取得
		auto& _instance = _instanceVec[_i];

		uint8_t* _hitPtr = m_pShaderTableData + m_hitOffset + _i * m_recordSize;	// アドレス
		memcpy(_hitPtr, _hitID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);	// シェーダーIDコピー

		// ハンドル格納先ポインタアドレス
		auto* _handles = 
			reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(_hitPtr + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		_handles[0] = GetTextureGPUHandle(_instance.pMaterial->baseColorTex);
		_handles[1] = DescriptorHeapManager::Instance().GetSRVGPUHandle(_instance.indexHandle);
		_handles[2] = DescriptorHeapManager::Instance().GetSRVGPUHandle(_instance.vertexHandle);
	}

	// シェーダーテーブル設定決定(ディスパッチレイ構造体作成)
	m_dispatchDesc = {};
	m_dispatchDesc = CreateDispatchDesc(_instanceVec.size());
}

void Engine::Raytracing::ShaderTable::Update(const RayWorld& a_rayWorld, Engine::Raytracing::RayPSO& a_rayPSO)
{
	// シェーダーID取得
	void* _raygenID = a_rayPSO.GetShaderID("RayGen");
	void* _missID = a_rayPSO.GetShaderID("Miss");
	void* _hitID = a_rayPSO.GetShaderID("HitGroup");

	assert(_raygenID);
	assert(_missID);
	assert(_hitID);

	// インスタンス配列
	auto& _instanceVec = a_rayWorld.GetInstnace();

	memcpy(m_pShaderTableData + m_rayGenOffset, _raygenID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	memcpy(m_pShaderTableData + m_missOffset, _missID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// ヒットシェーダーのデータ
	for (size_t _i = 0; _i < _instanceVec.size(); ++_i)
	{
		// インスタンス取得
		auto& _instance = _instanceVec[_i];

		uint8_t* _hitPtr = m_pShaderTableData + m_hitOffset + _i * m_recordSize;	// アドレス
		memcpy(_hitPtr, _hitID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);	// シェーダーIDコピー

		// ハンドル格納先ポインタアドレス
		auto* _handles =
			reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(_hitPtr + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		_handles[0] = GetTextureGPUHandle(_instance.pMaterial->baseColorTex);
		_handles[1] = DescriptorHeapManager::Instance().GetSRVGPUHandle(_instance.indexHandle);
		_handles[2] = DescriptorHeapManager::Instance().GetSRVGPUHandle(_instance.vertexHandle);
	}

	// シェーダーテーブル設定決定(ディスパッチレイ構造体作成)
	m_dispatchDesc = CreateDispatchDesc(_instanceVec.size());
}

D3D12_DISPATCH_RAYS_DESC Engine::Raytracing::ShaderTable::CreateDispatchDesc(UINT a_instnaceNum)
{
	// シェーダーテーブル設定決定(ディスパッチレイ構造体作成)
	D3D12_DISPATCH_RAYS_DESC _dispatchDesc = {};
	uint64_t _base = m_cpShaderTable->GetGPUVirtualAddress();

	_dispatchDesc.RayGenerationShaderRecord.StartAddress = _base + m_rayGenOffset;
	_dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_recordSize;

	_dispatchDesc.MissShaderTable.StartAddress = _base + m_missOffset;
	_dispatchDesc.MissShaderTable.SizeInBytes = m_recordSize;
	_dispatchDesc.MissShaderTable.StrideInBytes = m_recordSize;

	_dispatchDesc.HitGroupTable.StartAddress = _base + m_hitOffset;
	_dispatchDesc.HitGroupTable.SizeInBytes = m_recordSize * a_instnaceNum;
	_dispatchDesc.HitGroupTable.StrideInBytes = m_recordSize;

	// ディスプレイ設定
	_dispatchDesc.Width = 1280;
	_dispatchDesc.Height = 720;
	_dispatchDesc.Depth = 1;

	return _dispatchDesc;
}

void Engine::Raytracing::ShaderTable::CalucShaderTableSize(UINT a_instanceNum)
{
	// シェーダーIDのサイズ
	uint32_t _shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	// ローカルルートシグネチャサイズ
	uint32_t _localRootSize = sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * 3;

	// シェーダー一つ分のサイズ
	m_recordSize = Alignment::Up(
		_shaderIDSize + _localRootSize,
		D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
	);

	// 各シェーダーごとのオフセット値を求める
	// レイ生成シェーダー開始位置
	m_rayGenOffset = 0;

	// ミスシェーダー開始位置
	m_missOffset = Alignment::Up(
		m_rayGenOffset + m_recordSize,
		D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
	);

	// ヒットシェーダー開始位置
	 m_hitOffset = Alignment::Up(
		m_missOffset + m_recordSize,
		D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
	);

	// テーブルサイズを求める（ヒットシェーダーはインスタンス数確保）
	m_tableSize = m_hitOffset + m_recordSize * a_instanceNum;
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::ShaderTable::GetTextureGPUHandle(
	const Resource::Handle<Resource::Texture>& a_texHandle
)
{
	auto& _tex = Resource::TextureManager::Instance().GetTexture(a_texHandle);


	return DescriptorHeapManager::Instance().GetSRVGPUHandle(_tex.GetSRV());
}
