#include "AssimpSerialize.h"

#include "../../../Resource/Model/Model.h"
#include "../../../Loader/ModelLoader/AssimpLoader/AssimpLoader.h"

namespace
{
	void CreateNodes(Engine::Resource::Model& a_dst, const std::shared_ptr<AssimpModel>& a_src)
	{
		AssimpModel* _assimpModel = a_src.get();

		//=================================================
		// ノード作成
		//=================================================
		a_dst.originalNodes.resize(_assimpModel->nodes.size());		// ノード配列確保
		for (UINT _i = 0; _i < _assimpModel->nodes.size(); ++_i)
		{
			// ノード情報
			const AssimpNode& _srcNode = _assimpModel->nodes[_i];	// 元データ
			Engine::Resource::Node& _dstNode = a_dst.originalNodes[_i];						// 出力先

			// 基本情報コピー
			if (_srcNode.spMesh)
			{
				// メッシュ作成
				auto _spMesh = std::make_shared<Engine::Resource::Mesh>();

				// 頂点配列作成
				std::vector<Engine::Resource::MeshVertexFloat> _vertices = {};
				_vertices.resize(_srcNode.spMesh->vertices.size());
				for (size_t _j = 0; _j < _srcNode.spMesh->vertices.size(); ++_j)
				{
					Engine::Resource::MeshVertexFloat _dstVertex = _srcNode.spMesh->vertices[_j];
					_vertices[_j] = _dstVertex;
				}

				// フェイス情報作成
				std::vector<Engine::Resource::MeshFace> _faces;
				UINT _faceCount = (UINT)_srcNode.spMesh->indices.size() / 3;
				_faces.resize(_faceCount);
				for (UINT _fIdx = 0; _fIdx < _faceCount; ++_fIdx)
				{
					_faces[_fIdx].idx[0] = _srcNode.spMesh->indices[_fIdx * 3 + 0];
					_faces[_fIdx].idx[1] = _srcNode.spMesh->indices[_fIdx * 3 + 1];
					_faces[_fIdx].idx[2] = _srcNode.spMesh->indices[_fIdx * 3 + 2];
				}

				// サブセット生成
				Engine::Resource::MeshSubset _subset = {};
				_subset.faceStart = 0;
				_subset.faceCount = (UINT)_faces.size() * 3;
				_subset.materialNumber = _srcNode.spMesh->materialIndex;
				std::vector<Engine::Resource::MeshSubset> _subsets = {};
				_subsets.push_back(_subset);

				// メッシュデータコピー
				if(_spMesh)
				{
					_spMesh->CreateFloat(
						_vertices,
						_faces,
						_subsets,
						_srcNode.spMesh->isSkinMesh
					);
				}

				// メッシュノードリストにインデックス登録
				a_dst.meshNodeIndices.push_back(_i);

				_dstNode.meshIndices.push_back(_i);		// ノードのメッシュインデックスリストに登録
				a_dst.spMeshVec.push_back(_spMesh);
			}

			// ノード情報セット
			_dstNode.name = _srcNode.name;										// ノード名

			_dstNode.localTransform = DirectX::XMFLOAT4X4
			(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			);	// ローカル行列
			_dstNode.worldTransform = DirectX::XMFLOAT4X4
			(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			);	// ワールド行列

			_dstNode.boneInverseWorldMatrix = {};		// ボーンのオフセット行列

			_dstNode.parent = -1;									// 親インデックス
			_dstNode.children = {};								// 子供リスト

			_dstNode.boneIndex = -1;						// ボーンインデックス
			_dstNode.isSkinMesh = _srcNode.isSkinMesh;					// スキンメッシュ持ちかどうか

			// 当たり判定用ノードの検索
			if (_dstNode.name.find("COL") != std::string::npos)
			{
				// 判定用ノードに割り当て
				a_dst.collisionMeshNodeIndices.push_back(_i);
			}
			else
			{
				// 描画用ノードに割り当て
				a_dst.drawMeshNodeIndices.push_back(_i);
			}
		}

		//=================================================
		// ルートノード・ボーンノードリスト作成
		//=================================================
		for (UINT _nodeIdx = 0; _nodeIdx < _assimpModel->nodes.size(); ++_nodeIdx)
		{
			// ルートノードの登録
			if (_assimpModel->nodes[_nodeIdx].parent == -1)
			{
				a_dst.rootNodeIndices.push_back(_nodeIdx);
			}
			// ボーンノードの登録
			int _boneIdx = a_src->nodes[_nodeIdx].boneIndex;		// ボーンインデックス取得
			if (_boneIdx >= 0)
			{
				// ボーンノードリストのサイズ確保
				if (_boneIdx >= (int)a_dst.boneNodeIndices.size())
				{
					a_dst.boneNodeIndices.resize(_boneIdx + 1);
				}

				// 登録
				a_dst.boneNodeIndices[_boneIdx] = _nodeIdx;
			}
		}

		// 当たり判定用ノードがなければ、a_dst.drawMeshNodeと同じ割り当てを行う
		if (a_dst.collisionMeshNodeIndices.size() == 0)
		{
			a_dst.collisionMeshNodeIndices = a_dst.drawMeshNodeIndices;
		}
	}
	void CreateMaterials(Engine::Resource::Model& a_dst, const std::shared_ptr<AssimpModel>& a_src, const std::string& a_fileDir)
	{
		//=================================================
	// マテリアル作成
	//=================================================

		// マテリアル配列を受け取れるサイズのメモリを確保
		a_dst.materials.resize(a_src->nodes.size());
		for (UINT _i = 0; _i < a_dst.materials.size(); ++_i)
		{
			// マテリアル情報
			const AssimpMaterial& _srcMaterial = a_src->nodes[_i].spMesh->material;	// 元データ
			Engine::Resource::Material& _dstMaterial = a_dst.materials[_i];							// 出力先

			// マテリアル情報コピー
			_dstMaterial.name = "";									// マテリアル名
			std::string _baseName = FileUtility::GetFileName(StringUtility::ToUTF8(FileUtility::ReplaceFilePathExtension(_srcMaterial.diffuseMap, "tga")));
			// テクスチャセット
			_dstMaterial.SetTexture2D(
				a_fileDir,
				_baseName,
				"",
				"",
				""
			);

			// 材質データ
			//_dstMaterial.baseColor = { 1.0f,1.0f,1.0f,1.0f };				// 基本色
			_dstMaterial.metallic = 0.0f;					// 金属製
			_dstMaterial.roughness = 1.0f;				// 粗さ
			_dstMaterial.emissive = { 1.0f,1.0f,1.0f };					// エミッシブ
		}
	}
	void CreateAnimations(Engine::Resource::Model& a_dst, const std::shared_ptr<AssimpModel>& a_src)
	{

	}
}

void Serialize::Assimp(Engine::Resource::Model& a_dst, std::shared_ptr<AssimpModel> a_src, const std::string& a_fileDir)
{
	CreateNodes(a_dst,a_src);

	CreateMaterials(a_dst,a_src,a_fileDir);

	CreateAnimations(a_dst,a_src);
}
