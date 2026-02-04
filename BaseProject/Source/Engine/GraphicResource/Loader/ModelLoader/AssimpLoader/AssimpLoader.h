#pragma once

struct aiMesh;
struct aiMaterial;

//=========================================================
// Assimpマテリアルデータ
//=========================================================
struct AssimpMaterial
{
	std::wstring diffuseMap = L"none";			// ディフューズテクスチャのファイルパス
};

//=========================================================
// Assimpメッシュデータ
//=========================================================
struct AssimpMesh
{
	std::vector<MeshVertexFloat> vertices = {};		// 頂点データの配列
	std::vector<uint32_t> indices = {};			// インデックスの配列
	AssimpMaterial material = {};				// マテリアルデータ

	VertexBuffer* vertexBuffer = nullptr;		// 頂点バッファ
	IndexBuffer* indexBuffer = nullptr;			// インデックスバッファ
	//DescriptorHandle* materialHandle;	// テクスチャハンドル
	Storage::Range srvHandle = {};

	uint32_t materialIndex = 0;		// マテリアルインデックス
	bool isSkinMesh = false;		// スキンメッシュかどうか
};

//=========================================================
// Assimpノードデータ
//=========================================================
struct AssimpNode
{
	std::string					name = "none";				// ノード名

	DirectX::XMFLOAT4X4			localTransform = {};		// ローカル行列
	DirectX::XMFLOAT4X4			worldTransform = {};		// ワールド行列
	
	int 						parent = -1;		// 親インデックス
	std::vector<int>			children = {};			// 子供リスト

	int 						boneIndex = -1;		// ボーンインデックス
	bool 						isSkinMesh = false;	// スキンメッシュ持ちかどうか

	std::shared_ptr<AssimpMesh>		spMesh = nullptr;				// メッシュ
};

//=========================================================
// Assimpモデルデータ
//=========================================================
struct AssimpModel
{
	std::vector<AssimpNode>			nodes = {};			// ノードリスト
};

// インポートするときのパラメーター
struct ImportSettings
{
	const wchar_t*		pFilePath = nullptr;	// インポートするファイルパス
	std::vector<AssimpMesh>&	meshes;					// メッシュデータの出力先
	bool				isInverseU = false;		// U座標を反転させるかどうか
	bool				isInverseV = true;		// V座標を反転させるかどうか
};


class AssimpLoader
{
public:

	bool Load(ImportSettings a_setting);		// モデルのロード
	bool Load(
		std::string& 				a_filePath,			// インポートするファイルパス
		std::vector<AssimpMesh>&	a_meshes,			// メッシュデータの出力先
		bool						a_isInverseU,		// U座標を反転させるかどうか
		bool						a_isInverseV		// V座標を反転させるかどうか
	);
	bool Load(
		std::string a_filePath,		// インポートするファイルパス
		AssimpModel& a_model,			// モデルデータの出力先
		bool		 a_isInverseU,		// U座標を反転させるかどうか
		bool		 a_isInverseV		// V座標を反転させるかどうか
	);
private:

	void LoadMesh(AssimpMesh& a_dst, const aiMesh* a_src, bool a_isInverseU, bool a_isInverseV);
	void LoadTexture(const wchar_t* a_pFilePath, AssimpMesh& a_dst, const aiMaterial* a_src);
};


