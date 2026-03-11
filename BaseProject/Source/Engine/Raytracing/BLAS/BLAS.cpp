#include "BLAS.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

bool Engine::Raytracing::BLAS::Build(
	ID3D12Device5* a_pDevice, 
	ID3D12GraphicsCommandList4* a_cmdList, 
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec
)
{
	auto _pDevice = D3D12Wrapper::Instance().GetDevice5();

	// スクラッチリソース構築
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
	_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	_inputs.NumDescs = 1;
	_inputs.pGeometryDescs = a_geometryDescVec.data();
	_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO _info;
	_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&_inputs,&_info);

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



	return false;
}
