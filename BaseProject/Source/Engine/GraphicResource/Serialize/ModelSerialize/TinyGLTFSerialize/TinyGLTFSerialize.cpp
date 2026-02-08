#include "TinyGLTFSerialize.h"

#include "../../../Resource/Model/Model.h"
#include "../../../Loader/ModelLoader/TinyGLTFLoader/TinyGLTFLoader.h"

namespace
{
	void CreateNodes(Model& a_dst, const std::shared_ptr<GLTFModel>& a_src)
	{
		//=================================================
		// ノード作成
		//=================================================
		a_dst.originalNodes.resize(a_src->nodes.size());		// ノード配列確保
		for (UINT _i = 0; _i < a_src->nodes.size(); ++_i)
		{
			// ノード情報
			const GLTFNode& _srcNode = a_src->nodes[_i];	// 元データ
			Node& _dstNode = a_dst.originalNodes[_i];						// 出力先

			// 基本情報コピー
			if (_srcNode.isMesh)
			{
				// メッシュ作成
				_dstNode.spMesh = std::make_shared<Mesh>();

				// メッシュデータコピー
				if (_dstNode.spMesh)
				{
					// 頂点配列作成
					std::vector<MeshVertexFloat> _vertices = {};
					_vertices.resize(_srcNode.nodeMesh.vertices.size());
					for (size_t _j = 0; _j < _srcNode.nodeMesh.vertices.size(); ++_j)
					{
						MeshVertexFloat _dstVertex = {};

						unsigned int _srcColor = _srcNode.nodeMesh.vertices[_j].color;
						float r = ((float)((_srcColor >> 24) & 0xFF)) / 255.0f;
						float g = ((float)((_srcColor >> 16) & 0xFF)) / 255.0f;
						float b = ((float)((_srcColor >> 8) & 0xFF)) / 255.0f;
						float a = ((float)((_srcColor >> 0) & 0xFF)) / 255.0f;
						_dstVertex.color = DirectX::XMFLOAT4(r, g, b, a);

						_dstVertex.normal = _srcNode.nodeMesh.vertices[_j].normal;
						_dstVertex.pos = _srcNode.nodeMesh.vertices[_j].pos;
						_dstVertex.tangent = _srcNode.nodeMesh.vertices[_j].tangent;
						_dstVertex.uv = _srcNode.nodeMesh.vertices[_j].uv;

						_vertices[_j] = _dstVertex;
					}

					// メッシュ作成
					_dstNode.spMesh->CreateFloat(
						_vertices,
						_srcNode.nodeMesh.faces,
						_srcNode.nodeMesh.subsets,
						_srcNode.nodeMesh.isSkinMesh
					);
				}

				// メッシュノードリストにインデックス登録
				a_dst.meshNodeIndices.push_back(_i);
			}

			// ノード情報セット
			_dstNode.name = _srcNode.name;										// ノード名
			_dstNode.localTransform = _srcNode.localTransform;					// ローカル行列
			_dstNode.worldTransform = _srcNode.worldTransform;					// ワールド行列
			_dstNode.boneInverseWorldMatrix = _srcNode.inverseBindMatrix;		// ボーンのオフセット行列

			_dstNode.parent = _srcNode.parent;									// 親インデックス
			_dstNode.children = _srcNode.children;								// 子供リスト

			_dstNode.boneIndex = _srcNode.boneNodeIndex;						// ボーンインデックス
			_dstNode.isSkinMesh = _srcNode.nodeMesh.isSkinMesh;					// スキンメッシュ持ちかどうか

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
		for (UINT _nodeIdx = 0; _nodeIdx < a_src->nodes.size(); ++_nodeIdx)
		{
			// ルートノードの登録
			if (a_src->nodes[_nodeIdx].parent == -1)
			{
				a_dst.rootNodeIndices.push_back(_nodeIdx);
			}
			// ボーンノードの登録
			int _boneIdx = a_src->nodes[_nodeIdx].boneNodeIndex;		// ボーンインデックス取得
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
	void CreateMaterials(Model& a_dst, const std::shared_ptr<GLTFModel>& a_src, const std::string& a_fileDir)
	{
		//=================================================
		// マテリアル作成
		//=================================================

		// マテリアル配列を受け取れるサイズのメモリを確保
		a_dst.materials.resize(a_src->materials.size());

		for (UINT _i = 0; _i < a_dst.materials.size(); ++_i)
		{
			// マテリアル情報
			const GLTFMaterial& _srcMaterial = a_src->materials[_i];	// 元データ
			Material& _dstMaterial = a_dst.materials[_i];							// 出力先

			// マテリアル情報コピー
			_dstMaterial.name = _srcMaterial.name;									// マテリアル名

			// テクスチャセット
			_dstMaterial.SetTexture2D(
				a_fileDir,
				_srcMaterial.baseColorTexName,
				_srcMaterial.metallicRoughnessTexName,
				_srcMaterial.emissiveTexName,
				_srcMaterial.normalTexName
			);

			// 材質データ
			_dstMaterial.baseColor = _srcMaterial.baseColorFactor;				// 基本色
			_dstMaterial.metallic = _srcMaterial.metallicFactor;					// 金属製
			_dstMaterial.roughness = _srcMaterial.roughnessFactor;				// 粗さ
			_dstMaterial.emissive = _srcMaterial.emissiveFactor;					// エミッシブ
		}
	}
	void CreateAnimations(Model& a_dst, const std::shared_ptr<GLTFModel>& a_src)
	{
		//=================================================
		// アニメーション作成
		//=================================================
		a_dst.spAnimations.resize(a_src->animations.size());		// アニメーション配列確保

		for (UINT _i = 0; _i < a_dst.spAnimations.size(); ++_i)
		{
			auto _srcAnimation = a_src->animations[_i];	// 元データ

			a_dst.spAnimations[_i] = std::make_shared<AnimationData>();
			AnimationData& _dstAnimation = *a_dst.spAnimations[_i];							// 出力先

			// アニメーション情報コピー
			_dstAnimation.name = _srcAnimation->name;									// 名前
			_dstAnimation.maxLength = _srcAnimation->maxLength;							// アニメーションの長さ

			_dstAnimation.nodes.resize(_srcAnimation->spAnimationNodes.size());			// ノード配列確保
			for (UINT _nIdx = 0; _nIdx < _dstAnimation.nodes.size(); ++_nIdx)
			{
				auto _srcNode = _srcAnimation->spAnimationNodes[_nIdx];	// 元データ
				AnimationNode& _dstAnimaNode = _dstAnimation.nodes[_nIdx];						// 出力先
				// ノード情報コピー
				_dstAnimaNode.nodeOffset = _srcNode->nodeOffset;				// 対象ノードのオフセット
				_dstAnimaNode.translations = _srcNode->translations;			// 座標キーリスト
				_dstAnimaNode.rotations = _srcNode->rotations;					// 回転キーリスト
				_dstAnimaNode.scales = _srcNode->scales;						// 拡縮キーリスト
			}

		}
	}
}

void Serialize::TinyGLTF(Model& a_dst, std::shared_ptr<GLTFModel> a_src, const std::string& a_fileDir)
{
	// ノード作成
	CreateNodes(a_dst, a_src);

	// マテリアル作成
	CreateMaterials(a_dst, a_src, a_fileDir);

	// アニメーション
	CreateAnimations(a_dst, a_src);
}