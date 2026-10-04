//==========================================================================================
//
// GroundFieldCS
//
// 衝撃(GroundImpulse)が地面のチリをどう動かしたかを、真上から見たテクスチャへ書く。
// カメラを中心にした GROUND_FIELD_WORLD_SIZE (m) 四方を xz で並べたもの。
//   r = 払われずに残ったチリの量(1 = 手つかず。衝撃が重なると掛け合わせで減る)
//   g = 波頭に寄せられたチリの量(衝撃が重なると足し合わせ)
//
// 衝撃の数だけ回す計算をここで1テクセル1回に済ませておき、
// SceneVolumetricFogCS はレイの1歩ごとにこのテクスチャを1回引くだけにする。
//
// 震源からの距離は水平(xz)で測る。波の輪は地面の起伏へ真上から投影した形になる
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/GroundFieldData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ(フィールドの中心を決める)
//   1 : CBV(b1)            グラウンドフィールドの定数(経過時間・衝撃の数)
//   2 : UAVの番号(u0)    フィールド(レンダーグラフが張る)
//   3 : SRVの番号(t0)    衝撃の配列(GraphicsEngine が詰めたもの)
//
// 追加は必ず末尾へ足すこと。間に挟むと既存の番号が全部ずれる
//==========================================================================================
#define GROUND_FIELD_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=1, b100), " \
"RootConstants(num32BitConstants=1, b101)"

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBGroundField : register(b1)
{
	GroundFieldData g_groundField;
}

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_outTexIndex;
}

RWTexture2D<float2> Get_outTex() { RWTexture2D<float2> _r = ResourceDescriptorHeap[g_outTexIndex]; return _r; }	// フィールド
#define g_outTex Get_outTex()

// 衝撃の配列 : GraphicsEngine が毎フレーム詰め直したもの。
// 要素数は g_groundField.impulseCount
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_impulsesIndex;
}

StructuredBuffer<GroundImpulse> Get_impulses() { StructuredBuffer<GroundImpulse> _r = ResourceDescriptorHeap[g_impulsesIndex]; return _r; }
#define g_impulses Get_impulses()

[RootSignature(GROUND_FIELD_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力画像の解像度を取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外チェック
	if (DTid.x >= _width || DTid.y >= _height) return;

	// このテクセルが指す水平位置(テクセルの中心)
	const float2 _center = CalcGroundFieldCenter(g_camera.cameraPos.xyz, (float) _width);
	const float2 _uv = (float2(DTid.xy) + 0.5f) / float2(_width, _height);
	const float2 _posXZ = GroundFieldUVToWorld(_uv, _center);

	// 衝撃がチリをどう動かしたか
	float _remain = 1.0f;
	float _pile = 0.0f;
	for (uint _i = 0; _i < g_groundField.impulseCount; ++_i)
	{
		GroundImpulse _impulse = g_impulses[_i];
		float _distance = distance(_posXZ, _impulse.pos.xz);

		_remain *= 1.0f - CalcGroundImpulseSweep(_impulse, _distance);
		_pile += CalcGroundImpulseWave(_impulse, _distance);
	}

	g_outTex[DTid.xy] = float2(_remain, _pile);
}
