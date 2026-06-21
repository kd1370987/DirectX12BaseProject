#include "BLAS.h"

#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

void Engine::Raytracing::BLAS::Create(
	D3D12::Device* a_pDevice,
	D3D12::GraphicsCommandList* a_pCmdList,
	const D3D12::DynamicVertexBuffer<Resource::RTVertex>& a_vertexBuffer, 
	const D3D12::DynamicIndexBuffer& a_indexBuffer
)
{
	// ジオメトリ情報生成
	D3D12_RAYTRACING_GEOMETRY_DESC _desc = {};
	_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	_desc.Triangles.VertexBuffer.StartAddress = a_vertexBuffer.GetGPUVirtualAddress();
	_desc.Triangles.VertexBuffer.StrideInBytes = a_vertexBuffer.GetStrideSize();
	_desc.Triangles.VertexCount = a_vertexBuffer.GetElementNum();
	_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

	_desc.Triangles.IndexBuffer = a_indexBuffer.GetGPUVirtualAddress();
	_desc.Triangles.IndexCount = a_indexBuffer.GetElementNum();
	_desc.Triangles.IndexFormat = a_indexBuffer.GetView().Format;

	_desc.Triangles.Transform3x4 = 0;

	// BLAS生成
	Create(a_pDevice,a_pCmdList,_desc);
}

void Engine::Raytracing::BLAS::Release()
{
	m_cpResource.Reset();
	m_cpScratch.Reset();

	m_geometryDescVec.clear();
}

void Engine::Raytracing::BLAS::Create(
	D3D12::Device* a_pDevice,
	D3D12::GraphicsCommandList* a_pCmdList,
	const D3D12_RAYTRACING_GEOMETRY_DESC& a_desc
)
{
	// BLAS生成
	m_geometryDescVec.clear();
	m_geometryDescVec.push_back(a_desc);
	Build(
		a_pDevice,
		a_pCmdList,
		m_geometryDescVec
	);
}

void Engine::Raytracing::BLAS::Create(
	D3D12::Device* a_pDevice,
	D3D12::GraphicsCommandList* a_pCmdList,
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_desc
)
{
	// BLAS生成
	m_geometryDescVec.clear();
	m_geometryDescVec = a_desc;
	Build(
		a_pDevice,
		a_pCmdList,
		m_geometryDescVec
	);
}


bool Engine::Raytracing::BLAS::Build(
	D3D12::Device* a_pDevice,
	D3D12::GraphicsCommandList* a_pCmdList,
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec
)
{

	// スクラッチリソース構築
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
	_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	_inputs.NumDescs = static_cast<UINT>(a_geometryDescVec.size());
	_inputs.pGeometryDescs = a_geometryDescVec.data();
	_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	// BLASのサイズを問い合わせる
	// ResultDataMaxSizeBytes == BLASサイズ
	// ScratchDataSizeInBytes == ビルド用スクラッチ
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO _info;
	a_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&_inputs,&_info);

	// 設定
	D3D12_RESOURCE_DESC _bufDesc = {};
	_bufDesc.Alignment = 0;
	_bufDesc.DepthOrArraySize = 1;
	_bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	_bufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	_bufDesc.Height = 1;
	_bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	_bufDesc.MipLevels = 1;
	_bufDesc.SampleDesc.Count = 1;
	_bufDesc.SampleDesc.Quality = 0;
	_bufDesc.Width = _info.ScratchDataSizeInBytes;

	// ヒーププロパティ
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// 生成
	a_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_bufDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_cpScratch.ReleaseAndGetAddressOf())
	);

	auto barrierAS2 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_cpScratch.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	a_pCmdList->ResourceBarrier(1, &barrierAS2);

	// BLASバッファ作成
	auto _blasDesc = CD3DX12_RESOURCE_DESC::Buffer(
		_info.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);

	a_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_blasDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(m_cpResource.ReleaseAndGetAddressOf())
	);

	// Buildコマンド発行
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC _buildDesc{};
	_buildDesc.Inputs = _inputs;
	_buildDesc.ScratchAccelerationStructureData = m_cpScratch->GetGPUVirtualAddress();
	_buildDesc.DestAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	a_pCmdList->BuildRaytracingAccelerationStructure(&_buildDesc, 0, nullptr);

	// UAVバリア
	D3D12_RESOURCE_BARRIER _barrier = {};
	_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_barrier.UAV.pResource = m_cpResource.Get();

	a_pCmdList->ResourceBarrier(1,&_barrier);

	return true;
}
