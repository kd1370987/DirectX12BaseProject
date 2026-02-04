#pragma once

enum class RangeType
{
	CBV,
	SRV,
	UAV,
};

enum class RootParameterType
{
	DescriptorTable,
	RootCBV,
};

enum class RootSigSemantic
{
	None,
	CameraCB,
	ObjectCB,
	MeshTransCB,
	MaterialCB,
	MaterialSRV,
	PostScreenSRV,
};

struct RootSigLayout
{
	RootParameterType parameType = RootParameterType::DescriptorTable;
	std::vector<RangeType> rangeTypeVec = {};

	RootSigSemantic semantic = RootSigSemantic::None;
	bool isCBV = false;
};

// 構造体
// UV操作
struct alignas(256) CBObject
{
	DirectX::XMFLOAT4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
};

// メッシュ座標用定数バッファ
struct alignas(256) CBMeshTrans
{
	DirectX::XMFLOAT4X4 worldMat = {};
};

// マテリアル単位更新用定数バッファ
struct alignas(256) CBMaterial
{
	DirectX::XMFLOAT4 baseColorXYZW = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT4 emissiveXYZ = { 0.0f,0.0f,0.0f,0.0f };
	DirectX::XMFLOAT4 metallicRoughnessXY = { 0.0f,0.0f,0.0f,0.0f };
};


// カメラ用定数バッファ
struct alignas(256) CBCamera
{
	DirectX::XMFLOAT4X4 viewMat = {};			// ビュー行列
	DirectX::XMFLOAT4X4 projMat = {};			// 射影行列
	DirectX::XMFLOAT4X4 projInvMat = {};			// 射影逆行列

	DirectX::XMFLOAT4 cameraPosXYZ = { 0.0f,0.0f,0.0f,0.0f };	// カメラのワールド座標
};

// Traitsパターン。セマンティクスごとにTraits定義
template<RootSigSemantic s>
struct RootSemanticTraits;

template<>
struct RootSemanticTraits<RootSigSemantic::CameraCB>
{
	using Type = CBCamera;
};

template<>
struct RootSemanticTraits<RootSigSemantic::ObjectCB>
{
	using Type = CBObject;
};

template<>
struct RootSemanticTraits<RootSigSemantic::MeshTransCB>
{
	using Type = CBMeshTrans;
};

template<>
struct RootSemanticTraits<RootSigSemantic::MaterialCB>
{
	using Type = CBMaterial;
};
