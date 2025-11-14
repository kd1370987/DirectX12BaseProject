#pragma once

struct Mesh;
struct Vertex;

struct Material;
struct Node;
struct AnimationData;

struct aiMesh;
struct aiMaterial;

// インポートするときのパラメーター
struct ImportSettings
{
	const wchar_t*		pFilePath = nullptr;	// インポートするファイルパス
	std::vector<Mesh>&	meshes;					// メッシュデータの出力先
	bool				isInverseU = false;		// U座標を反転させるかどうか
	bool				isInverseV = true;		// V座標を反転させるかどうか
};


class AssimpLoader
{
public:

	bool Load(ImportSettings a_setting);		// モデルのロード

private:

	void LoadMesh(Mesh& a_dst, const aiMesh* a_src, bool a_isInverseU, bool a_isInverseV);
	void LoadTexture(const wchar_t* a_pFilePath, Mesh& a_dst, const aiMaterial* a_src);
};


