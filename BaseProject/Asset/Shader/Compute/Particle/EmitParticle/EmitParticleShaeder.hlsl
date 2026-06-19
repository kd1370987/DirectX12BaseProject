
#include "../../../Source/RootSignatureLayout.hlsli"

// ルートシグネチャ
#define EMITPARTICLE_ROOT_SIG \
"RootFlags(0),"\
"DescriptorTable(SRV(t0,numDescriptors=1)),"\
"DescriptorTable(UAV(u0,numDescriptors=3))"

// パーティクルデータ
struct ParticleData
{
	float3 pos;			// 現在座標
	float life;			// 残り寿命

	float3 velocity;	// 現在の移動ベクトル
	float size;			// スケール値
};

// 生成用データ
struct EmitData
{
	float3 pos; // 発生源のワールド座標
	uint emitCount; // 発生させる数

	float3 emitDirection; // 発生させたい方向
	float baseScale; // スケール値
};

// 入力
StructuredBuffer<EmitData>			g_emitData			: register(t0); // 生成用データ

// 入出力
RWStructuredBuffer<ParticleData>	g_particleBuffer	: register(u0); // パーティクルデータ
RWStructuredBuffer<uint>			g_deadList			: register(u1);	// デッドリスト
RWStructuredBuffer<uint>			g_counterBuffer		: register(u2); // パーティクルデータ


[RootSignature(EMITPARTICLE_ROOT_SIG)]

// １スレッド当たり
[numthreads(32, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 
}
