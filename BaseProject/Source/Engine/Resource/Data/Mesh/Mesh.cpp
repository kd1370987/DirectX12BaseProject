#include "Mesh.h"

#include "../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
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
		if (!m_vertexBuffer.Create(
			(UINT)a_vertices.size(),
			sizeof(MeshVertexFloat),
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

		if (!m_indexBuffer.Create(
			(UINT)_indices.size(),
			sizeof(UINT),
			_indices.data()
		))
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
	m_indexBuffer.CreateSRV();
	m_vertexBuffer.CreateSRV();

	m_BLAS.Create(
		m_vertexBuffer,
		m_indexBuffer
	);

	m_vertexData = a_vertices;

	// 構造体バッファ作成
	auto* _pDevice = D3D12Wrapper::Instance().GetDevice();
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();
	std::vector<RTVertex> _rtVertDataVec = {};
	for (auto& _vert : a_vertices)
	{
		RTVertex _rt = {};
		_rt.uv = _vert.uv;
		_rtVertDataVec.push_back(_rt);
	}
	//m_sVertexBuffer.Create(_pDevice,_pCmdList,a_vertices.size(),a_vertices.data());
	m_sVertexBuffer.Create(_pDevice,_pCmdList, _rtVertDataVec.size(),_rtVertDataVec.data());
	SRVViewInit _viewInit = {};
	_viewInit.pResource = m_sVertexBuffer.GetResource();
	_viewInit.pDesc = m_sVertexBuffer.GetViewDesc();
	auto _handle = DescriptorHeapManager::Instance().AllocateSRVRange({ _viewInit })[0];
	m_sVertexBuffer.SetHandle(_handle);

	m_sIndexBuffer.Create(_pDevice,_pCmdList,m_indexData.size(),m_indexData.data());
	_viewInit = {};
	_viewInit.pResource = m_sIndexBuffer.GetResource();
	_viewInit.pDesc = m_sIndexBuffer.GetViewDesc();
	_handle = DescriptorHeapManager::Instance().AllocateSRVRange({ _viewInit })[0];
	m_sIndexBuffer.SetHandle(_handle);

	// コマンドキューリセット
	D3D12Wrapper::Instance().CommandQueueReset();

	m_sVertexBuffer.Update(_pDevice, _pCmdList);
	m_sIndexBuffer.Update(_pDevice,_pCmdList);

	// コマンドリストをクローズ
	_pCmdList->Close();

	// コマンドキューに積む
	ID3D12CommandList* _ppCommandLists[] = { _pCmdList };
	auto* _cmdQueue = D3D12Wrapper::Instance().GetCommandQueue();
	_cmdQueue->ExecuteCommandLists(std::size(_ppCommandLists), _ppCommandLists);

	// 終了待ち
	D3D12Wrapper::Instance().SignalRenderFence();
	D3D12Wrapper::Instance().WaitRender();

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

