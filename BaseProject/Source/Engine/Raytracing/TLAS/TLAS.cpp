#include "TLAS.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void Engine::Raytracing::TLAS::Create(const std::vector<Instance>& a_instanceVec)
{
	// GPU操作のためリセット
	D3D12Wrapper::Instance().CommandQueueReset();

	// 参照
	uint64_t _tlasSize;
	auto* _pDevice5 = D3D12Wrapper::Instance().GetDevice5();
	int _numInstance = static_cast<int>(a_instanceVec.size());
	ID3D12GraphicsCommandList4* _pCmdList = D3D12Wrapper::Instance().GetCommandList4();

	// インスタンスバッファ作成
	D3D12_HEAP_PROPERTIES _uploadHeapProp = {
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};
	CreateBuffer(
		_pDevice5,
		m_cpInstanceBuffer,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * _numInstance,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		_uploadHeapProp
	);

	// インプット情報
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
	_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	_inputs.NumDescs = _numInstance;
	_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	_inputs.InstanceDescs = m_cpInstanceBuffer->GetGPUVirtualAddress();

	// 出力構造体
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO _info;
	_pDevice5->GetRaytracingAccelerationStructurePrebuildInfo(&_inputs,&_info);
	uint64_t _scratchSize = static_cast<uint64_t>(Alignment::Up(
		static_cast<size_t>(_info.ScratchDataSizeInBytes),
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT
	));
	uint64_t _resultSize = static_cast<uint64_t>(Alignment::Up(
		static_cast<size_t>(_info.ResultDataMaxSizeInBytes),
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT
	));

	//if (a_isUpdate)
	//{
	//	更新
	//}
	//else 新規
	{
		D3D12_HEAP_PROPERTIES _defaultHeapProp = {
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,0
		};

		// スクラッチバッファ作成
		CreateBuffer(
			_pDevice5,
			m_cpScratch,
			_scratchSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			_defaultHeapProp
		);
		// リザルトバッファ作成
		CreateBuffer(
			_pDevice5,
			m_cpResource,
			_resultSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			_defaultHeapProp
		);



		// サイズ記録
		_tlasSize = _info.ResultDataMaxSizeInBytes;
	}

	// インスタンスバッファ構造体
	D3D12_RAYTRACING_INSTANCE_DESC* _pInstanceDesc = nullptr;
	m_cpInstanceBuffer->Map(0,nullptr,(void**)&_pInstanceDesc);
	ZeroMemory(_pInstanceDesc,sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * _numInstance);

	for (int _i = 0; _i < _numInstance; ++_i)
	{
		_pInstanceDesc[_i].InstanceID = _i;
		_pInstanceDesc[_i].InstanceContributionToHitGroupIndex = 0;
		_pInstanceDesc[_i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		_pInstanceDesc[_i].AccelerationStructure = a_instanceVec[_i].pBLAS->GetGPUAddress();
		auto& m = a_instanceVec[_i].worldMat;

		_pInstanceDesc[_i].Transform[0][0] = m._11;
		_pInstanceDesc[_i].Transform[0][1] = m._12;
		_pInstanceDesc[_i].Transform[0][2] = m._13;
		_pInstanceDesc[_i].Transform[0][3] = m._41;

		_pInstanceDesc[_i].Transform[1][0] = m._21;
		_pInstanceDesc[_i].Transform[1][1] = m._22;
		_pInstanceDesc[_i].Transform[1][2] = m._23;
		_pInstanceDesc[_i].Transform[1][3] = m._42;

		_pInstanceDesc[_i].Transform[2][0] = m._31;
		_pInstanceDesc[_i].Transform[2][1] = m._32;
		_pInstanceDesc[_i].Transform[2][2] = m._33;
		_pInstanceDesc[_i].Transform[2][3] = m._43;

		_pInstanceDesc[_i].InstanceMask = 0xFF;
	}

	m_cpInstanceBuffer->Unmap(0,nullptr);

	// TLASを作成
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC _asDesc = {};
	_asDesc.Inputs = _inputs;
	_asDesc.Inputs.InstanceDescs = m_cpInstanceBuffer->GetGPUVirtualAddress();
	_asDesc.DestAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	_asDesc.ScratchAccelerationStructureData = m_cpScratch->GetGPUVirtualAddress();

	//if (a_isUpdate)
	//{
	//	_asDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	//	_asDesc.SourceAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	//}
	_pCmdList->BuildRaytracingAccelerationStructure(&_asDesc, 0, nullptr);


	// レイトレーシングアクセラレーション構造のビルド官僚待ちのバリア
	D3D12_RESOURCE_BARRIER _uavBarrier = {};
	_uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_uavBarrier.UAV.pResource = m_cpResource.Get();
	_pCmdList->ResourceBarrier(1,&_uavBarrier);


	_pCmdList->Close();

	// コマンド実行
	ID3D12CommandList* _ppCmdLists[] = { _pCmdList };
	auto* _cmdQueue = D3D12Wrapper::Instance().GetCommandQueue();
	_cmdQueue->ExecuteCommandLists(std::size(_ppCmdLists), _ppCmdLists);

	// 終了待ち
	D3D12Wrapper::Instance().SignalRenderFence();
	D3D12Wrapper::Instance().WaitRender();


	// SRVとして登録
	D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
	_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	_srvDesc.RaytracingAccelerationStructure.Location = GetGPUAddress();
	SRVViewInit _init;
	_init.pResource = nullptr;
	_init.pDesc = &_srvDesc;
	m_srvHandle = DescriptorHeapManager::Instance().AllocateSRVRange({_init})[0];
}

void Engine::Raytracing::TLAS::Build(ID3D12GraphicsCommandList4* a_pCmdList)
{
}

D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::TLAS::GetGPUHandle()
{
	return DescriptorHeapManager::Instance().GetSRVGPUHandle(m_srvHandle);
}

void Engine::Raytracing::TLAS::CreateBuffer(ID3D12Device5 * a_pDevice, ComPtr<ID3D12Resource>& a_cpRes, uint64_t a_size, D3D12_RESOURCE_FLAGS a_flags, D3D12_RESOURCE_STATES a_initState, const D3D12_HEAP_PROPERTIES & a_heapProps)
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
}
