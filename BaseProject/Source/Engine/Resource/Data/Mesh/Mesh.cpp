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

void Engine::Resource::Mesh::CreateCollision(
	const std::vector<MeshVertexFloat>& a_vertices,
	const std::vector<MeshFace>& a_face
)
{
	//------------------------------
	// 座標のみの配列
	//------------------------------
	std::vector<DirectX::XMFLOAT3> _posVec = {};
	_posVec.resize(a_vertices.size());			// サイズ確保
	for (size_t _i = 0; _i < a_vertices.size(); ++_i)
	{
		_posVec[_i] = a_vertices[_i].pos;
	}

	std::vector<DirectX::XMFLOAT3> _idxVec;
	_idxVec.resize(a_face.size());
	for(int _i = 0; _i < a_face.size(); ++_i)
	{
		_idxVec[_i].x = a_face[_i].idx[0];
		_idxVec[_i].y = a_face[_i].idx[1];
		_idxVec[_i].z = a_face[_i].idx[2];
	}
	
	// 生成
	m_opCollisionMesh = Engine::Collision::CreateMesh(
		_posVec,
		_idxVec
	);
}


