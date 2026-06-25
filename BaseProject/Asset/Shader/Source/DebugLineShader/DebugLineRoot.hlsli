// ルートパラメターズ
#include "../../Common/CB/CBCamera.hlsli"

// ヘルパー関数
#include "../../Common/Math/Transform.hlsli"
#include "../../Common/Math/Normal.hlsli"

#include "../RootSignatureLayout.hlsli"

// デバッグライン用構造体
struct DebugLine
{
	float4 color;
	float4x4 worldMat;
	uint shapeType;
};
StructuredBuffer<DebugLine> g_debuglineBuffer : register(t0);

// ルートシグネチャ
#define DEBUGLINE_ROOT_SIG \
RS_FLAGS","\
RS_CAMERA_CB ","\
"DescriptorTable(SRV(t0, numDescriptors=1),visibility = SHADER_VISIBILITY_VERTEX)"

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
