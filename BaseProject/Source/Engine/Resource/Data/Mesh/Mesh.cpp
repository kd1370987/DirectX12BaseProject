#include "Mesh.h"

#include "../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../../D3D12/D3DObject/CommandList/CommandList.h"

#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"
bool Engine::Resource::Mesh::CreateFloat(
	const std::vector<MeshVertexFloat>& a_vertices,
	const std::vector<MeshFace>& a_face,
	const std::vector<MeshSubset>& a_subsets,
	bool a_isSkinMesh
)
{
	m_vertices = a_vertices;
	m_face = a_face;
	m_subsets = a_subsets;
	m_isSkinMesh = a_isSkinMesh;

	auto _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

	// メタデータ作成
	CreateMeshMetaData(a_vertices,a_subsets,a_isSkinMesh);

	// ラスタライザーデータ作成
	CreateRasterData(_pDevice,a_vertices,a_face, DXGI_FORMAT_R32_UINT);
	
	// コマンドキューリセット
	auto* _pCmd = Engine::Resource::ResourceManager::Instance().GetCmdList();
	Engine::Resource::ResourceManager::Instance().CmdQueueReset();

	m_opRasterData->indexBuffer.CreateSRV(_pDevice);
	m_opRasterData->vertexBuffer.CreateSRV(_pDevice);

	// レイ用データ作成
	CreateRtData(
		_pDevice,
		_pCmd,
		a_subsets,
		m_opRasterData->vertexBuffer,
		DXGI_FORMAT_R32G32B32_FLOAT,
		m_opRasterData->indexBuffer,
		a_vertices,
		a_face
	);

	// コマンドリストをクローズ
	Engine::Resource::ResourceManager::Instance().GetCmdList()->NGet()->Close();

	// コマンドキューに積む
	ID3D12CommandList* _ppCommandLists[] = { Engine::Resource::ResourceManager::Instance().GetCmdList()->NGet() };
	auto* _cmdQueue = Engine::D3D12::D3D12Wrapper::Instance().GetCopyCommandQueue();
	_cmdQueue->ExecuteCommandLists(std::size(_ppCommandLists), _ppCommandLists);

	// 終了待ち
	ResourceManager::Instance().SignalFence(_cmdQueue);
	ResourceManager::Instance().WaitRender();
	return true;
}

void Engine::Resource::Mesh::CreateMeshMetaData(
	const std::vector<MeshVertexFloat>& a_vertices,
	const std::vector<MeshSubset>& a_subsets, 
	bool a_isSkinMesh
)
{
	m_meshMetaData.Create(a_vertices,a_subsets,a_isSkinMesh);
}

void Engine::Resource::Mesh::CreateRasterData(ID3D12Device* a_pDevice, const std::vector<MeshVertexFloat>& a_vertices, const std::vector<MeshFace>& a_face, DXGI_FORMAT a_indexFormat)
{
	auto& _raster = m_opRasterData.emplace();
	_raster.Create(a_pDevice, a_vertices, a_face, a_indexFormat);
}

void Engine::Resource::Mesh::CreateRtData(ID3D12Device* a_pDevice, D3D12::CommandList* a_pCmdList, const std::vector<MeshSubset>& a_subset, const D3D12::DynamicVertexBuffer<MeshVertexFloat>& a_vertexBuffer, DXGI_FORMAT a_vertexFarstFormat, const D3D12::DynamicIndexBuffer& a_indexBuffer, const std::vector<MeshVertexFloat>& a_vertices, const std::vector<MeshFace>& a_face)
{
	auto& _rtData = m_opRtData.emplace();
	_rtData.Create(a_pDevice,a_pCmdList,a_subset,a_vertexBuffer,a_vertexFarstFormat,a_indexBuffer,a_vertices,a_face);
}

void Engine::Resource::Mesh::CreateCollisionMesh(const std::vector<DirectX::XMFLOAT3>& a_vertices, const std::vector<UINT>& a_indices)
{
	auto& _collMesh = m_opCollMesh.emplace();
	_collMesh.Create(a_vertices,a_indices);
}

void Engine::Resource::Mesh::Release()
{
	// 各meshデータ解放
	m_meshMetaData.Release();
	if (m_opRasterData.has_value())
	{
		m_opRasterData->Release();
	}
	if (m_opCollMesh.has_value())
	{
		m_opCollMesh->Release();
	}
	if (m_opRtData.has_value())
	{
		m_opRtData->Release();
	}

	m_vertices.clear();
	m_face.clear();
	m_subsets.clear();
}

void Engine::Resource::Mesh::Save(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Save, a_fileDir, a_name, "mesh");

	// 頂点数の保存
	size_t _vertexCount = m_vertices.size();
	_ar.Field("VertexCount", _vertexCount);

	// 頂点データの保存
	int _v = 0;
	for (auto& _vert : m_vertices)
	{
		std::string _vStr = "Vert[" + std::to_string(_v) + "].";

		_ar.Field(_vStr + "Pos", _vert.pos);
		_ar.Field(_vStr + "Normal", _vert.normal);
		_ar.Field(_vStr + "UV", _vert.uv);
		_ar.Field(_vStr + "Tangent", _vert.tangent);
		_ar.Field(_vStr + "Color", _vert.color);

		int _i = 0;
		for (auto& _skIdx : _vert.skinIndexList)
		{
			_ar.Field(_vStr + "SkList" + std::to_string(_i), _skIdx);
			_i++;
		}
		_i = 0;
		for (auto& _skWeit : _vert.skinWeightList)
		{
			_ar.Field(_vStr + "SkWeit" + std::to_string(_i), _skWeit);
			_i++;
		}
		_v++;
	}

	// 面数の保存
	size_t _faceCount = m_face.size();
	_ar.Field("FaceCount", _faceCount);

	// 面データの保存
	int _i = 0;
	for (auto& _face : m_face)
	{
		int _j = 0;
		for (auto& _idx : _face.idx)
		{
			_ar.Field("Face" + std::to_string(_i) + "_" + std::to_string(_j), _idx);
			_j++;
		}
		_i++;
	}

	// サブセット数の保存
	size_t _subsetCount = m_subsets.size();
	_ar.Field("SubsetCount", _subsetCount);

	// サブセットの保存
	_i = 0;
	for (auto& _subset : m_subsets)
	{
		std::string _iStr = std::to_string(_i);
		_ar.Field("Subset_MaterialNumber_" + _iStr, _subset.materialNumber);
		_ar.Field("Subset_faceStart_" + _iStr, _subset.faceStart);
		_ar.Field("Subset_faceCount_" + _iStr, _subset.faceCount);
		_i++;
	}

	_ar.Field("IsSkinMesh", m_isSkinMesh);

	if (HasCollisionMesh())
	{
		m_opCollMesh->Archive(_ar);
	}
}

void Engine::Resource::Mesh::Load(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Load, a_fileDir, a_name, "mesh");

	// 頂点数を読み込んでリサイズ
	size_t _vertexCount = 0;
	_ar.Field("VertexCount", _vertexCount);
	m_vertices.resize(_vertexCount);

	// 頂点データの読み込み（Saveと完全に同じキー名にする）
	int _v = 0;
	for (auto& _vert : m_vertices)
	{
		std::string _vStr = "Vert[" + std::to_string(_v) + "].";

		_ar.Field(_vStr + "Pos", _vert.pos);
		_ar.Field(_vStr + "Normal", _vert.normal);
		_ar.Field(_vStr + "UV", _vert.uv);
		_ar.Field(_vStr + "Tangent", _vert.tangent);
		_ar.Field(_vStr + "Color", _vert.color);

		int _i = 0;
		for (auto& _skIdx : _vert.skinIndexList)
		{
			_ar.Field(_vStr + "SkList" + std::to_string(_i), _skIdx); // std::to_stringに修正
			_i++;
		}
		_i = 0;
		for (auto& _skWeit : _vert.skinWeightList)
		{
			_ar.Field(_vStr + "SkWeit" + std::to_string(_i), _skWeit); // std::to_stringに修正
			_i++;
		}
		_v++;
	}

	// 面数を読み込んでリサイズ
	size_t _faceCount = 0;
	_ar.Field("FaceCount", _faceCount);
	m_face.resize(_faceCount);

	// 面データの読み込み
	int _i = 0;
	for (auto& _face : m_face)
	{
		int _j = 0;
		for (auto& _idx : _face.idx)
		{
			_ar.Field("Face" + std::to_string(_i) + "_" + std::to_string(_j), _idx);
			_j++;
		}
		_i++;
	}

	// サブセット数を読み込んでリサイズ
	size_t _subsetCount = 0;
	_ar.Field("SubsetCount", _subsetCount);
	m_subsets.resize(_subsetCount);

	// サブセットの読み込み
	_i = 0;
	for (auto& _subset : m_subsets)
	{
		std::string _iStr = std::to_string(_i);
		_ar.Field("Subset_MaterialNumber_" + _iStr, _subset.materialNumber);
		_ar.Field("Subset_faceStart_" + _iStr, _subset.faceStart);
		_ar.Field("Subset_faceCount_" + _iStr, _subset.faceCount);
		_i++;
	}

	_ar.Field("IsSkinMesh", m_isSkinMesh);

	auto& _collMesh = m_opCollMesh.emplace();
	_collMesh.Archive(_ar);

	CreateFloat(
		m_vertices,
		m_face,
		m_subsets,
		m_isSkinMesh
	);
}

