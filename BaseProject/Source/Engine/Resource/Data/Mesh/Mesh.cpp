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
	m_subsets.clear();
	m_positions.clear();
	m_faces.clear();
	auto _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
	//------------------------------
	// サブセット情報
	//------------------------------
	m_subsets = a_subsets;

	//------------------------------
	// 頂点情報があるのなら
	//------------------------------
	if (a_vertices.size() > 0)
	{
		//------------------------------
		// 頂点バッファ作成
		//------------------------------
		//if (!m_vertexBuffer.Create(
		//	(UINT)a_vertices.size(),
		//	sizeof(MeshVertexFloat),
		//	a_vertices.data()
		//))
		
		if (!m_vertexBuffer.CreateAndUpload(
			_pDevice,
			(UINT)a_vertices.size(),
			a_vertices.data()
		))
		{
			assert(0 && "頂点バッファの生成に失敗");
			return false;
		}

		//------------------------------
		// 座標のみの配列
		//------------------------------
		m_positions.resize(a_vertices.size());			// サイズ確保
		for (size_t _i = 0; _i < a_vertices.size(); ++_i)
		{
			m_positions[_i] = a_vertices[_i].pos;
		}

		//------------------------------
		// 境界データ作成
		//------------------------------
		DirectX::BoundingBox::CreateFromPoints(					// AA境界データ作成
			m_aabb, m_positions.size(), &m_positions[0], sizeof(DirectX::XMFLOAT3));
		DirectX::BoundingSphere::CreateFromPoints(				// 境界球データ作成
			m_bSphere, m_positions.size(), &m_positions[0], sizeof(DirectX::XMFLOAT3));
	}

	//------------------------------
	// インデックス情報があるのなら
	//------------------------------
	if (a_face.size() > 0)
	{
		m_faces = a_face;		// 面情報コピー

		//------------------------------
		// インデックスバッファ作成
		//------------------------------
		std::vector<UINT> _indices;		// インデックス配列作成
		//_indices.resize(a_face.size() * 3);		// サイズ確保
		for (auto& _f : a_face)
		{
			_indices.push_back(_f.idx[0]);
			_indices.push_back(_f.idx[1]);
			_indices.push_back(_f.idx[2]);
		}

		D3D12::IndexBufferDesc _desc = {};
		_desc.count = _indices.size();
		_desc.pData = _indices.data();
		_desc.format = DXGI_FORMAT_R32_UINT;
		if (!m_indexBuffer.Create(_pDevice,_desc))
		{
			assert(0 && "インデックスバッファの生成に失敗");
			return false;
		}

		m_indexData = _indices;
	}

	//------------------------------
	// スキンメッシュ持ちかどうか
	//------------------------------
	m_isSkinMesh = a_isSkinMesh;

	
	//------------------------------
	// BLAS作成
	//------------------------------
	m_indexBuffer.CreateSRV(_pDevice);
	m_vertexBuffer.CreateSRV(_pDevice);

	CreateBLAS();

	m_vertexData = a_vertices;

	// 構造体バッファ作成
	//auto* _pDevice = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
	//auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCommandList();
	auto* _pCmdList = Engine::Resource::ResourceManager::Instance().GetCmdList()->NGet();
	std::vector<RTVertex> _rtVertDataVec = {};
	for (auto& _vert : a_vertices)
	{
		RTVertex _rt = {};
		_rt = _vert;
		_rtVertDataVec.push_back(_rt);
	}
	// 頂点バッファー側SRV作成
	m_sVertexBuffer.Create(_pDevice,_pCmdList, _rtVertDataVec.size(),_rtVertDataVec.data());
	auto _handle = D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::SRV>(
		_pDevice,
		m_sVertexBuffer.GetResource(),
		m_sVertexBuffer.GetViewDesc()
	);
	m_sVertexBuffer.SetHandle(_handle);

	// インデックスバッファー側SRV作成
	m_sIndexBuffer.Create(_pDevice,_pCmdList,m_indexData.size(),m_indexData.data());
	_handle = D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::SRV>(
		Engine::D3D12::D3D12Wrapper::Instance().GetDevice(),
		m_sIndexBuffer.GetResource(),
		m_sIndexBuffer.GetViewDesc()
	);
	m_sIndexBuffer.SetHandle(_handle);

	// コマンドキューリセット
	Engine::D3D12::D3D12Wrapper::Instance().CommandQueueReset();
	//ResourceManager::Instance().CmdQueueReset();

	m_sVertexBuffer.Update(_pDevice, _pCmdList);
	m_sIndexBuffer.Update(_pDevice,_pCmdList);

	// コマンドリストをクローズ
	//_pCmdList->Close();
	Engine::Resource::ResourceManager::Instance().GetCmdList()->NGet()->Close();

	// コマンドキューに積む
	ID3D12CommandList* _ppCommandLists[] = { Engine::Resource::ResourceManager::Instance().GetCmdList()->NGet() };
	//ID3D12CommandList* _ppCommandLists[] = { _pCmdList };
	auto* _cmdQueue = Engine::D3D12::D3D12Wrapper::Instance().GetCopyCommandQueue();
	//auto* _cmdQueue = Engine::D3D12::D3D12Wrapper::Instance().GetCommandQueue();
	_cmdQueue->ExecuteCommandLists(std::size(_ppCommandLists), _ppCommandLists);

	//// 終了待ち
	Engine::D3D12::D3D12Wrapper::Instance().SignalRenderFence();
	Engine::D3D12::D3D12Wrapper::Instance().WaitRender();
	//ResourceManager::Instance().SignalFence(_cmdQueue);
	//ResourceManager::Instance().WaitRender();

	return true;
}

void Engine::Resource::Mesh::CreateCollision()
{
	std::vector<DirectX::XMFLOAT3> _vec;
	_vec.resize(m_faces.size());
	for(int _i = 0; _i < m_faces.size(); ++_i)
	{
		_vec[_i].x = m_faces[_i].idx[0];
		_vec[_i].y = m_faces[_i].idx[1];
		_vec[_i].z = m_faces[_i].idx[2];
	}
	
	// 生成
	m_opCollisionMesh = Engine::Collision::CreateMesh(
		m_positions,
		_vec
	);
}

void Engine::Resource::Mesh::Release()
{
	m_subsets.clear();
	m_positions.clear();
	m_faces.clear();
}

void Engine::Resource::Mesh::CreateBLAS()
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> _descVec;
	// レイトレーシング用データ作成
	for (auto& _subset : m_subsets)
	{
		// ジオメトリ記述作成
		D3D12_RAYTRACING_GEOMETRY_DESC _desc = {};
		_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		// 頂点バッファ
		_desc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer.GetGPUVirtualAddress();
		_desc.Triangles.VertexBuffer.StrideInBytes = m_vertexBuffer.GetStrideSize();
		_desc.Triangles.VertexCount = m_vertexBuffer.GetElementNum();
		_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

		// インデックスバッファ
		_desc.Triangles.IndexBuffer = m_indexBuffer.GetGPUVirtualAddress() + sizeof(UINT) * _subset.faceStart * 3;
		_desc.Triangles.IndexCount = _subset.faceCount * 3;
		_desc.Triangles.IndexFormat = m_indexBuffer.GetView().Format;

		_descVec.push_back(_desc);
	}

	// BLAS作成
	m_BLAS.Create(_descVec);
}

