#include "ShaderTable.h"

#include "../RaytracingWorld/RaytracingWorld.h"
#include "../RayPSO/RayPSO.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

void Engine::Raytracing::ShaderTable::Init(const Engine::Raytracing::RayWorld& a_rayWorld, Engine::Raytracing::RayPSO& a_rayPSO)
{
	void* _raygenID = a_rayPSO.GetShaderID("RayGen");
	void* _missID = a_rayPSO.GetShaderID("Miss");
	void* _hitID = a_rayPSO.GetShaderID("HitGroup");

	assert(_raygenID);
	assert(_missID);
	assert(_hitID);

	uint32_t _recordSize = Alignment::Up(
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
			D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
		);
	uint32_t _tableSize = _recordSize * 3;

	D3D12_RESOURCE_DESC _desc = {};
	_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	_desc.Width = _tableSize;
	_desc.Height = 1;
	_desc.DepthOrArraySize = 1;
	_desc.MipLevels = 1;
	_desc.SampleDesc.Count = 1;
	_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	auto _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

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
	uint8_t* _mapped;
	m_cpShaderTable->Map(0, nullptr, (void**)&_mapped);

	memcpy(_mapped, _raygenID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	memcpy(_mapped + _recordSize, _missID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	memcpy(_mapped + _recordSize * 2, _hitID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	m_cpShaderTable->Unmap(0, nullptr);

	// シェーダーテーブル設定決定(ディスパッチレイ構造体作成)
	D3D12_DISPATCH_RAYS_DESC _dispatchDesc = {};
	uint64_t _base = m_cpShaderTable->GetGPUVirtualAddress();

	_dispatchDesc.RayGenerationShaderRecord.StartAddress = _base;
	_dispatchDesc.RayGenerationShaderRecord.SizeInBytes = _recordSize;

	_dispatchDesc.MissShaderTable.StartAddress = _base + _recordSize;
	_dispatchDesc.MissShaderTable.SizeInBytes = _recordSize;
	_dispatchDesc.MissShaderTable.StrideInBytes = _recordSize;

	_dispatchDesc.HitGroupTable.StartAddress = _base + _recordSize * 2;
	_dispatchDesc.HitGroupTable.SizeInBytes = _recordSize;
	_dispatchDesc.HitGroupTable.StrideInBytes = _recordSize;

	_dispatchDesc.Width = 1280;
	_dispatchDesc.Height = 720;
	_dispatchDesc.Depth = 1;

	m_dispatchDesc = {};
	m_dispatchDesc = _dispatchDesc;
}

void Engine::Raytracing::ShaderTable::CountUpNumRayGenMissHitShader()
{
}
