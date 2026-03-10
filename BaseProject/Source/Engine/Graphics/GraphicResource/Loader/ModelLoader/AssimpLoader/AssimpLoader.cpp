#include "AssimpLoader.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

#ifdef _DEBUG
#pragma comment(lib,"assimp-vc143-mtd.lib")
#else
#pragma comment(lib,"assimp-vc143-mt.lib")
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Engine/D3D12//D3DObject/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/VertexBuffer/VertexBuffer.h"

bool AssimpLoader::Load(ImportSettings a_setting)
{
	// ファイルパスが入っていなかったら
	if (a_setting.pFilePath == nullptr)
	{
		return false;
	}

	auto& _meshes = a_setting.meshes;
	auto _isInvrseU = a_setting.isInverseU;
	auto _isInvrseV = a_setting.isInverseV;

	// UTF8に変換
	auto _path = StringUtility::ToUTF8(a_setting.pFilePath);

	// Assimpで読み込み
	Assimp::Importer _importer;
	unsigned int _flg = 0;
	_flg |= aiProcess_Triangulate;					// 三角形化
	_flg |= aiProcess_PreTransformVertices;			// すべてのノードをルートノードに変換
	_flg |= aiProcess_CalcTangentSpace;				// 接空間計算
	_flg |= aiProcess_GenSmoothNormals;				// スムーズシェーディング用法線計算
	_flg |= aiProcess_GenUVCoords;					// UV座標生成
	_flg |= aiProcess_RemoveRedundantMaterials;		// 冗長なマテリアル削除
	_flg |= aiProcess_OptimizeMeshes;				// メッシュ最適化

	auto _scene = _importer.ReadFile(_path, _flg);
	if (_scene == nullptr)
	{
		// 読み込み失敗
		assert(0 && "モデルの読み込みに失敗");
		return false;
	}

	// 読み込んだデータを自分で定義したMesh構造体に変換する
	_meshes.clear();
	_meshes.resize(_scene->mNumMeshes);
	for (size_t _i = 0; _i < _meshes.size(); ++_i)
	{
		// メッシュの読み込み
		const auto _pMesh = _scene->mMeshes[_i];
		LoadMesh(_meshes[_i], _pMesh, _isInvrseU, _isInvrseV);
		// テクスチャ読み込み
		const auto _pMaterial = _scene->mMaterials[_i];
		LoadTexture(a_setting.pFilePath, _meshes[_i], _pMaterial);
	}

	// 読み込み成功
	_scene = nullptr;
	return true;
}

bool AssimpLoader::Load(
	std::string& 			a_filePath,
	std::vector<AssimpMesh>& a_meshes, 
	bool a_isInverseU, bool a_isInverseV)
{
	auto& _meshes = a_meshes;
	auto _isInvrseU = a_isInverseU;
	auto _isInvrseV = a_isInverseV;

	// Assimpで読み込み
	Assimp::Importer _importer;
	unsigned int _flg = 0;
	_flg |= aiProcess_Triangulate;					// 三角形化
	_flg |= aiProcess_PreTransformVertices;			// すべてのノードをルートノードに変換
	_flg |= aiProcess_CalcTangentSpace;				// 接空間計算
	_flg |= aiProcess_GenSmoothNormals;				// スムーズシェーディング用法線計算
	_flg |= aiProcess_GenUVCoords;					// UV座標生成
	_flg |= aiProcess_RemoveRedundantMaterials;		// 冗長なマテリアル削除
	_flg |= aiProcess_OptimizeMeshes;				// メッシュ最適化

	auto _scene = _importer.ReadFile(a_filePath, _flg);
	if (_scene == nullptr)
	{
		// 読み込み失敗
		assert(0 && "モデルの読み込みに失敗");
		return false;
	}

	// 読み込んだデータを自分で定義したMesh構造体に変換する
	_meshes.clear();
	_meshes.resize(_scene->mNumMeshes);
	for (size_t _i = 0; _i < _meshes.size(); ++_i)
	{
		// メッシュの読み込み
		const auto _pMesh = _scene->mMeshes[_i];
		LoadMesh(_meshes[_i], _pMesh, _isInvrseU, _isInvrseV);
		// テクスチャ読み込み
		const auto _pMaterial = _scene->mMaterials[_i];
		//LoadTexture(a_pFilePath, _meshes[_i], _pMaterial);
		LoadTexture(StringUtility::ToWideString(a_filePath).c_str(), _meshes[_i], _pMaterial);
	}

	// 読み込み成功
	_scene = nullptr;
	return true;
}

bool AssimpLoader::Load(std::string a_filePath, AssimpModel& a_model, bool a_isInverseU, bool a_isInverseV)
{
	auto& _model = a_model;
	_model = {};
	auto _isInvrseU = a_isInverseU;
	auto _isInvrseV = a_isInverseV;

	// Assimpで読み込み
	Assimp::Importer _importer;
	unsigned int _flg = 0;
	_flg |= aiProcess_Triangulate;					// 三角形化
	_flg |= aiProcess_PreTransformVertices;			// すべてのノードをルートノードに変換
	_flg |= aiProcess_CalcTangentSpace;				// 接空間計算
	_flg |= aiProcess_GenSmoothNormals;				// スムーズシェーディング用法線計算
	_flg |= aiProcess_GenUVCoords;					// UV座標生成
	_flg |= aiProcess_RemoveRedundantMaterials;		// 冗長なマテリアル削除
	_flg |= aiProcess_OptimizeMeshes;				// メッシュ最適化

	auto _scene = _importer.ReadFile(a_filePath, _flg);
	if (_scene == nullptr)
	{
		// 読み込み失敗
		assert(0 && "モデルの読み込みに失敗");
		return false;
	}

	// 読み込んだデータを自分で定義したMesh構造体に変換する
	_model = {};
	_model.nodes.resize(_scene->mNumMeshes);
	for (size_t _i = 0; _i < _scene->mNumMeshes; ++_i)
	{
		_model.nodes[_i].spMesh = std::make_shared<AssimpMesh>();
		// メッシュの読み込み
		const auto _pMesh = _scene->mMeshes[_i];
		LoadMesh(*_model.nodes[_i].spMesh.get(), _pMesh, _isInvrseU, _isInvrseV);
		// テクスチャ読み込み
		const auto _pMaterial = _scene->mMaterials[_i];
		//LoadTexture(a_pFilePath, _meshes[_i], _pMaterial);
		LoadTexture(StringUtility::ToWideString(a_filePath).c_str(), *_model.nodes[_i].spMesh.get(), _pMaterial);
		_model.nodes[_i].spMesh->materialIndex = static_cast<uint32_t>(_i);
	}

	// 読み込み成功
	_scene = nullptr;
	return true;
}



// メッシュの読み込み
void AssimpLoader::LoadMesh(AssimpMesh& a_dst, const aiMesh* a_src, bool a_isInverseU, bool a_isInverseV)
{
	aiVector3D	_zero3D(0.0f, 0.0f, 0.0f);
	aiColor4D	_zeroColor(0.0f, 0.0f, 0.0f, 0.0f);

	a_dst.vertices.resize(a_src->mNumVertices);

	for (auto _i = 0u; _i < a_src->mNumVertices; ++_i)
	{
		auto _position	= &(a_src->mVertices[_i]);
		auto _normal	= &(a_src->mNormals[_i]);
		auto _uv		= (a_src->HasTextureCoords(0)) ? &(a_src->mTextureCoords[0][_i]) : &_zero3D;
		auto _tangent	= (a_src->HasTangentsAndBitangents()) ? &(a_src->mTangents[_i]) : &_zero3D;
		auto _color		= (a_src->HasVertexColors(0)) ? &(a_src->mColors[0][_i]) : &_zeroColor;
		
		// 反転オプションがある場合は反転
		if (a_isInverseU)
		{
			_uv->x = 1.0f - _uv->x;
		}
		if (a_isInverseV)
		{
			_uv->y = 1.0f - _uv->y;
		}

		// 頂点情報代入
		Engine::Resource::MeshVertexFloat _vertex = {};
		_vertex.pos	= { _position->x, _position->y, _position->z };		// 位置座標
		_vertex.normal		= { _normal->x, _normal->y, _normal->z };			// 法線
		_vertex.uv			= { _uv->x, _uv->y };								// uv座標
		_vertex.tangent		= { _tangent->x, _tangent->y, _tangent->z };		// 接空間
		_vertex.color		= { _color->r, _color->g, _color->b, _color->a };	// 頂点色

		a_dst.vertices[_i] = _vertex;
	}

	// インデックス情報代入
	a_dst.indices.resize(a_src->mNumFaces * 3);
	for (auto _i = 0u; _i < a_src->mNumFaces; ++_i)
	{
		const auto& _face = a_src->mFaces[_i];
		a_dst.indices[_i * 3 + 0] = _face.mIndices[0];
		a_dst.indices[_i * 3 + 1] = _face.mIndices[1];
		a_dst.indices[_i * 3 + 2] = _face.mIndices[2];
	}

	//頂点バッファ作成
	auto _pVB = new VertexBuffer();
	if (!_pVB->Create(a_dst.vertices.size(), sizeof(Engine::Resource::MeshVertexFloat), a_dst.vertices.data()))
	{
		assert(0 && "頂点バッファの生成に失敗\n");
		return;
	}
	a_dst.vertexBuffer = _pVB;

	// インデックスバッファ作成
	auto _pIB = new IndexBuffer();
	if (!_pIB->Create(a_dst.indices.size(), sizeof(uint32_t), a_dst.indices.data()))
	{
		assert(0 && "インデックスバッファの生成に失敗\n");
		return;
	}
	a_dst.indexBuffer = _pIB;
}

// テクスチャ読み込み
void AssimpLoader::LoadTexture(const wchar_t* a_pFilePath, AssimpMesh& a_dst, const aiMaterial* a_src)
{
	aiString _path;
	// ディフューズテクスチャのパスを取得
	if (a_src->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), _path) == AI_SUCCESS)
	{
		// テクスチャパスは相対パスで入っているので、元のファイルパスからディレクトリ部分を取得して結合する
		auto _dir = FileUtility::GetDirectoryPath(a_pFilePath);
		auto _file = std::string(_path.C_Str());
		a_dst.material.diffuseMap = _dir + StringUtility::ToWideString(_file);
		
		auto _texPath = FileUtility::ReplaceFilePathExtension(a_dst.material.diffuseMap, "tga");
		Engine::Resource::ID _texID = 0;
		//auto _mainTex = GraphicResourceManager::Instance().GetTexture(_texID,StringUtility::ToUTF8(_texPath), TextureUse::Albedo);
		
	
		//const TextureResource* _tex = GraphicResourceManager::Instance().NGetTexture(_mainTex);
		const TextureS* _tex = GraphicResourceManager::Instance().NGetTexture(_texID);
		if (!_tex)
		{
			assert(0 && "テクスチャの取得に失敗\n");
			return;
		}
		//a_dst.srvHandle = DescriptorHeapManager::Instance().RegisterSRV(_tex->GetResource());
		//a_dst.srvHandle = DescriptorHeapManager::Instance().RegisterSRV(_tex->cpResource.Get());
	}
	else
	{
		//a_dst.diffuseMap.clear();
		a_dst.material.diffuseMap.clear();
	}
}



