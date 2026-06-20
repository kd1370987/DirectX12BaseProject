#include "../../../Common/RootParameters/Particle.hlsli"

// ルートシグネチャ
#define UPDATEPARTICLE_ROOT_SIG \
	"RootFlags(0)," \
	"CBV(b0)," \
	"DescriptorTable(SRV(t0,numDescriptors=1)),"\
	"DescriptorTable(UAV(u0,numDescriptors=3))"

// 定数バッファ (C++側から毎フレーム渡す)
cbuffer UpdateCB : register(b0)
{
	float deltaTime; // 経過時間
	float3 gravity; // 重力など
};

// 入力
StructuredBuffer<EmitData> g_emitData : register(t0);

// 入出力
RWStructuredBuffer<ParticleData> g_particleBuffer : register(u0);
RWStructuredBuffer<uint> g_deadList : register(u1);
RWStructuredBuffer<uint> g_counterBuffer : register(u2);

// ルートシグネチャセット
[RootSignature(UPDATEPARTICLE_ROOT_SIG)]

// １スレッド当たり
[numthreads(32, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// バッファの最大容量を取得し、範囲外アクセスを防ぐ
	uint _maxCapacity, _stride;
	g_particleBuffer.GetDimensions(_maxCapacity,_stride);

	// 配列外アクセス防止
	uint _particleIndex = DTid.x;
	if (_particleIndex >= _maxCapacity) return;

	// 自分が担当するパーティクルを読み込む
	ParticleData _p = g_particleBuffer[_particleIndex];

	// すでに死んでいるパーティクルなら何もしない
	if (_p.life <= 0.0f) return;

	// パーティクルの更新ロジック
	_p.life -= deltaTime;					// 寿命を減らす
	_p.velocity += gravity * deltaTime;		// 重力を減らす
	_p.pos += _p.velocity * deltaTime;		// 座標を更新

	// デッドリストへの返却
	if(_p.life <= 0.0f)
	{
		uint _count;

		// カウンターを１増やし、増やす前の値取得
		InterlockedAdd(g_counterBuffer[0], 1, _count);

		// デッドリストに返却
		g_deadList[_count] = _particleIndex;
	}

	// 更新したデータをVRAMに書き戻す
	g_particleBuffer[_particleIndex] = _p;
}
