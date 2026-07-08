#include "BLAS.h"

#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

void Engine::Raytracing::BLAS::Release()
{
	m_cpResource.Reset();
	m_cpUpdateScratch.Reset();

	m_geometryDescVec.clear();
}

void Engine::Raytracing::BLAS::UAVBarrier(D3D12::GraphicsCommandList* a_pCmdList) const
{
	if (!m_cpResource) return;
	D3D12_RESOURCE_BARRIER _barrier = {};
	_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	_barrier.UAV.pResource = m_cpResource.Get();
	a_pCmdList->ResourceBarrier(1, &_barrier);
}

void Engine::Raytracing::BLAS::SetName(LPCWSTR a_name)
{
	m_cpResource->SetName(a_name);
}

bool Engine::Raytracing::BLAS::BuildInternal(
	D3D12::Device* a_pDevice,
	D3D12::GraphicsCommandList* a_pCmdList, 
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS a_buildFlags, 
	bool a_isUpdate
)
{
	m_geometryDescVec = a_geometryDescVec;

	// スクラッチリソース構築
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
	_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_inputs.Flags = a_buildFlags;
	_inputs.NumDescs = static_cast<UINT>(a_geometryDescVec.size());
	_inputs.pGeometryDescs = a_geometryDescVec.data();
	_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	// BLASのサイズを問い合わせる
	// ResultDataMaxSizeBytes == BLASサイズ
	// ScratchDataSizeInBytes == ビルド用スクラッチ
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO _info;
	a_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&_inputs, &_info);

	// 動的な場合、初回ビルド用と更新用の大きいほうを採用する
	UINT64 _scratchSizeInBytes = a_isUpdate ?
		std::max(_info.ScratchDataSizeInBytes, _info.UpdateScratchDataSizeInBytes) :
		_info.ScratchDataSizeInBytes;

	// ヒーププロパティとバッファ設定（CD3DX12ヘルパーでスッキリ記述）
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto _scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
		_scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);

	// 生成
	a_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_scratchDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_cpUpdateScratch.ReleaseAndGetAddressOf())
	);

	auto barrierAS2 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_cpUpdateScratch.Get(),
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
	_buildDesc.ScratchAccelerationStructureData = m_cpUpdateScratch->GetGPUVirtualAddress();
	_buildDesc.DestAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	a_pCmdList->BuildRaytracingAccelerationStructure(&_buildDesc, 0, nullptr);

	// UAVバリア
	D3D12_RESOURCE_BARRIER _barrier = {};
	_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_barrier.UAV.pResource = m_cpResource.Get();

	a_pCmdList->ResourceBarrier(1, &_barrier);

	

	return true;
}

void Engine::Raytracing::BLAS::CreateStatic(
	D3D12::Device* a_pDevice, 
	D3D12::GraphicsCommandList* a_pCmdList,
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS a_buildFlags
)
{
	// スタティックモデル
	m_isDynamic = false;
	// 初回ビルドの実行
	BuildInternal(a_pDevice, a_pCmdList, a_geometryDescVec, a_buildFlags, false);
}

void Engine::Raytracing::BLAS::CreateDynamic(
	D3D12::Device* a_pDevice, 
	D3D12::GraphicsCommandList* a_pCmdList, 
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_animatedGeometries
)
{
	// ALLOW_UPDATE フラグを立てて、更新とビルド速度を優先する構成
	// FAST_TRACE にすると更新負荷が跳ね上がるから、動的モデルは FAST_BUILD
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS _flags =
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;

	// ダイナミックモデル
	m_isDynamic = true;

	// 初回ビルドの実行
	BuildInternal(a_pDevice, a_pCmdList, a_animatedGeometries, _flags, false);
}

void Engine::Raytracing::BLAS::Update(D3D12::GraphicsCommandList* a_pCmdList)
{
	if (!m_isDynamic) 
	{
		ENGINE_WARNING("静的BLASを更新しようとしています");
		return;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC _buildDesc = {};
	_buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	_buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_buildDesc.Inputs.NumDescs = static_cast<UINT>(m_geometryDescVec.size());
	_buildDesc.Inputs.pGeometryDescs = m_geometryDescVec.data();

	// 更新フラグを立てる
	_buildDesc.Inputs.Flags =
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

	// In-PlaceUpdate : SourceとDestinationに同じBLASアドレスを指定して上書き更新をする
	_buildDesc.SourceAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	_buildDesc.DestAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();

	// 更新用のスクラッチバッファを使用する
	_buildDesc.ScratchAccelerationStructureData = m_cpUpdateScratch->GetGPUVirtualAddress();
	a_pCmdList->BuildRaytracingAccelerationStructure(&_buildDesc, 0, nullptr);
}
