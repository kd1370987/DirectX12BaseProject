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

	ModelResource(){}
	~ModelResource(){}

	bool Load(std::string_view a_filePath);

	// アクセサ
	const std::shared_ptr<Mesh> GetMesh(UINT a_idx) const 
	{
		return a_idx < m_originalNodes.size() ? m_originalNodes[a_idx]
	}

private:

	// モデルシーンクリエイト関数
	void CreateNodes(const std::shared_ptr<GLTFModel>& a_spGltfModel);										// ノード作成
	void CreateMaterials(const std::shared_ptr<GLTFModel>& a_spGltfModel,const std::string& a_fileDir);		// マテリアル作成
	void CreateAnimations(const std::shared_ptr<GLTFModel>& a_spGltfModel);									// アニメーション作成

	// 解放処理
	void Release();

private:

	// マテリアル
	std::vector<Material>						m_materials;				// マテリアルの配列

	// アニメーション
	std::vector<std::shared_ptr<AnimationData>> m_spAnimations;				// データリスト

	// ノード
	std::vector<Node>							m_originalNodes;			// 全ノード配列
	std::vector<int>							m_rootNodeIndeces;			// Rootノード
	std::vector<int>							m_booneNodeIndeices;		// ボーンノード
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