#include "ModelConverter.h"

namespace Engine::Resource::Converter
{
	namespace
	{
		void ConvertNodes(ModelData& a_destModel, const GLTF::ModelData& a_rawModel)
		{
			//=================================================
			// ノード作成
			//=================================================
			a_destModel.originalNodes.resize(a_rawModel.nodes.size());		// ノード配列確保
			for (UINT _i = 0; _i < static_cast<UINT>(a_rawModel.nodes.size()); ++_i)
			{
				// ノード情報
				const Engine::Resource::GLTF::Node& _srcNode = a_rawModel.nodes[_i];	// 入力元
				Engine::Resource::Node& _dstNode = a_destModel.originalNodes[_i];			// 出力先

				// 基本情報コピー
				if (_srcNode.isMesh)
				{
					// メッシュ作成
					Engine::Resource::Mesh _mesh = {};

					// 頂点配列作成
					std::vector<Engine::Resource::MeshVertexFloat> _vertices = {};
					_vertices.resize(_srcNode.nodeMesh.vertices.size());
					for (size_t _j = 0; _j < _srcNode.nodeMesh.vertices.size(); ++_j)
					{
						Engine::Resource::MeshVertexFloat _dstVertex = {};

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

						_dstVertex.skinIndexList = _srcNode.nodeMesh.vertices[_j].skinIndexList;
						_dstVertex.skinWeightList = _srcNode.nodeMesh.vertices[_j].skinWeightList;

						_vertices[_j] = _dstVertex;
					}

					// メッシュ作成
					_mesh.CreateFloat(
						_vertices,
						_srcNode.nodeMesh.faces,
						_srcNode.nodeMesh.subsets,
						_srcNode.nodeMesh.isSkinMesh
					);


					// メッシュノードリストにインデックス登録
					a_destModel.meshNodeIndices.push_back(_i);

					_dstNode.meshIndices.push_back(static_cast<int>(a_destModel.MeshVec.size()));
					a_destModel.MeshVec.push_back(std::move(_mesh));
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
					a_destModel.collisionMeshNodeIndices.push_back(_i);
				}
				else
				{
					// 描画用ノードに割り当て
					a_destModel.drawMeshNodeIndices.push_back(_i);
				}
			}

			//=================================================
			// ルートノード・ボーンノードリスト作成
			//=================================================
			for (UINT _nodeIdx = 0; _nodeIdx < a_rawModel.nodes.size(); ++_nodeIdx)
			{
				// ルートノードの登録
				if (a_rawModel.nodes[_nodeIdx].parent == -1)
				{
					a_destModel.rootNodeIndices.push_back(_nodeIdx);
				}
				// ボーンノードの登録
				int _boneIdx = a_rawModel.nodes[_nodeIdx].boneNodeIndex;		// ボーンインデックス取得
				if (_boneIdx >= 0)
				{
					// ボーンノードリストのサイズ確保
					if (_boneIdx >= (int)a_destModel.boneNodeIndices.size())
					{
						a_destModel.boneNodeIndices.resize(_boneIdx + 1);
					}

					// 登録
					a_destModel.boneNodeIndices[_boneIdx] = _nodeIdx;
				}
			}

			// 当たり判定用ノードがなければ、a_dst.drawMeshNodeと同じ割り当てを行う
			if (a_destModel.collisionMeshNodeIndices.size() == 0)
			{
				a_destModel.collisionMeshNodeIndices = a_destModel.drawMeshNodeIndices;

				for (auto& _idx : a_destModel.collisionMeshNodeIndices)
				{
					for (auto& _meshIdx : a_destModel.originalNodes[_idx].meshIndices)
					{
						const auto& _node = a_rawModel.nodes[_idx];

						// 頂点配列作成
						std::vector<DirectX::XMFLOAT3> _collisionVertices;
						_collisionVertices.resize(_node.nodeMesh.vertices.size());

						for (size_t _j = 0; _j < _node.nodeMesh.vertices.size(); ++_j)
						{
							_collisionVertices[_j] = _node.nodeMesh.vertices[_j].pos;
						}
						std::vector<UINT> _indices = {};
						for (auto& _f : _node.nodeMesh.faces)
						{
							_indices.push_back(_f.idx[0]);
							_indices.push_back(_f.idx[1]);
							_indices.push_back(_f.idx[2]);
						}
						a_destModel.MeshVec[_meshIdx].CreateCollisionMesh(
							_collisionVertices,
							_indices
						);
					}
				}
			}
		}

		/// <summary>
		/// マテリアルのコンバート
		/// </summary>
		/// <param name="a_dstModel">出力モデル</param>
		/// <param name="a_srcModel">入力モデル : パース直後</param>
		/// <param name="a_fileDir">モデルのディレクトリパス</param>
		void ConvertMaterial(ModelData& a_dstModel,const GLTF::ModelData& a_srcModel,const std::string& a_fileDir)
		{
			//=================================================
			// マテリアル作成
			//=================================================

			// マテリアル配列を受け取れるサイズのメモリを確保
			a_dstModel.MaterialVec.resize(a_srcModel.materials.size());

			for (UINT _i = 0; _i < a_dstModel.MaterialVec.size(); ++_i)
			{
				// マテリアル情報
				a_dstModel.MaterialVec[_i] = {};
				const Engine::Resource::GLTF::Material& _srcMaterial = a_srcModel.materials[_i];	// 入力元
				Engine::Resource::Material& _dstMaterial = a_dstModel.MaterialVec[_i];				// 出力先

				// マテリアル情報コピー
				_dstMaterial.name = _srcMaterial.name;									// マテリアル名

				// アルファ
				if (_srcMaterial.alphaMode == "OPAQUE")
				{
					_dstMaterial.alphaMode = Engine::Resource::Alpha::Opaque;
				}
				else if (_srcMaterial.alphaMode == "MASK")
				{
					_dstMaterial.alphaMode = Engine::Resource::Alpha::Mask;
				}
				else if (_srcMaterial.alphaMode == "BLEND")
				{
					_dstMaterial.alphaMode = Engine::Resource::Alpha::Blend;
				}

				// 材質データ
				_dstMaterial.baseColor = _srcMaterial.baseColorFactor;				// 基本色
				_dstMaterial.metallic = _srcMaterial.metallicFactor;				// 金属製
				_dstMaterial.roughness = _srcMaterial.roughnessFactor;				// 粗さ
				_dstMaterial.emissive = _srcMaterial.emissiveFactor;				// エミッシブ

				// テクスチャセット
				_dstMaterial.SetTexture2D(
					a_fileDir,
					_srcMaterial.baseColorTexName,
					_srcMaterial.metallicRoughnessTexName,
					_srcMaterial.emissiveTexName,
					_srcMaterial.normalTexName
				);
			}
		}

		void ConvertAnimation(ModelData& a_dstModel,const GLTF::ModelData& a_srcModel)
		{
			a_dstModel.AnimationVec.resize(a_srcModel.animations.size());		// アニメーション配列確保

			for (UINT _i = 0; _i < a_dstModel.AnimationVec.size(); ++_i)
			{
				auto _srcAnimation = a_srcModel.animations[_i];	// 元データ

				a_dstModel.AnimationVec[_i] = {};
				Engine::Resource::AnimationData& _dstAnimation = a_dstModel.AnimationVec[_i];							// 出力先

				// アニメーション情報コピー
				_dstAnimation.name = _srcAnimation->name;									// 名前
				_dstAnimation.maxLength = _srcAnimation->maxLength;							// アニメーションの長さ

				_dstAnimation.nodes.resize(_srcAnimation->spAnimationNodes.size());			// ノード配列確保
				for (UINT _nIdx = 0; _nIdx < _dstAnimation.nodes.size(); ++_nIdx)
				{
					auto _srcNode = _srcAnimation->spAnimationNodes[_nIdx];	// 元データ
					Engine::Resource::AnimationNode& _dstAnimaNode = _dstAnimation.nodes[_nIdx];						// 出力先
					// ノード情報コピー
					_dstAnimaNode.nodeOffset = _srcNode->nodeOffset;				// 対象ノードのオフセット
					_dstAnimaNode.translations = _srcNode->translations;			// 座標キーリスト
					_dstAnimaNode.rotations = _srcNode->rotations;					// 回転キーリスト
					_dstAnimaNode.scales = _srcNode->scales;						// 拡縮キーリスト
				}

			}
		}
	}

	ModelData Engine::Resource::Converter::ModelConverter::ConvertModelData(
		const std::string& a_filePath,
		const GLTF::ModelData& a_rawModel
	)
	{
		// 出力データ
		ModelData _modelData = {};

		ConvertNodes(_modelData,a_rawModel);
		ConvertMaterial(_modelData,a_rawModel,a_filePath);
		ConvertNodes(_modelData,a_rawModel);

		return _modelData;
	}
}