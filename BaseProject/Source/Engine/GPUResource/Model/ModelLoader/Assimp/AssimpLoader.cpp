#include "AssimpLoader.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Framework/Shader/ShaderCommon/SharedStruct.h"

#include "Engine/GPUResource/Model/ModelResource/Mesh/Mesh.h"
#include "Engine/GPUResource/Model/ModelResource/Material/Material.h"
#include "Engine/GPUResource/Model/ModelResource/Animation/Animation.h"
#include "Engine/GPUResource/Model/ModelResource/Node/Node.h"

#ifdef _DEBUG
#pragma comment(lib,"assimp-vc143-mtd.lib")
#else
#pragma comment(lib,"assimp-vc143-mt.lib")
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
		printf("モデルの読み込みに失敗 : %s\n", _importer.GetErrorString());
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
		AssimpVertex _vertex = {};
		_vertex.position	= { _position->x, _position->y, _position->z };		// 位置座標
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
}

// テクスチャ読み込み
void AssimpLoader::LoadTexture(const wchar_t* a_pFilePath, AssimpMesh& a_dst, const aiMaterial* a_src)
{
	aiString _path;
	printf("マテリアル情報読み込み\n");
	// ディフューズテクスチャのパスを取得
	if (a_src->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), _path) == AI_SUCCESS)
	{
		// テクスチャパスは相対パスで入っているので、元のファイルパスからディレクトリ部分を取得して結合する
		auto _dir = FileUtility::GetDirectoryPath(a_pFilePath);
		auto _file = std::string(_path.C_Str());
		a_dst.diffuseMap = _dir + StringUtility::ToWideString(_file);
		printf("テクスチャパス:%ls\n", a_dst.diffuseMap.c_str());
	}
	else
	{
		a_dst.diffuseMap.clear();
		printf("テクスチャなし\n");
	}
}



