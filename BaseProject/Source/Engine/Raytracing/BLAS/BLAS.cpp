#include "BLAS.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../D3D12/D3DObject/CommandList/CommandList.h"

void Engine::Raytracing::BLAS::Create(const D3D12::DynamicVertexBuffer<Resource::RTVertex>& a_vertexBuffer, const D3D12::DynamicIndexBuffer& a_indexBuffer)
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
	Create(_desc);
}

void Engine::Raytracing::BLAS::Create(const D3D12_RAYTRACING_GEOMETRY_DESC& a_desc)
{
	// BLAS生成
	auto* _pDevice5 = D3D12::D3D12Wrapper::Instance().GetDevice5();
	//auto* _pCmdList4 = D3D12::D3D12Wrapper::Instance().GetCommandList4();
	auto* _pCmdList4 = Resource::ResourceManager::Instance().GetCmdList()->Get4();
	m_geometryDescVec.clear();
	m_geometryDescVec.push_back(a_desc);
	Build(
		_pDevice5,
		_pCmdList4,
		m_geometryDescVec
	);
}

void Engine::Raytracing::BLAS::Create(const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_desc)
{
	// BLAS生成
	auto* _pDevice5 = D3D12::D3D12Wrapper::Instance().GetDevice5();
	//auto* _pCmdList4 = D3D12::D3D12Wrapper::Instance().GetCommandList4();
	auto* _pCmdList4 = Resource::ResourceManager::Instance().GetCmdList()->Get4();
	m_geometryDescVec.clear();
	m_geometryDescVec = a_desc;
	Build(
		_pDevice5,
		_pCmdList4,
		m_geometryDescVec
	);
}


bool Engine::Raytracing::BLAS::Build(
	ID3D12Device5* a_pDevice, 
	ID3D12GraphicsCommandList4* a_cmdList, 
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec
)
{
	// コマンドキューリセット
	//D3D12::D3D12Wrapper::Instance().CommandQueueReset();
	//Resource::ResourceManager::Instance().CmdQueueReset();

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
	a_cmdList->ResourceBarrier(1, &barrierAS2);

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
	a_cmdList->BuildRaytracingAccelerationStructure(&_buildDesc, 0, nullptr);

	// UAVバリア
	D3D12_RESOURCE_BARRIER _barrier = {};
	_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_barrier.UAV.pResource = m_cpResource.Get();

	a_cmdList->ResourceBarrier(1,&_barrier);
	//a_cmdList->Close();

	// コマンドキューに積む
	//ID3D12CommandList* _ppCommandLists[] = { a_cmdList };
	//auto* _cmdQueue = D3D12::D3D12Wrapper::Instance().GetCopyCommandQueue();
	//_cmdQueue->ExecuteCommandLists(std::size(_ppCommandLists), _ppCommandLists);

	// 終了待ち
	//D3D12::D3D12Wrapper::Instance().SignalRenderFence();
	//D3D12::D3D12Wrapper::Instance().WaitRender();
	//Resource::ResourceManager::Instance().SignalFence(_cmdQueue);
	//Resource::ResourceManager::Instance().WaitRender();



	return true;
}
