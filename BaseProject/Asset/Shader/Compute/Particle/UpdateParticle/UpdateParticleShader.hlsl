#include "../../../Common/RootParameters/Particle.hlsli"

// ルートシグネチャ
#define UPDATEPARTICLE_ROOT_SIG \
"RootFlags(0),"\
RS_PARTICLE_TABLE

// ルートシグネチャセット
[RootSignature(UPDATEPARTICLE_ROOT_SIG)]

// １スレッド当たり
[numthreads(32, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// エミッター総数がDTid.xより小さければ return
	uint _emitterIndex = DTid.x;
	EmitData _emitInfo = g_emitData[_emitterIndex];

	// パーティクルデータをみて寿命が尽きていればデッドリストに返す
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
			_p.life = _emitInfo.lifeTime;
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
