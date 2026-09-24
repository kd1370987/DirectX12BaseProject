// ルートパラメーターの構造体
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/DebugLineData.hlsli"

#include "../../../Common/RootSignatureLayout.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)         カメラ
//   1 : SRVの番号(t0) 描く形状の一覧
//
// 頂点バッファは持たない。形状ごとの線をVSが SV_VertexID から組み立てる
//==========================================================================================
#define DEBUGLINE_ROOT_SIG \
RS_FLAGS","\
"CBV(b0, visibility = SHADER_VISIBILITY_ALL),"\
"RootConstants(num32BitConstants=1, b100, visibility = SHADER_VISIBILITY_VERTEX)"

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_debuglineBufferIndex;
}

StructuredBuffer<DebugLineData> Get_debuglineBuffer() { StructuredBuffer<DebugLineData> _r = ResourceDescriptorHeap[g_debuglineBufferIndex]; return _r; }
#define g_debuglineBuffer Get_debuglineBuffer()

// ヘルパー関数 : 上で宣言した g_camera を使うので、必ずこの位置で読むこと
#include "../../../Common/Math/Transform.hlsli"
#include "../../../Common/Math/Normal.hlsli"

// 頂点シェーダー入力構造体
struct VSInput
{
	uint vertexID : SV_VertexID;
	uint instID : SV_InstanceID;
};

struct VSOutput
{
	float4 svPos : SV_Position;
	float4 color : COLOR;
};
