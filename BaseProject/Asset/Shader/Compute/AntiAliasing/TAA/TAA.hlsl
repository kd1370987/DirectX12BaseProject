
#include "../../../Source/CalcNormal.hlsli"
#include "../../../Source/RootSignatureLayout.hlsli"

// ルートシグネチャデータ
#define TEMPORALACCUMULATION_ROOT_SIG \
"RootFlags(0), " \
"DescriptorTable(SRV(t0, numDescriptors=5)),"\
"DescriptorTable(UAV(u0, numDescriptors=1)),"\
RS_STATIC_SAMPLER

// 入力
Texture2D<float4> g_currentColorTex : register(t0); // 現在の色
Texture2D<float4> g_historyColorTex : register(t1); // 過去の色
Texture2D<float4> g_motionVectorTex : register(t2); // モーションベクター
Texture2D<float4> g_depthTex		: register(t3); // 現在深度
Texture2D<float4> g_normalTex		: register(t4); // 現在法線

// 出力
RWTexture2D<float4> g_outputTAA : register(u0); // 結果書き込み用



[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
}
