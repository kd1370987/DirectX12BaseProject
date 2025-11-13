#include "Model.h"

// モデルローダー
#include "ModelLoader/Assimp/AssimpLoader.h"
#include "ModelLoader/TinyGLTF/TinyGLTFLoader.h"


#include "ModelResource/Animation/Animation.h"
#include "ModelResource/Mesh/Mesh.h"
#include "ModelResource/Node/Node.h"
#include "ModelResource/Material/Material.h"

//==========================================================
// 
// 初期化・解放
// 
//==========================================================
ModelResource::ModelResource()
{
}
ModelResource::~ModelResource()
{
	Release();
}

//==========================================================
// 
// モデル読み込み
// 
//==========================================================
bool ModelResource::Load(const std::string& a_filePath)
{
	// データをクリア
	Release();

	//-------------------------------------
	// Assimpを使用する場合
	//-------------------------------------

	const wchar_t* _modelPath = StringUtility::ToWideString(a_filePath).c_str();		// 文字列変換
	ImportSettings _impSet = 
	{
		_modelPath,
		m_meshes,
	};
	AssimpLoader _assimp;
	if(_assimp.Load())


	// モデル読み込み（とりあえずGLTFのみ）
	std::shared_ptr<GLTFModel> _spGltfModel = GLTFLoader::LoadGLTFModel(a_filePath.data());
	if (_spGltfModel == nullptr) 
	{
		// 読み込み失敗
		printf("モデルの読み込みに失敗\n");
		return false;
	}

	// モデルシーンクリエイト
	CreateNodes(_spGltfModel);					// ノード作成
	CreateMaterials(_spGltfModel, _fileDir);	// マテリアル作成
	CreateAnimations(_spGltfModel);				// アニメーション作成

	// 読み込み成功
	return true;
}

const std::shared_ptr<Mesh> ModelResource::GetMesh(UINT a_idx) const
{
	// メッシュを持っているのならメッシュのシェアードポインタを返す
	return a_idx < m_originalNodes.size() ? m_originalNodes[a_idx].spMesh : nullptr;
}

Node* ModelResource::FindNode(std::string a_name)
{
	// 名前が一致するノードを探索
	for (auto&& _node : m_originalNodes)
	{
		if (_node.name == a_name)
		{
			return &_node;
		}
	}

	// 無ければnllptrを返す
	return nullptr;
}

const std::shared_ptr<AnimationData> ModelResource::GetAnimation(std::string_view a_animaName) const
{
	return std::shared_ptr<AnimationData>();
}

const std::shared_ptr<AnimationData> ModelResource::GetAnimation(UINT a_idx) const
{
	return std::shared_ptr<AnimationData>();
}

bool ModelResource::IsSkinMesh()
{
	return false;
}

//==========================================================
//
// モデルシーンクリエイト関数
//
//==========================================================
void ModelResource::CreateNodes(const std::shared_ptr<GLTFModel>& a_spGltfModel)
{
	//=================================================
	// ノード作成
	//=================================================
	m_originalNodes.resize(a_spGltfModel->nodes.size());		// ノード配列確保
	for (UINT _i = 0; _i < a_spGltfModel->nodes.size(); ++_i)
	{
		// ノード情報
		const GLTFNode& _srcNode = a_spGltfModel->nodes[_i];	// 元データ
		Node& _dstNode = m_originalNodes[_i];						// 出力先

		// 基本情報コピー
		if (_srcNode.isMesh)
		{
			// メッシュ作成
			_dstNode.spMesh = std::make_shared<Mesh>();

			// メッシュデータコピー
			if (_dstNode.spMesh)
			{
				_dstNode.spMesh->Create(
					_srcNode.nodeMesh.vertices,
					_srcNode.nodeMesh.faces,
					_srcNode.nodeMesh.subsets,
					_srcNode.nodeMesh.isSkinMesh
				);
			}

			// メッシュノードリストにインデックス登録
			m_meshNodeIndices.push_back(_i);
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
			m_collisionMeshNodeIndices.push_back(_i);
		}
		else
		{
			// 描画用ノードに割り当て
			m_drawMeshNodeIndices.push_back(_i);
		}	
	}

	//=================================================
	// ルートノード・ボーンノードリスト作成
	//=================================================
	for (UINT _nodeIdx = 0; _nodeIdx < a_spGltfModel->nodes.size(); ++_nodeIdx)
	{
		// ルートノードの登録
		if (a_spGltfModel->nodes[_nodeIdx].parent == -1)
		{
			m_rootNodeIndices.push_back(_nodeIdx);
		}
		// ボーンノードの登録
		int _boneIdx = a_spGltfModel->nodes[_nodeIdx].boneNodeIndex;		// ボーンインデックス取得
		if (_boneIdx >= 0)
		{
			// ボーンノードリストのサイズ確保
			if (_boneIdx >= (int)m_boneNodeIndices.size())
			{
				m_boneNodeIndices.resize(_boneIdx + 1);
			}

			// 登録
			m_boneNodeIndices[_boneIdx] = _nodeIdx;
		}
	}

	// 当たり判定用ノードがなければ、m_drawMeshNodeと同じ割り当てを行う
	if (m_collisionMeshNodeIndices.size() == 0)
	{
		m_collisionMeshNodeIndices = m_drawMeshNodeIndices;
	}
}
void ModelResource::CreateMaterials(const std::shared_ptr<GLTFModel>& a_spGltfModel, const std::string& a_fileDir)
{
	//=================================================
	// マテリアル作成
	//=================================================
	
	// マテリアル配列を受け取れるサイズのメモリを確保
	m_materials.resize(a_spGltfModel->materials.size());

	for (UINT _i = 0; _i < m_materials.size(); ++_i)
	{
		// マテリアル情報
		const GLTFMaterial& _srcMaterial = a_spGltfModel->materials[_i];	// 元データ
		Material& _dstMaterial = m_materials[_i];							// 出力先

		// マテリアル情報コピー
		

	}
}
void ModelResource::CreateAnimations(const std::shared_ptr<GLTFModel>& a_spGltfModel)
{
}

void ModelResource::Release()
{
}
