#include "../../../Common/RootParameters/Particle.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            更新ディスパッチの設定
//   1 : SRVテーブル(t0)    発生命令の一覧
//   2 : UAVテーブル(u0-u2) 粒 + デッドリスト + カウンター
//==========================================================================================
#define UPDATEPARTICLE_ROOT_SIG \
	"RootFlags(0)," \
	"CBV(b0)," \
	"DescriptorTable(SRV(t0,numDescriptors=1)),"\
	"DescriptorTable(UAV(u0,numDescriptors=3))"

cbuffer CBParticleUpdate : register(b0)
{
	ParticleUpdateSetting g_update;
}

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
	_p.life -= g_update.deltaTime;					// 寿命を減らす
	_p.velocity += g_update.gravity * g_update.deltaTime;		// 重力を減らす

	// 空気抵抗 : 勢いよく飛び出して失速する動きを作る。
	// 爆発の破片や煙は「初速だけ速い」ので、これが無いと最後まで等速で飛んでいってしまう。
	// フレームレートが変わっても減り方が同じになるよう指数で落とす
	// (1 - drag*dt の掛け算だと dt が大きいフレームで減りすぎる)
	if (g_update.drag > 0.0f)
	{
		_p.velocity *= exp(-g_update.drag * g_update.deltaTime);
	}

	_p.pos += _p.velocity * g_update.deltaTime;		// 座標を更新

	// ★NaN/Inf 対策。
	// NaN はあらゆる比較が false になるため、上の life<=0 も下の返却判定もすり抜け、
	// 永久に生き続けてスロットを占有し続ける(デッドリストへ返却されない)。
	// 一度でも混入すると空きが減りっぱなしになるので、ここで死亡扱いにして回収する。
	if (!(_p.life > 0.0f))
	{
		_p.life = 0.0f;
	}

	// デッドリストへの返却
	if(_p.life <= 0.0f)
	{
		uint _count;

		// カウンターを１増やし、増やす前の値取得
		InterlockedAdd(g_counterBuffer[0], 1, _count);

		// デッドリストに返却
		// 正常時は _count < 容量 だが、万一の二重返却などで容量を超えると
		// デッドリストを範囲外書き込みして隣接バッファを破壊するため防ぐ。
		// 超えた場合は増やしたカウンターも戻し、カウンターが容量を超えないようにする
		// (超えると Emit 側が範囲外のデッドリストを読み、無効なインデックスを掴む)。
		if (_count < _maxCapacity)
		{
			g_deadList[_count] = _particleIndex;
		}
		else
		{
			InterlockedAdd(g_counterBuffer[0], (uint) - 1);
		}
	}

	// 更新したデータをVRAMに書き戻す
	g_particleBuffer[_particleIndex] = _p;
}
