#include "ShaderTable.h"

#include "../RaytracingWorld/RaytracingWorld.h"
#include "../RayPSO/RayPSO.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../Graphics/RenderContext/RenderContext.h"

void Engine::Raytracing::ShaderTable::Init(const ShaderTableInit& a_shaderInit)
{
	m_maxLocalRootSigSize = a_shaderInit.maxLocalRootSize;

	// シェーダーID計算
	CalucShaderNum(a_shaderInit.pRayPSO, a_shaderInit.shaderData, a_shaderInit.hitGroup);

	// シェーダーサイズ計算
	CalucShaderTableSize(a_shaderInit.maxInstance);

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
	auto* _pDevice = Engine::D3D12::D3D12Wrapper::Instance().GetDevice5();
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
}

void Engine::Raytracing::ShaderTable::Release()
{
	// リソースがマップされていればアンマップする
	if (m_cpShaderTable && m_pShaderTableData)
	{
		m_cpShaderTable->Unmap(0, nullptr);
	}

	// マップ用ポインタを初期化
	m_pShaderTableData = nullptr;

	// リソース本体の解放
	if (m_cpShaderTable)
	{
		m_cpShaderTable.Reset();
	}

	// ID群（ベクター）のクリア
	m_missIDVec.clear();
	m_hitIDVec.clear();
	m_rayGenID = nullptr;

	// ディスパッチレイ構造体のクリア
	m_dispatchDesc = {};

	// 各種サイズ・オフセット変数の初期化（再利用時のバグ防止）
	m_maxLocalRootSigSize = 0;

	m_rayGenOffset = 0;
	m_missOffset = 0;
	m_hitOffset = 0;

	m_tableSize = 0;
	m_recordSize = 0;
}

void Engine::Raytracing::ShaderTable::CommitInstanceBindLess(
	const std::vector<Instance>& a_instanceVec, 
	Graphics::RenderContext* a_pRCT,
	UINT a_width,
	UINT a_height
)
{

	// レイジェネレーションシェーダー
	assert(m_rayGenID);
	memcpy(m_pShaderTableData + m_rayGenOffset, m_rayGenID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// ミスシェーダーID
	for (UINT _i = 0; _i < m_missIDVec.size(); ++_i)
	{
		assert(m_missIDVec[_i]);
		uint8_t* _missPtr = m_pShaderTableData + m_missOffset + _i * m_recordSize;	// アドレス
		memcpy(_missPtr, m_missIDVec[_i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}

	// ヒットシェーダー
	for (size_t _h = 0; _h < m_hitIDVec.size(); ++_h)
	{
		assert(m_hitIDVec[_h]);
		uint8_t* _hitPtr = m_pShaderTableData + m_hitOffset + (_h * m_recordSize);
		memcpy(_hitPtr, m_hitIDVec[_h], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}

	m_dispatchDesc = CreateDispatchDesc(a_instanceVec.size(),a_width,a_height);
}

const D3D12_DISPATCH_RAYS_DESC& Engine::Raytracing::ShaderTable::GetDispatchDesc()
{
	return m_dispatchDesc;
}

D3D12_DISPATCH_RAYS_DESC Engine::Raytracing::ShaderTable::CreateDispatchDesc(
	UINT a_instnaceNum,
	UINT a_width,
	UINT a_height
)
{
	// シェーダーテーブル設定決定(ディスパッチレイ構造体作成)
	D3D12_DISPATCH_RAYS_DESC _dispatchDesc = {};
	uint64_t _base = m_cpShaderTable->GetGPUVirtualAddress();

	_dispatchDesc.RayGenerationShaderRecord.StartAddress = _base + m_rayGenOffset;
	_dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_recordSize;

	_dispatchDesc.MissShaderTable.StartAddress = _base + m_missOffset;
	_dispatchDesc.MissShaderTable.SizeInBytes = m_recordSize * (UINT)m_missIDVec.size();
	_dispatchDesc.MissShaderTable.StrideInBytes = m_recordSize;

	_dispatchDesc.HitGroupTable.StartAddress = _base + m_hitOffset;
	_dispatchDesc.HitGroupTable.SizeInBytes = m_recordSize * (UINT)m_hitIDVec.size();
	_dispatchDesc.HitGroupTable.StrideInBytes = m_recordSize;

	// ディスプレイ設定
	_dispatchDesc.Width = a_width;
	_dispatchDesc.Height = a_height;
	_dispatchDesc.Depth = 1;

	return _dispatchDesc;
}

void Engine::Raytracing::ShaderTable::CalucShaderTableSize(UINT a_instanceNum)
{
	// シェーダーIDのサイズ
	uint32_t _shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	// ローカルルートシグネチャサイズ
	uint32_t _localRootSize = m_maxLocalRootSigSize;

	// シェーダー一つ分のサイズ
	m_recordSize = Alignment::Up(
		_shaderIDSize + _localRootSize,
		D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
	);

	// 各シェーダーごとのオフセット値を求める
	uint32_t _offset = 0;
	m_rayGenOffset = _offset;
	_offset = Alignment::Up(
		_offset + m_recordSize,
		D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
	);
	
	m_missOffset = _offset;
	_offset = Alignment::Up(
		_offset + m_recordSize * m_missIDVec.size(),
		D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
	);
	
	m_hitOffset = _offset;
	_offset = Alignment::Up(
		_offset + m_recordSize * m_hitIDVec.size(),
		D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
	);

	// テーブルサイズを求める（ヒットシェーダーはインスタンス数確保）
	m_tableSize = _offset;
}

void Engine::Raytracing::ShaderTable::CalucShaderNum(
	const RayPSO* a_rayPSO,
	const std::vector<RayShaderData>& a_shaderData, 
	const std::vector<HitGroup>& a_hitGroup
)
{
	// シェーダーIDを取得
	for(auto& _shader : a_shaderData)
	{
		switch (_shader.category)
		{
		case ShaderCategory::RayGenerator:
			m_rayGenID =  a_rayPSO->GetShaderID(_shader.entryName);
			break;
		case ShaderCategory::Miss:
			m_missIDVec.push_back(a_rayPSO->GetShaderID(_shader.entryName));
			break;
		case ShaderCategory::ClosestHit:
			break;
		case ShaderCategory::AnyHit:
			break;
		default:
			assert(0 && "不正なシェーダーカテゴリー");
			break;
		}
	}

	// ヒットグループのシェーダーIDを取得
	for (auto& _hitGroup : a_hitGroup)
	{
		m_hitIDVec.push_back(a_rayPSO->GetShaderID(_hitGroup.name));
	}
}


D3D12_GPU_DESCRIPTOR_HANDLE Engine::Raytracing::ShaderTable::GetTextureGPUHandle(const Resource::Material* a_pMaterial, Graphics::RenderContext* a_pRCT)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuHandles = {};
	const auto* _pBaseTex = Resource::ResourceManager::Instance().Get(a_pMaterial->baseColorTex);
	const auto* _pMetaTex = Resource::ResourceManager::Instance().Get(a_pMaterial->metaRoughTex);
	const auto* _pEmiTex = Resource::ResourceManager::Instance().Get(a_pMaterial->emissiveTex);
	const auto* _pNormalTex = Resource::ResourceManager::Instance().Get(a_pMaterial->normalTex);

	_cpuHandles.push_back(D3D12::DescriptorHeapManager::Instance().GetCPU(_pBaseTex->GetSRV()));
	_cpuHandles.push_back(D3D12::DescriptorHeapManager::Instance().GetCPU(_pMetaTex->GetSRV()));
	_cpuHandles.push_back(D3D12::DescriptorHeapManager::Instance().GetCPU(_pEmiTex->GetSRV()));
	_cpuHandles.push_back(D3D12::DescriptorHeapManager::Instance().GetCPU(_pNormalTex->GetSRV()));

	return a_pRCT->GetGPUHandle(_cpuHandles);
}
