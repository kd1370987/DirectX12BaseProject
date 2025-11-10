#pragma once

//==========================================================
// メッシュ用 頂点情報
//==========================================================
struct MeshVertex
{
	DirectX::XMFLOAT3		pos;					// 座標
	DirectX::XMFLOAT2		uv;						// uv座標
	unsigned int			color = 0xFFFFFFFF;		// RGBA(各色0～255のUINT型)
	DirectX::XMFLOAT3		normal;					// 法線
	DirectX::XMFLOAT3		tangent;				// 接線

	std::array<short, 4>	skinIndexList;			// スキニングIndexリスト
	std::array<float, 4>	skinWeightList;			// スキニングウェイトリスト
};

//==========================================================
// メッシュ用 面情報
//==========================================================
struct MeshFace
{
	UINT idx[3];			// 三角形を構成する頂点のIndex
};

//==========================================================
// メッシュ用 サブセット情報
//==========================================================
struct MeshSubset
{
	UINT materialNumber = 0;		// マテリアルナンバー
	UINT faceStart = 0;				// 面Index : このマテリアルで使用されている最初の面のIndex
	UINT faceCount = 0;				// 面数    : faceStartから、何枚の面が使用されているかの数
};

//==========================================================
// 
// メッシュ用クラス
// 
//==========================================================
class Mesh
{
public:

	//=================================================
	// 作成・解放
	//=================================================
	Mesh() {}
	~Mesh() {}

	// メッシュ作成（成功 : true / 失敗 : false）
	bool Create(
		const std::vector<MeshVertex>&	a_vertices,			// 頂点配列
		const std::vector<MeshFace>&	a_face,				// 面インデックス情報配列
		const std::vector<MeshSubset>&	a_subsets,			// サブセット情報配列
		bool							a_isSkinMesh		// スキンメッシュ持ちかどうか
	);

	// メッシュ解放
	void Release();


private:

	// バッファ
	VertexBuffer					m_vertexBuffer;		// 頂点バッファ
	IndexBuffer						m_indexBuffer;		// インデックスバッファ

	// サブセット情報
	std::vector<MeshSubset>			m_subsets;

	// 境界データ
	DirectX::BoundingBox			m_aabb;				// 軸平行境界ボックス
	DirectX::BoundingSphere			m_bSphere;			// 境界球

	// 座標のみの配列情報
	std::vector<DirectX::XMFLOAT3>	m_positions;

	// 面情報のみの配列
	std::vector<MeshFace>			m_faces;

	// スキンメッシュかどうか
	bool							m_isSkinMesh = false;

private:
	// コピー禁止
	Mesh(const Mesh& src) = delete;
	void operator=(const Mesh& src) = delete;
};