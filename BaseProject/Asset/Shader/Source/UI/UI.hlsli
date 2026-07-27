// ヘルパー関数
#include "../../Common/Math/Transform.hlsli"
#include "../../Common/Math/Normal.hlsli"

// ルートパラメター
#include "../../Common/CB/CBCamera.hlsli"
#include "../RootSignatureLayout.hlsli"

struct UIData
{
	uint texIndex;
	
	float2 pos;		// 座標
	float2 size;	// サイズ
	float rotation; // 回転
	
	float layer;	// Z順

	float2 uvOffset;		// UVをずらす際のオフセット

	float4 color;		// 色調補正

	float2 pivot;		// 中心点
};

// ルートシグネチャ
#define UI_RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
	"SRV(t0)," \
	RS_STATIC_SAMPLER

// UIデータ : CPU側でインスタンス順に並んでいる
StructuredBuffer<UIData> g_uiDataBuffer : register(t0);

// サンプラー
SamplerState g_samp : register(s0);

// 頂点入力
struct VSInput
{
	float4 pos : POSITION; // 頂点座標
	float2 uv : TEXCOORD0; // UV座標
	uint instID : SV_InstanceID; // インスタンス番号
};

// 頂点出力
struct VSOutput
{
	float4 pos : SV_Position; // 射影行列
	
	float2 uv : TEXCOORD0;
	float4 color : TEXCOORD1;
	
	uint instID : SV_InstanceID; // インスタンス番号
};
