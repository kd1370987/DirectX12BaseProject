#pragma once

//==========================================================
// メッシュ用 頂点情報
//==========================================================
struct MeshVertex8bit
{
	DirectX::XMFLOAT3		pos;					// 座標
	DirectX::XMFLOAT2		uv;						// uv座標
	unsigned int			color = 0xFFFFFFFF;		// RGBA(各色0～255のUINT型)
	DirectX::XMFLOAT3		normal;					// 法線
	DirectX::XMFLOAT3		tangent;				// 接線

	std::array<short, 4>	skinIndexList;			// スキニングIndexリスト
	std::array<float, 4>	skinWeightList;			// スキニングウェイトリスト
};
struct MeshVertexFloat
{
	DirectX::XMFLOAT3		pos;					// 座標
	DirectX::XMFLOAT3		normal;					// 法線
	DirectX::XMFLOAT2		uv;						// uv座標
	DirectX::XMFLOAT3		tangent;				// 接線
	DirectX::XMFLOAT4		color;					// RGBA(各色0.0f～1.0fのFLOAT型)
};