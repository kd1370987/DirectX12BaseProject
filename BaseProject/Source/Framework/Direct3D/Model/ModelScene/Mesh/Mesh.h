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