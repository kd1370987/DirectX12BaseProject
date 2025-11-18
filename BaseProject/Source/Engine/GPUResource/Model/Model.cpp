#include "Model.h"

#include "ModelLoader/TinyGLTF/TinyGLTFLoader.h"
#include "ModelLoader/Assimp/AssimpLoader.h"


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
	//-------------------------------------
	// データをクリア
	//-------------------------------------
	Release();

	//-------------------------------------
	// 対応形式チェック
	//-------------------------------------
	std::string _fileDir = FileUtility::GetDirFromPath(a_filePath);		// 親ディレクトリパス取得
	std::string _ext = FileUtility::GetFilePathExtension(a_filePath);	// 拡張子取得

	// 対応拡張子のファイルをディレクトリ内から全て取得
	auto _modelBinFile = FileUtility::FindExtensionInDirectory(_fileDir, ".modelBin");
	
	printf("モデル読み込み開始 : %s\n", a_filePath.c_str());

	//-------------------------------------
	// 独自の形式があった場合
	//-------------------------------------
	if (_modelBinFile.size() > 0)
	{
		// モデルクラスをバイナリ化したデータを読み込む
		printf("独自形式のモデル読み込みは未実装\n");
	}
	//-------------------------------------
	// TinyGLTFを使用する場合
	//-------------------------------------
	else if (_ext == "gltf" || _ext == "glb")
	{
		// GLTFもしくはGLB形式のモデルデータを読み込む
		std::shared_ptr<GLTFModel> _spGltfModel = nullptr;
		TinyGLTFLoader _loader;
		_spGltfModel = _loader.LoadModel(a_filePath);
		if (!_spGltfModel)
		{
			// 読み込み失敗
			printf("モデルの読み込みに失敗\n");
			return false;
		}

		// ノード作成
		CreateNodes(_spGltfModel);
		// マテリアル作成
		CreateMaterials(_spGltfModel, _fileDir);
		// アニメーション作成
		CreateAnimations(_spGltfModel);

		printf("モデルの読み込みに成功 : %s\n", a_filePath.c_str());
	}
	//-------------------------------------
	// Assimpを使用する場合
	//-------------------------------------
	else
	{
		// Assimpを使用してモデルデータを読み込む
		printf("Assimpを使用したモデル読み込みは未実装\n");
	}
	
	printf("モデル読み込み終了 : %s\n", a_filePath.c_str());

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
void ModelResource::CreateAnimations(const std::shared_ptr<GLTFModel>& a_spGltfModel)
{
	//=================================================
	// アニメーション作成
	//=================================================
	m_spAnimations.resize(a_spGltfModel->animations.size());		// アニメーション配列確保

	for (UINT _i = 0; _i < m_spAnimations.size(); ++_i)
	{
		const GLTFAnimationData& _srcAnimation = *a_spGltfModel->animations[_i];	// 元データ

		m_spAnimations[_i] = std::make_shared<AnimationData>();
		AnimationData& _dstAnimation = *m_spAnimations[_i];							// 出力先

		// アニメーション情報コピー
		_dstAnimation.name = _srcAnimation.name;									// 名前
		_dstAnimation.maxLength = _srcAnimation.maxLength;							// アニメーションの長さ

		_dstAnimation.nodes.resize(_srcAnimation.spAnimationNodes.size());			// ノード配列確保
		for (UINT _nIdx = 0; _nIdx < _dstAnimation.nodes.size(); ++_nIdx)
		{
			const AnimationNode& _srcAnimaNode = *_srcAnimation.spAnimationNodes[_nIdx];	// 元データ
			AnimationNode& _dstAnimaNode = _dstAnimation.nodes[_nIdx];						// 出力先
			// ノード情報コピー
			_dstAnimaNode.nodeOffset = _srcAnimaNode.nodeOffset;				// 対象ノードのオフセット
			_dstAnimaNode.translations = _srcAnimaNode.translations;			// 座標キーリスト
			_dstAnimaNode.rotations = _srcAnimaNode.rotations;					// 回転キーリスト
			_dstAnimaNode.scales = _srcAnimaNode.scales;						// 拡縮キーリスト
		}

	}
}

void ModelResource::Release()
{
	m_materials.clear();
	m_originalNodes.clear();

	m_rootNodeIndices.clear();
	m_boneNodeIndices.clear();
	m_meshNodeIndices.clear();
}
