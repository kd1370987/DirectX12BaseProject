#include "TLAS.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

// CD3DX12_* のヘルパーはここだけで使う。
// プリコンパイル済みヘッダーへ置くと全翻訳単位に広がるため
#pragma warning(push, 0)
#include "d3dx12.h"
#pragma warning(pop)

void Engine::Raytracing::TLAS::Create(
	D3D12::Device* a_pDevice,
	D3D12::DescriptorHeapManager* a_pHeapManager,
	D3D12::GraphicsCommandList* a_pCmdList,
	UINT a_maxInstanceNum
)
{
	// 参照
	uint64_t _tlasSize;

	// インプット情報
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
	_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	_inputs.NumDescs = a_maxInstanceNum;
	_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	// 出力構造体
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO _info;
	a_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&_inputs,&_info);
	uint64_t _scratchSize = static_cast<uint64_t>(Math::Alignment::Up(
		static_cast<size_t>(_info.ScratchDataSizeInBytes),
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT
	));
	uint64_t _resultSize = static_cast<uint64_t>(Math::Alignment::Up(
		static_cast<size_t>(_info.ResultDataMaxSizeInBytes),
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT
	));


	D3D12_HEAP_PROPERTIES _defaultHeapProp = {
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,0
	};

	// スクラッチバッファ作成
	CreateBuffer(
		a_pDevice,
		a_pCmdList,
		m_cpScratch,
		_scratchSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON,
		_defaultHeapProp
	);
	// リザルトバッファ作成
	CreateBuffer(
		a_pDevice,
		a_pCmdList,
		m_cpResource,
		_resultSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		_defaultHeapProp
	);

	// インスタンスバッファ作成 : フレームの数だけ区画を並べる
	m_maxInstanceCount = a_maxInstanceNum;
	m_isOverflowReported = false;
	const size_t _instanceBufferSize =
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * m_maxInstanceCount * CPU_FRAME_COUNT;

	D3D12_HEAP_PROPERTIES _uploadHeapProp = {
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};
	CreateBuffer(
		a_pDevice,
		a_pCmdList,
		m_cpInstanceBuffer,
		_instanceBufferSize,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		_uploadHeapProp
	);

	// サイズ記録
	_tlasSize = _info.ResultDataMaxSizeInBytes;
	

	// インスタンスバッファ構造体初期化・マップポイント取得
	m_pInstanceDesc = nullptr;
	m_cpInstanceBuffer->Map(0,nullptr,(void**)&m_pInstanceDesc);
	ZeroMemory(m_pInstanceDesc, _instanceBufferSize);

	// TLASを作成
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC _asDesc = {};
	_asDesc.Inputs = _inputs;
	_asDesc.Inputs.InstanceDescs = m_cpInstanceBuffer->GetGPUVirtualAddress();
	_asDesc.DestAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	_asDesc.ScratchAccelerationStructureData = m_cpScratch->GetGPUVirtualAddress();

	// Scratch
	auto barrierScratch = CD3DX12_RESOURCE_BARRIER::Transition(
		m_cpScratch.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	a_pCmdList->ResourceBarrier(1, &barrierScratch);


	a_pCmdList->BuildRaytracingAccelerationStructure(&_asDesc, 0, nullptr);


	// レイトレーシングアクセラレーション構造のビルド官僚待ちのバリア
	D3D12_RESOURCE_BARRIER _uavBarrier = {};
	_uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_uavBarrier.UAV.pResource = m_cpResource.Get();
	a_pCmdList->ResourceBarrier(1,&_uavBarrier);

	// SRVとして登録
	D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
	_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	_srvDesc.RaytracingAccelerationStructure.Location = GetGPUAddress();
	// 返却先を控える
	m_pHeapManager = a_pHeapManager;
	m_srvHandle = a_pHeapManager->Allocate<D3D12::SRV>(a_pDevice,nullptr,&_srvDesc);

}

void Engine::Raytracing::TLAS::Release()
{

	m_pInstanceDesc = nullptr;

	m_cpInstanceBuffer.Reset();
	m_cpResource.Reset();
	m_cpScratch.Reset();

	if (m_pHeapManager)
	{
		m_pHeapManager->Free(m_srvHandle);
		m_srvHandle = {};
		m_pHeapManager = nullptr;
	}
}

void Engine::Raytracing::TLAS::Update(D3D12::GraphicsCommandList* a_pCmdList, const std::vector<Instance>& a_instanceVec, UINT a_frameIndex)
{
	if (!m_pInstanceDesc || m_maxInstanceCount == 0) return;
	assert(a_frameIndex < CPU_FRAME_COUNT && "TLAS::Update : フレーム番号が範囲外です");

	// 今のフレームの区画 : 前のフレームのビルドが読んでいる区画には触らない
	const UINT _slot = a_frameIndex % CPU_FRAME_COUNT;
	D3D12_RAYTRACING_INSTANCE_DESC* _pDescs = m_pInstanceDesc + static_cast<size_t>(_slot) * m_maxInstanceCount;

	//------------------------------------------------------------------------------------------
	// インスタンスを詰めて書く
	//
	// BLAS がまだ無いインスタンスは飛ばすが、書く位置は別に数えて詰める。
	// 添字のまま飛ばすと、その位置に前回の中身(解放済みの BLAS を指していることもある)が
	// 残ったまま NumDescs に数えられ、GPU がそれを辿ってデバイスごと落ちる。
	//
	// InstanceID は詰める前の添字のままにする : シェーダーは InstanceID() で
	// インスタンスデータ(RayWorld が同じ並びで積んでいる)を引くため
	//------------------------------------------------------------------------------------------
	uint32_t _count = 0;
	for (size_t _i = 0; _i < a_instanceVec.size(); ++_i)
	{
		const Instance& _instance = a_instanceVec[_i];
		if (_instance.pBLAS == nullptr || _instance.pBLAS->GetGPUAddress() == 0) continue;

		// 区画に入り切らないぶんは描かない(隣のフレームの区画を踏むため)
		if (_count >= m_maxInstanceCount)
		{
			if (!m_isOverflowReported)
			{
				ENGINE_WARNING("[TLAS] インスタンス数が上限(%u)を超えました。超えたぶんはレイトレに載りません", m_maxInstanceCount);
				m_isOverflowReported = true;
			}
			break;
		}

		D3D12_RAYTRACING_INSTANCE_DESC& _desc = _pDescs[_count];
		_desc.InstanceID = static_cast<UINT>(_i);
		_desc.InstanceContributionToHitGroupIndex = 0;
		_desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		_desc.AccelerationStructure = _instance.pBLAS->GetGPUAddress();
		_desc.InstanceMask = 0xFF;

		const auto& m = _instance.worldMat;
		_desc.Transform[0][0] = m._11;
		_desc.Transform[0][1] = m._21;
		_desc.Transform[0][2] = m._31;
		_desc.Transform[0][3] = m._41;

		_desc.Transform[1][0] = m._12;
		_desc.Transform[1][1] = m._22;
		_desc.Transform[1][2] = m._32;
		_desc.Transform[1][3] = m._42;

		_desc.Transform[2][0] = m._13;
		_desc.Transform[2][1] = m._23;
		_desc.Transform[2][2] = m._33;
		_desc.Transform[2][3] = m._43;

		++_count;
	}

	// インプット情報
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
	_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	_inputs.NumDescs = _count;
	_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	// TLASを作成
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC _asDesc = {};
	_asDesc.Inputs = _inputs;
	_asDesc.Inputs.InstanceDescs =
		m_cpInstanceBuffer->GetGPUVirtualAddress() +
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * static_cast<UINT64>(_slot) * m_maxInstanceCount;
	_asDesc.DestAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	_asDesc.ScratchAccelerationStructureData = m_cpScratch->GetGPUVirtualAddress();

	a_pCmdList->BuildRaytracingAccelerationStructure(&_asDesc, 0, nullptr);

	// レイトレーシングアクセラレーション構造のビルド完了待ちのバリア
	D3D12_RESOURCE_BARRIER _uavBarrier = {};
	_uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_uavBarrier.UAV.pResource = m_cpResource.Get();
	a_pCmdList->ResourceBarrier(1, &_uavBarrier);
}


D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::TLAS::GetGPUHandle()
{
	return m_pHeapManager->GetGPU(m_srvHandle);
}

void Engine::Raytracing::TLAS::CreateBuffer(
	D3D12::Device * a_pDevice, 
	D3D12::GraphicsCommandList* a_pCmdList,
	ComPtr<ID3D12Resource>& a_cpRes,
	uint64_t a_size,
	D3D12_RESOURCE_FLAGS a_flags,
	D3D12_RESOURCE_STATES a_initState,
	const D3D12_HEAP_PROPERTIES & a_heapProps
)
{
	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Alignment = 0;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Flags = a_flags;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.Height = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Width = a_size;

	a_pDevice->CreateCommittedResource(&a_heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, a_initState, nullptr, IID_PPV_ARGS(a_cpRes.ReleaseAndGetAddressOf()));
	if (a_cpRes) a_cpRes->SetName(L"TLAS_Buffer");	// リーク調査用
}
