#ifndef PARTICLE_CORE_HLSLI
#define PARTICLE_CORE_HLSLI

// -------------------------------------------------
// 構造体の定義
// -------------------------------------------------
struct ParticleData
{
	float3 pos; // 現在座標
	float life; // 残り寿命
	float3 velocity; // 現在の移動ベクトル
	float size; // スケール値
};

struct EmitData
{
	float3 pos; // 発生源のワールド座標
	uint emitCount; // 発生させる数
	float3 emitDirection; // 発生させたい方向
	float baseScale; // スケール値
};

// -------------------------------------------------
// 共通バッファの定義
// -------------------------------------------------
// 入力（UPLOADヒープから）
StructuredBuffer<EmitData> g_emitData : register(t0);

// 入出力（DEFAULTヒープ UAV）
RWStructuredBuffer<ParticleData> g_particleBuffer : register(u0);
RWStructuredBuffer<uint> g_deadList : register(u1);
RWStructuredBuffer<uint> g_counterBuffer : register(u2);

#endif // PARTICLE_CORE_HLSLI

// ルートシグネチャ設定
#define RS_PARTICLE_TABLE \
"DescriptorTable(SRV(t0,numDescriptors=1)),"\
"DescriptorTable(UAV(u0,numDescriptors=3))"
