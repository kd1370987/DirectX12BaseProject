
#include "../../../Common/RootParameters/Particle.hlsli"

// ルートシグネチャ
#define EMITPARTICLE_ROOT_SIG \
	"RootFlags(0),"\
	"CBV(b0),"\
	"DescriptorTable(SRV(t0,numDescriptors=1)),"\
	"DescriptorTable(UAV(u0,numDescriptors=3))"

cbuffer EmitCB : register(b0)
{
	uint requestCount;		// 今回発生するエミット命令の数
}

// 入力（UPLOADヒープから）
StructuredBuffer<EmitData> g_emitData : register(t0);

// 入出力（DEFAULTヒープ UAV）
RWStructuredBuffer<ParticleData> g_particleBuffer : register(u0);
RWStructuredBuffer<uint> g_deadList : register(u1);
RWStructuredBuffer<uint> g_counterBuffer : register(u2);


// ルートシグネチャセット
[RootSignature(EMITPARTICLE_ROOT_SIG)]

// １スレッド当たり
[numthreads(32, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{

	//// デバッグ用：最初の10スレッド（10個のパーティクル）だけ処理する
	//if (DTid.x >= 10)
	//	return;
	
	//// エミットバッファ（g_emitData）からの読み込みやforループは全部無視！

	//uint _origCount;

	//// カウンターから１引いて、引く前の数を origCount に取得する
	//InterlockedAdd(g_counterBuffer[0], -1, _origCount);

	//// 空きがあった場合
	//if (_origCount > 0)
	//{
	//	uint _newIndex = g_deadList[_origCount - 1];

	//	// 新しいパーティクルデータを適当な値で初期化
	//	ParticleData _p;
	//	_p.pos = float3(0.0f, 0.0f, 0.0f); // 画面の中心（原点）
	//	_p.life = 5.0f; // すぐ死なないように長めの5秒
	//	_p.velocity = float3(0.0f, 2.0f, 0.0f); // 上に向かって飛ぶ
	//	_p.size = 1.0f; // 見えやすいようにデカく

	//	// 書き込み！
	//	g_particleBuffer[_newIndex] = _p;
	//}
	//else
	//{
	//	// カウンターのマイナスを戻す
	//	InterlockedAdd(g_counterBuffer[0], 1, _origCount);
	//}
	// リクエスト以上のスレッドは落とす
	if (DTid.x >= requestCount)
		return;
	
	// エミッター総数がDTid.xより小さければ return
	uint _emitterIndex = DTid.x;
	EmitData _emitInfo = g_emitData[_emitterIndex];

	// エミッターが要求する個数分だけパーティクルを発生させる
	for (uint _i = 0; _i < _emitInfo.emitCount; ++_i)
	{
		uint _origCount;

		// カウンターから１引いて、引く前の数を origCount に取得する（アトミック演算）
		InterlockedAdd(g_counterBuffer[0], -1, _origCount);

		// 空きがあった場合 : 引く前の数が１以上なら
		if (_origCount > 0)
		{
			// デッドリストの末尾から、空いているパーティクルのインデックス番号を取得
			uint _newIndex = g_deadList[_origCount - 1];

			// 新しいパーティクルデータを初期化してプールに書き込む
			ParticleData _p;
			_p.pos = _emitInfo.pos;
			_p.life = 1.0f;
			_p.velocity = _emitInfo.emitDirection; // 初速
			_p.size = _emitInfo.baseScale;

			g_particleBuffer[_newIndex] = _p;
		}
		// 空きがなかった場合 : 最大容量に達している場合
		else
		{
			// カウンターのマイナスを戻す
			InterlockedAdd(g_counterBuffer[0], 1, _origCount);

			// これ以上は出せないためプールから抜ける
			break;
		}
	}

}
