#pragma once

enum class RangeType
{
	CBV,
	SRV,
	UAV,
	Sampler,
};

enum class RootParameterType
{
	DescriptorTable,
	RootCBV,
	RootSRV,
	Bindless
};

enum class RootSigSemantic
{
	None,
	CameraCB,
	ObjectCB,
	MeshTransCB,
	MaterialCB,
	BoneCB,
	AmbientCB,
	UICB,
	MaterialSRV,
	PostScreenSRV,
};

struct RootRangeInit
{
	// レンジタイプ
	RangeType type;

	// ルート定数などを使用する際のシェーダーインデックス
	UINT shaderRegisterIndex;
};

struct RootParamInit
{
	RootParameterType paramType;				// パラメーター
	std::vector<RootRangeInit> rangeVec = {};	// レンジタイプ・インデックス

	// ルート定数などを使用する際のシェーダーインデックス
	UINT shaderRegisterIndex;					
};

struct RootSigInit
{
	void SetFlags(D3D12_ROOT_SIGNATURE_FLAGS a_flgas)
	{
		flags = a_flgas;
	}

	void AddDescriptorHeap(std::vector<RootRangeInit> a_rangeVec)
	{
		RootParamInit _init = {};
		_init.paramType = RootParameterType::DescriptorTable;
		_init.rangeVec = a_rangeVec;
		_init.shaderRegisterIndex = 0;
		paramVec.push_back(_init);
	}
	void AddRoot(RootParameterType a_type,UINT a_shaderIndex)
	{
		RootParamInit _init = {};
		_init.paramType = a_type;
		_init.rangeVec = {};
		_init.shaderRegisterIndex = a_shaderIndex;
		paramVec.push_back(_init);
	}

	// ルートシグネチャフラグ
	D3D12_ROOT_SIGNATURE_FLAGS flags = 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	std::vector<RootParamInit> paramVec = {};	// ルートパラメーター構築情報

	bool isUseStaticSampler = true;				// スタティックサンプラーを使用するかどうか
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
	DirectX::XMFLOAT4X4 viewInvMat = {};			// ビュー行列
	DirectX::XMFLOAT4X4 projMat = {};			// 射影行列
	DirectX::XMFLOAT4X4 projInvMat = {};			// 射影逆行列

	DirectX::XMFLOAT4 cameraPosXYZ = { 0.0f,0.0f,0.0f,0.0f };	// カメラのワールド座標
};

// スキニング
struct alignas(256) CBBone
{
	DirectX::XMFLOAT4X4 boneMat[300] = {};
};

// 環境
struct alignas(256) CBAmbient
{
	// 環境光の強さを表す定数バッファ
	DirectX::XMFLOAT4 ambientLightColor = { 0.f,0.f,0.f,0.f };

	// DirectionalLightの情報を表す定数バッファ
	DirectX::XMFLOAT4 directionalLightDir = { 0.f,0.f,0.f,0.f };
	DirectX::XMFLOAT4 directionalLightColor = { 0.f,0.f,0.f,0.f };
};

// UI
struct alignas(256) CBUI
{
	DirectX::XMFLOAT4X4 uiMat = {};

	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
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

template<>
struct RootSemanticTraits<RootSigSemantic::BoneCB>
{
	using Type = CBBone;
};

template<>
struct RootSemanticTraits<RootSigSemantic::AmbientCB>
{
	using Type = CBAmbient;
};

template<>
struct RootSemanticTraits<RootSigSemantic::UICB>
{
	using Type = CBUI;
};