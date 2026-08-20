
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

			// 新しいパーティクルデータを初期化してプールに書き込む。
			// ★構造体の全メンバーを埋めること。1つでも未初期化のまま UAV へ書くと
			//   DXC の検証が "Assignment of undefined values to UAV" で落ちる
			ParticleData _p;
			_p.pad = float2(0.0f, 0.0f);

			// どの発生源の座標系で回るか。ワールド空間なら 0(単位行列)
			_p.emitterIndex = _emitInfo.emitterIndex;

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

			// ★寿命に沿った変化(サイズ・色・フェード)の分母。
			//   寿命は粒ごとにランダムなので、発生時の値を覚えておかないと
			//   「今どこまで進んだか」が出せない。
			_p.startLife = _p.life;

			// 発射方向計算
			//
			//   Cone       : 指定方向を軸にした円錐。噴射・排気のように向きがあるもの
			//   Sphere     : 中心から全方向。爆発のように四方八方へ飛び散るもの
			//   Hemisphere : 指定方向側の半球だけ。地面での爆発など、下へ潜らせたくないもの
			//
			// ※ Cone の角度を 360 度にしても全方向にはならない。
			//    円錐の「半頂角」なので全方向にしたければ 180 度で、
			//    しかもその極限は分布が偏る。全方向は専用の分岐で出すこと。
			float3 _forward = normalize(_emitInfo.emitDirection);
			float3 _emitDir;

			if (_emitInfo.emitShape == PARTICLE_EMIT_SHAPE_SPHERE)
			{
				_emitDir = RandomDirection(_seed);
				_seed += 2;		// RandomDirection は内部で種を2つ消費する
			}
			else if (_emitInfo.emitShape == PARTICLE_EMIT_SHAPE_HEMISPHERE)
			{
				// 全方向のうち、基準方向の裏側へ出たものだけ折り返す。
				// コーンの角度を90度にするより分布が素直になる
				float3 _randomDir = RandomDirection(_seed);
				_seed += 2;
				_emitDir = (dot(_randomDir, _forward) < 0.0f) ? -_randomDir : _randomDir;
			}
			else
			{
				_emitDir = RandomConeDirection(_forward, _emitInfo.directionAngle, _seed);
				_seed += 2;		// RandomConeDirection も内部で種を2つ消費する
			}

			_p.velocity = _emitDir * ValueFloat(_emitInfo.minSpeed, _emitInfo.maxSpeed, _seed++);

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
