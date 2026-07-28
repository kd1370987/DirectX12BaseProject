
#include "../../../Common/RootParameters/Particle.hlsli"
#include "../../../Common/Math/Hash.hlsli"

// ルートシグネチャ
#define EMITPARTICLE_ROOT_SIG \
	"RootFlags(0),"\
	"CBV(b0),"\
	"DescriptorTable(SRV(t0,numDescriptors=1)),"\
	"DescriptorTable(UAV(u0,numDescriptors=3))"

cbuffer EmitCB : register(b0)
{
	uint requestCount;		// 今回発生するエミット命令の数
	uint frameSeed;			// フレームごとに変わる乱数の種
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
		InterlockedAdd(g_counterBuffer[0], (uint) - 1, _origCount);

		// 空きがあった場合 : 引く前の数が１以上なら
		// ★カウンターは uint。空きが0の状態で複数スレッドが同時に引くと、
		//   2番目以降は 0xFFFFFFFF(=符号付きなら-1) へラップする。
		//   unsigned のまま > 0 判定すると、この巨大値を「空きあり」と誤認して
		//   デッドリストを範囲外参照し、しかもカウンターを復元しないため
		//   カウンターが永久破壊され、以降エミットが止まる。
		//   符号付きで判定すれば、引きすぎたスレッドは必ず else 側で復元され、
		//   カウンターは最終的に 0 へ収束する（自己修復する）。
		if ((int) _origCount > 0)
		{
			// デッドリストの末尾から、空いているパーティクルのインデックス番号を取得
			uint _newIndex = g_deadList[_origCount - 1];

			// 新しいパーティクルデータを初期化してプールに書き込む
			ParticleData _p;

			// シード値取得
			// ・frameSeed を混ぜないと毎フレーム完全に同じパーティクルが生成され、
			//   すべて同じ位置に重なって「1個しか出ていない」ように見える。
			// ・単純な足し算だと (スレッド0,_i=1) と (スレッド1,_i=0) が同じ種になるので
			//   ハッシュを噛ませてから組み合わせる。
			uint _seed = PCGHash(PCGHash(frameSeed + _emitterIndex * 9781u) + _i);

			// 発射位置計算
			// ※ 要素ごとに種を進めること。同じ種を使い回すと
			//    寿命・速度・スケールがすべて同じ乱数値になる。
			float _radius = Random(_seed++) * _emitInfo.positionRadius;
			float3 _offset = RandomDirection(_seed) * _radius;
			_seed += 2;		// RandomDirection は内部で種を2つ消費する
			_p.pos = _emitInfo.pos + _offset;

			// 生存時間
			_p.life = ValueFloat(_emitInfo.minLifeTime, _emitInfo.maxLifeTime, _seed++);

			// ★寿命が 0 以下だと Update 側の「死亡済み」ガードに即座に弾かれ、
			//   デッドリストへ返却されずスロットが永久リークする。
			//   （ライフタイム未設定 min=max=0 などで起きる）
			//   必ず正の最低寿命を保証し、たとえ即死でも次フレームに返却されるようにする。
			_p.life = max(_p.life, 0.0001f);

			// 発射方向計算
			float3 _forward = normalize(_emitInfo.emitDirection);
			float3 _coneDir = RandomConeDirection(_forward, _emitInfo.directionAngle, _seed);
			_seed += 2;		// RandomConeDirection も内部で種を2つ消費する
			_p.velocity = _coneDir * ValueFloat(_emitInfo.minSpeed, _emitInfo.maxSpeed, _seed++);

			// スケール
			_p.size = _emitInfo.baseScale * ValueFloat(_emitInfo.minScale, _emitInfo.maxScale, _seed++);

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
