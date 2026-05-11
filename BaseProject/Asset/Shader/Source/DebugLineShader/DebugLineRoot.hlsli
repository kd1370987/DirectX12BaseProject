
#include "../RootSignatureLayout.hlsli"

#define DEBUGLINE_ROOT_SIG \
RS_FLAGS","\
RS_CAMERA_CB ","\
"CBV(b1,visibility = SHADER_VISIBILITY_ALL), " \

// カメラの定数バッファ
cbuffer cbCamera : register(b0)
{
	float4x4 viewMat;		// ビュー行列
	float4x4 viewInvMat;	// ビュー行列
	float4x4 projMat;		// 投影行列
	float4x4 projInvMat;	// 投影行列の逆行列

	float4 cameraPos;		// カメラ位置
}

// メッシュのワールド行列
cbuffer cbTransform : register(b1)
{
	float4x4 mat;			// ワールド行列
}

// ボーン行列
//cbuffer cbBones : register(b2)
//{
//	row_major float4x4 boneMats[300];
//};

// 頂点シェーダー入力構造体
struct VSInput
{
	float3 pos : POSITION;
};

// アニメーション頂点シェーダー入力構造体
struct AnimationVSInput
{
	float3 pos : POSITION;			// 頂点座標
	uint4 skinIndex : SKININDEX;	// スキンメッシュのボーンインデックス（何番目のボーンに影響しているかのデータ（最大４））
	float4 skinWeight : SKINWEIGHT; // ボーンの影響度（最大４）
};

struct VSOutput
{
	float4 svPos : SV_Position;
};
