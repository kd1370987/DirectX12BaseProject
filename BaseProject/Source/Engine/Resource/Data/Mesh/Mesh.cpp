#include "Mesh.h"

#include "../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../MainEngine.h"
#include "../../../Graphics/GraphicEngine.h"

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


	std::vector<uint32_t> _indices;		// インデックス配列作成
	for (auto& _f : a_face)
	{
		_indices.push_back(_f.idx[0]);
		_indices.push_back(_f.idx[1]);
		_indices.push_back(_f.idx[2]);
	}
	auto _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

	// メタデータ作成
	CreateMeshMetaData(a_vertices,a_subsets,a_isSkinMesh);

	// ラスタライザーデータ作成
	CreateRasterData(_pDevice, a_vertices, a_face, DXGI_FORMAT_R32_UINT);


	m_opRtData.emplace();

	D3D12::D3D12Wrapper::Instance().ExecuteAsyncCompute(
		// コマンドを積む処理
		[this, _pDevice, a_subsets, a_vertices, a_face](D3D12::GraphicsCommandList* a_pCmdList)
		{
			// レイ用データ（BLAS等）の作成とコピー命令を積む
			CreateRtData(
				_pDevice,
				a_pCmdList,
				a_subsets,
				m_opRasterData->vertexBuffer,
				DXGI_FORMAT_R32G32B32_FLOAT,
				m_opRasterData->indexBuffer,
				a_vertices,
				a_face
			);
		},
		// 完了時のコールバック
		[]()
		{
			ENGINE_LOG("メッシュの非同期セットアップが完了");
		}
	);
	D3D12::D3D12Wrapper::Instance().ExecuteAsyncCopy(
		// コマンドを積む処理
		[this, _pDevice, a_subsets, a_vertices, a_face](D3D12::GraphicsCommandList* a_pCmdList)
		{
			// メッシュシェーダー用データ作成
			std::vector<uint32_t> _indices = {};
			for (auto& _f : a_face)
			{
				_indices.push_back(_f.idx[0]);
				_indices.push_back(_f.idx[1]);
				_indices.push_back(_f.idx[2]);
			}
			CreateMeshShaderData(a_pCmdList, a_vertices, _indices, a_face);
		},
		// 完了時のコールバック
		[this, _indices]()
		{
			ENGINE_LOG("メッシュの非同期セットアップが完了");
		}
	);

	// ハンドルの登録
	auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
	if (!_pGE) return false;

	// 構造体バッファ作成
	std::vector<RTVertex> _rtVertDataVec = {};
	for (auto& _vert : a_vertices)
	{
		RTVertex _rt = {};
		_rt = _vert;
		_rtVertDataVec.push_back(_rt);
	}
	auto iii = sizeof(MeshVertexFloat);
	m_opRtData->vertexHandle = _pGE->AllocateMeshVertex(a_vertices);
	m_opRtData->indexHandle = _pGE->AllocateMeshIndex(_indices);

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

void Engine::Resource::Mesh::CreateRasterData(D3D12::Device* a_pDevice, const std::vector<MeshVertexFloat>& a_vertices, const std::vector<MeshFace>& a_face, DXGI_FORMAT a_indexFormat)
{
	auto& _raster = m_opRasterData.emplace();
	_raster.Create(a_pDevice, a_vertices, a_face, a_indexFormat);
}

void Engine::Resource::Mesh::CreateRtData(
	D3D12::Device* a_pDevice,
	D3D12::GraphicsCommandList* a_pCmdList, 
	const std::vector<MeshSubset>& a_subset,
	const D3D12::DynamicVertexBuffer<MeshVertexFloat>& a_vertexBuffer,
	DXGI_FORMAT a_vertexFarstFormat, 
	const D3D12::DynamicIndexBuffer& a_indexBuffer,
	const std::vector<MeshVertexFloat>& a_vertices, 
	const std::vector<MeshFace>& a_face
)
{
	m_opRtData->Create(a_pDevice,a_pCmdList,a_subset,a_vertexBuffer,a_vertexFarstFormat,a_indexBuffer,a_vertices,a_face);
}

void Engine::Resource::Mesh::CreateCollisionMesh(const std::vector<DirectX::XMFLOAT3>& a_vertices, const std::vector<UINT>& a_indices)
{
	auto& _collMesh = m_opCollMesh.emplace();
	_collMesh.Create(a_vertices,a_indices);
}

void Engine::Resource::Mesh::CreateMeshShaderData(
	D3D12::GraphicsCommandList* a_pCmdList,
	const std::vector<MeshVertexFloat>& a_vertices, 
	const std::vector<uint32_t>& a_indices,
	const std::vector<MeshFace>& a_face
)
{
	// メッシュレット生成用パラメーター
	const size_t MAX_VERTS = 64;			// 最大頂点数
	const size_t MAX_PRIMS = 126;			// 最大プリミティブ数
	uint32_t _currentMeshletOffset = 0;
	uint32_t _currentVertexOffset = 0;
	uint32_t _currentPrimitiveOffset = 0;

	// 空で生成
	m_opMeshShaderData.emplace();

	// 全サブセットのデータをまとめるマスター配列
	std::vector<Engine::Resource::Meshlet> _masterMeshlets;
	std::vector<uint32_t> _masterUVI;
	std::vector<DirectX::MeshletTriangle> _masterPrimitives;
	std::vector<SubsetMeshletData> _subsetMeshletData;

	// 位置情報だけの配列を作成
	std::vector<DirectX::XMFLOAT3> _positions;
	_positions.reserve(a_vertices.size());
	for (const auto& v : a_vertices)
	{
		_positions.push_back(v.pos);
	}

	for (const auto& _subset : m_meshMetaData.subsets)
	{
		std::vector<uint32_t> _subsetIndices;
		// このサブセットが持つ面だけを抽出
		for (uint32_t i = 0; i < _subset.faceCount; ++i)
		{
			auto& _f = a_face[_subset.faceStart + i];
			_subsetIndices.push_back(_f.idx[0]);
			_subsetIndices.push_back(_f.idx[1]);
			_subsetIndices.push_back(_f.idx[2]);
		}

		// サブセット単体で ComputeMeshlets を実行
		std::vector<DirectX::Meshlet> _dxMeshlets;
		std::vector<uint8_t> _uniqueVertexIB;
		std::vector<DirectX::MeshletTriangle> _primitiveIndices;

		// メッシュレット生成
		auto _hr = DirectX::ComputeMeshlets(
			_subsetIndices.data(), _subsetIndices.size() / 3,
			_positions.data(), _positions.size(),
			nullptr, _dxMeshlets, _uniqueVertexIB, _primitiveIndices,
			MAX_VERTS, MAX_PRIMS
		);
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(false, "Meshletの生成に失敗");
			return;
		}

		// 前回のサブセットまでのオフセットを足してマスター配列に追加
		for (const auto& _dxm : _dxMeshlets)
		{
			Engine::Resource::Meshlet _m = {};
			_m.vertexCount = _dxm.VertCount;
			_m.vertexOffset = _dxm.VertOffset + _currentVertexOffset;
			_m.primitiveCount = _dxm.PrimCount;
			_m.primitiveOffset = _dxm.PrimOffset + _currentPrimitiveOffset;
			_masterMeshlets.push_back(_m);
		}

		// ---------------------------------------------------------
		// uniqueVertexIndices を uint32_t に統一して変換
		// ---------------------------------------------------------
		std::vector<uint32_t> _convertedUVI;

		// DirectXMeshは頂点数が65535以下なら16bit、それより大きければ32bitでUVIを出力する
		if (a_vertices.size() <= 65535)
		{
			// 16bitとして出力されているので32bitに変換
			const uint16_t* _p16 = reinterpret_cast<const uint16_t*>(_uniqueVertexIB.data());
			size_t _indexCount = _uniqueVertexIB.size() / sizeof(uint16_t);

			_convertedUVI.resize(_indexCount);
			for (size_t i = 0; i < _indexCount; ++i)
			{
				_convertedUVI[i] = static_cast<uint32_t>(_p16[i]);
			}
		}
		else
		{
			// 最初から32bitで出力されているのでそのままコピー
			const uint32_t* _p32 = reinterpret_cast<const uint32_t*>(_uniqueVertexIB.data());
			size_t _indexCount = _uniqueVertexIB.size() / sizeof(uint32_t);

			_convertedUVI.assign(_p32, _p32 + _indexCount);
		}

		// 変換した配列をマスター配列の後ろに結合 (Append)
		_masterUVI.insert(_masterUVI.end(), _convertedUVI.begin(), _convertedUVI.end());
		_masterPrimitives.insert(_masterPrimitives.end(), _primitiveIndices.begin(), _primitiveIndices.end());

		// 描画システムに渡すためのサブセットごとのカタログ情報を記録
		SubsetMeshletData _smd = {};
		_smd.meshletOffset = _currentMeshletOffset;
		_smd.meshletCount = static_cast<uint32_t>(_dxMeshlets.size());
		_subsetMeshletData.push_back(_smd);

		// 次のサブセットのためにオフセットを進める
		_currentMeshletOffset += static_cast<uint32_t>(_dxMeshlets.size());
		_currentVertexOffset += static_cast<uint32_t>(_convertedUVI.size());
		_currentPrimitiveOffset += static_cast<uint32_t>(_primitiveIndices.size());
	}

	// すべてのループが終わったら、構造体にマスター配列をムーブ
	m_opMeshShaderData.value().meshlets = std::move(_masterMeshlets);
	m_opMeshShaderData.value().uniqueVertexIndices = std::move(_masterUVI);
	m_opMeshShaderData.value().primitiveIndices = std::move(_masterPrimitives);
	m_opMeshShaderData.value().subsetMeshlets = std::move(_subsetMeshletData);

	// メガバッファに登録
	ENGINE_LOG("メッシュの非同期セットアップ完了");
	auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
	ENGINE_ERRLOG(_pGE, "メッシュ読み込み時にグラフィックスエンジンがありません");

	//auto _handle = _pGE->AllocateAndUpload(a_pCmdList, *this);
	//m_opMeshShaderData.value().meshHandle = _handle;
}

void Engine::Resource::Mesh::Release()
{
	auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
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

void Engine::Resource::Mesh::Load(const std::string& a_filePath)
{
	auto _fileDir = FileUtility::GetDirFromPath(a_filePath);
	auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
	Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "mesh");

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

