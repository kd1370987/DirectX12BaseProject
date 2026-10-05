#include "../../../Common/RootParameters/Particle.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            更新ディスパッチの設定
//   1 : SRVの番号        発生命令の一覧 + 発生源の席
//   2 : UAVの番号        粒 + デッドリスト + カウンター + 生存リスト + 間接描画の引数
//==========================================================================================
#define UPDATEPARTICLE_ROOT_SIG \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
	"CBV(b0)," \
	"RootConstants(num32BitConstants=2, b100),"\
	"RootConstants(num32BitConstants=5, b101)"

cbuffer CBParticleUpdate : register(b0)
{
	ParticleUpdateSetting g_update;
}

// 入力
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_emitDataIndex;
	uint g_emitterSlotIndex;
}

StructuredBuffer<EmitData> Get_emitData() { StructuredBuffer<EmitData> _r = ResourceDescriptorHeap[g_emitDataIndex]; return _r; }
#define g_emitData Get_emitData()

StructuredBuffer<EmitterTransform> Get_emitterSlots() { StructuredBuffer<EmitterTransform> _r = ResourceDescriptorHeap[g_emitterSlotIndex]; return _r; }
#define g_emitterSlots Get_emitterSlots()

// 入出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_particleBufferIndex;
	uint g_deadListIndex;
	uint g_counterBufferIndex;
	uint g_aliveListIndex;		// 生き残った粒の番号を積む先(生存リスト)
	uint g_drawArgsIndex;		// 間接描画の引数。[1](InstanceCount)で生き残りを数える
}

RWStructuredBuffer<ParticleData> Get_particleBuffer() { RWStructuredBuffer<ParticleData> _r = ResourceDescriptorHeap[g_particleBufferIndex]; return _r; }
#define g_particleBuffer Get_particleBuffer()
RWStructuredBuffer<uint> Get_deadList() { RWStructuredBuffer<uint> _r = ResourceDescriptorHeap[g_deadListIndex]; return _r; }
#define g_deadList Get_deadList()
RWStructuredBuffer<uint> Get_counterBuffer() { RWStructuredBuffer<uint> _r = ResourceDescriptorHeap[g_counterBufferIndex]; return _r; }
#define g_counterBuffer Get_counterBuffer()
RWStructuredBuffer<uint> Get_aliveList() { RWStructuredBuffer<uint> _r = ResourceDescriptorHeap[g_aliveListIndex]; return _r; }
#define g_aliveList Get_aliveList()
RWStructuredBuffer<uint> Get_drawArgs() { RWStructuredBuffer<uint> _r = ResourceDescriptorHeap[g_drawArgsIndex]; return _r; }
#define g_drawArgs Get_drawArgs()

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
	_p.life -= g_update.deltaTime;								// 寿命を減らす

	//----------------------------------------------------------------
	// 重力を加える
	//
	// 重力はワールドの下向き。ローカル空間で回している粒は席(発生源)の座標系で
	// 速度を持っているので、ワールドの重力を席の回転の逆で戻してから足す。
	// 席の行列は拡縮を落としてあるので、回転の逆は転置で済む。
	// 席 0 は単位行列なので、ワールド空間の粒はそのまま素通りする
	//----------------------------------------------------------------
	uint _slotCount, _slotStride;
	g_emitterSlots.GetDimensions(_slotCount, _slotStride);
	const uint _slotIndex = min(_p.emitterIndex, _slotCount - 1);		// 範囲外は読まない(ふつうは起きない)

	const float3x3 _slotRot = (float3x3) g_emitterSlots[_slotIndex].worldMat;
	_p.velocity += mul(g_update.gravity, transpose(_slotRot)) * g_update.deltaTime;

	// 空気抵抗 : 勢いよく飛び出して失速する動きを作る。
	// 爆発の破片や煙は「初速だけ速い」ので、これが無いと最後まで等速で飛んでいってしまう。
	// フレームレートが変わっても減り方が同じになるよう指数で落とす
	// (1 - drag*dt の掛け算だと dt が大きいフレームで減りすぎる)
	if (g_update.drag > 0.0f)
	{
		_p.velocity *= exp(-g_update.drag * g_update.deltaTime);
	}

	_p.pos += _p.velocity * g_update.deltaTime;		// 座標を更新
	_p.rotation += _p.angularVelocity * g_update.deltaTime;	// 板を面の中で回す

	// NaN/Inf 対策。
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

	//----------------------------------------------------------------
	// 生き残った粒を描く一覧(生存リスト)へ積む
	//
	// 描画は生きている粒の数ぶんだけ走る(ExecuteIndirect)。その数を GPU のここで数える。
	// InstanceCount(引数の[1])を1つ進めて自分の枠を取る(戻り値は進める前の値 = 自分の枠の番号)。
	// このフレームで寿命が尽きた粒は上で life = 0 にしてあるので積まれない。
	// InstanceCount はフレームの頭でリセット用の CS が 0 にしている
	//----------------------------------------------------------------
	if (_p.life > 0.0f)
	{
		uint _slot;
		InterlockedAdd(g_drawArgs[1], 1, _slot);

		// 生きている粒は容量を超えないが、壊れていても範囲外は書かない
		if (_slot < _maxCapacity)
		{
			g_aliveList[_slot] = _particleIndex;
		}
	}

	// 更新したデータをVRAMに書き戻す
	g_particleBuffer[_particleIndex] = _p;
}
