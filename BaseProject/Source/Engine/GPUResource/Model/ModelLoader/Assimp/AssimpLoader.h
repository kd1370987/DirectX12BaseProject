#pragma once

struct aiMesh;
struct aiMaterial;

//=========================================================
// Assimp頂点データ
//=========================================================
struct AssimpVertex
{
	DirectX::XMFLOAT3 position;		// 位置座標
	DirectX::XMFLOAT3 normal;		// 法線
	DirectX::XMFLOAT2 uv;			// uv座標
	DirectX::XMFLOAT3 tangent;		// 接空間
	DirectX::XMFLOAT4 color;		// 頂点色
};

//=========================================================
// Assimpマテリアルデータ
//=========================================================
struct AssimpMaterial
{
	std::wstring diffuseMap;			// ディフューズテクスチャのファイルパス
};

//=========================================================
// Assimpメッシュデータ
//=========================================================
struct AssimpMesh
{
	std::vector<AssimpVertex> vertices;		// 頂点データの配列
	std::vector<uint32_t> indices;			// インデックスの配列
	AssimpMaterial material;				// マテリアルデータ

	VertexBuffer* vertexBuffer;		// 頂点バッファ
	IndexBuffer* indexBuffer;			// インデックスバッファ
	DescriptorHandle* materialHandle;	// テクスチャハンドル

	uint32_t materialIndex = 0;		// マテリアルインデックス
	bool isSkinMesh = false;		// スキンメッシュかどうか
};

//=========================================================
// Assimpノードデータ
//=========================================================
struct AssimpNode
{
	std::string					name;				// ノード名

	DirectX::XMFLOAT4X4			localTransform;		// ローカル行列
	DirectX::XMFLOAT4X4			worldTransform;		// ワールド行列
	
	int 						parent = -1;		// 親インデックス
	std::vector<int>			children;			// 子供リスト

	int 						boneIndex = -1;		// ボーンインデックス
	bool 						isSkinMesh = false;	// スキンメッシュ持ちかどうか

	std::shared_ptr<AssimpMesh>		spMesh;				// メッシュ
};

//=========================================================
// Assimpモデルデータ
//=========================================================
struct AssimpModel
{
	std::vector<AssimpNode>			nodes;			// ノードリスト
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


