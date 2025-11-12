#pragma once
struct AnimationData;
struct GLTFModel;
struct Node;

class Mesh;
class Material;

//===============================================
//
// 静的モデル
//
//===============================================
class ModelResource
{
public:

	//=================================================
	// 初期化・解放
	//=================================================
	ModelResource();
	~ModelResource();

	//=================================================
	// モデルをファイルからロード
	//=================================================
	bool Load(std::string_view a_filePath);

	//=================================================
	// アクセサ
	//=================================================
	const std::shared_ptr<Mesh> GetMesh(UINT a_idx) const;							// メッシュ取得
	Node* FindNode(std::string a_name);												// ノード探索

	const std::vector<Material>& GetMaterials() const { return m_materials; }		// マテリアル配列
	const std::vector<Node>& GetOriginalNodes() const { return m_originalNodes; }	// ノード配列

	// アニメーションデータ
	const std::shared_ptr<AnimationData> GetAnimation(std::string_view a_animaName) const;
	const std::shared_ptr<AnimationData> GetAnimation(UINT a_idx) const;

	// それぞれのノードのインデックスリストを取得
	const std::vector<int>& GetRootNodeIndices()			const { return m_rootNodeIndices; }				// ルート
	const std::vector<int>& GetBoneNodeIndices()			const { return m_boneNodeIndices; }				// ボーン
	const std::vector<int>& GetMeshNodeIndices()			const { return m_meshNodeIndices; }				// メッシュ付き
	const std::vector<int>& GetDrawMeshNodeIndices()		const { return m_drawMeshNodeIndices; }			// 描画用
	const std::vector<int>& GetCollisionMeshNodeIndices()	const { return m_collisionMeshNodeIndices; }	// 当たり判定

	// スキンメッシュ持ちかどうか
	bool IsSkinMesh();

private:

	//=================================================
	// モデルシーンクリエイト関数
	//=================================================
	void CreateNodes(const std::shared_ptr<GLTFModel>& a_spGltfModel);										// ノード作成
	void CreateMaterials(const std::shared_ptr<GLTFModel>& a_spGltfModel,const std::string& a_fileDir);		// マテリアル作成
	void CreateAnimations(const std::shared_ptr<GLTFModel>& a_spGltfModel);									// アニメーション作成

	//=================================================
	// 解放処理
	//=================================================
	void Release();

private:

	// マテリアル
	std::vector<Material>						m_materials;				// マテリアルの配列

	// アニメーション
	std::vector<std::shared_ptr<AnimationData>> m_spAnimations;				// データリスト

	// ノード
	std::vector<Node>							m_originalNodes;			// 全ノード配列
	std::vector<int>							m_rootNodeIndices;			// Rootノード
	std::vector<int>							m_boneNodeIndices;			// ボーンノード
	std::vector<int>							m_meshNodeIndices;			// メッシュが存在するノード
	std::vector<int>							m_collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
	std::vector<int>							m_drawMeshNodeIndices;		// 描画するノード
};


//===============================================
//
// 動的モデル
//
//===============================================
class ModelInstance
{
public:

private:

};