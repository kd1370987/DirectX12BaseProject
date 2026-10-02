//==========================================================================================
//
// GroundFieldCS
//
// GroundDepthPass が描いた地面だけの深度から、地面のワールド座標を復元して書き出す。
//   xyz = ワールド座標 / w = 地面があれば 1、無ければ 0
//
// 衝撃(GroundImpulse)と経過時間も受け取っているが、今は位置を戻して書くところまで。
// 波紋などの処理はこの後ろに足していく
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/GroundFieldData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ
//   1 : CBV(b1)            グラウンドフィールドの定数(経過時間・衝撃の数)
//   2 : SRVの番号(t0)    地面の深度(レンダーグラフが張る)
//   3 : UAVの番号(u0)    地面のワールド座標(レンダーグラフが張る)
//   4 : SRVの番号(t1)    衝撃の配列(GraphicsEngine が詰めたもの)
//
// 追加は必ず末尾へ足すこと。間に挟むと既存の番号が全部ずれる
//==========================================================================================
#define GROUND_FIELD_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=1, b100), " \
"RootConstants(num32BitConstants=1, b101), " \
"RootConstants(num32BitConstants=1, b102), " \
RS_STATIC_SAMPLER_CLAMP

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBGroundField : register(b1)
{
	GroundFieldData g_groundField;
}

// 入力
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_groundDepthTexIndex;
}

Texture2D<float> Get_groundDepthTex() { Texture2D<float> _r = ResourceDescriptorHeap[g_groundDepthTexIndex]; return _r; }	// 地面の深度
#define g_groundDepthTex Get_groundDepthTex()

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outTexIndex;
}

RWTexture2D<float4> Get_outTex() { RWTexture2D<float4> _r = ResourceDescriptorHeap[g_outTexIndex]; return _r; }	// 地面のワールド座標
#define g_outTex Get_outTex()

// 衝撃の配列 : GraphicsEngine が毎フレーム詰め直したもの。
// 要素数は g_groundField.impulseCount
cbuffer PassDescriptorIndex2 : register(b102)
{
	uint g_impulsesIndex;
}

StructuredBuffer<GroundImpulse> Get_impulses() { StructuredBuffer<GroundImpulse> _r = ResourceDescriptorHeap[g_impulsesIndex]; return _r; }
#define g_impulses Get_impulses()

// サンプラー
SamplerState g_samp : register(s0);

[RootSignature(GROUND_FIELD_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力画像の解像度を取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外チェック
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _coord = int2(DTid.xy);

	// 地面が描かれていないピクセルは「地面なし」(w = 0)
	float _depth = g_groundDepthTex.Load(int3(_coord, 0));
	if (_depth >= 1.0f)
	{
		g_outTex[_coord] = float4(0.0f, 0.0f, 0.0f, 0.0f);
		return;
	}

	// 3D空間での位置を復元
	// 画素の中心を指すUV(+0.5 を足さないと半画素ずれる)
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);
	float4 _clip = float4(_uv.x * 2.0f - 1.0f, 1.0f - _uv.y * 2.0f, _depth, 1.0f);
	float4 _worldPos4 = mul(_clip, g_camera.invViewProj);
	float3 _worldPos = _worldPos4.xyz / _worldPos4.w;

	// 衝撃伝搬を計算する
	float _field = 0.0f;
	for (uint _i = 0; _i < g_groundField.impulseCount; ++_i)
	{
		// 衝撃データ
		GroundImpulse _impulse = g_impulses[_i];

		// ピクセル座標と衝撃位置の距離から波の強さを求める
		_field += CalcGroundImpulseWave(_impulse, distance(_worldPos, _impulse.pos));
	}
	
	// 出力
	//g_outTex[_coord] = float4(_worldPos, 1.0f);

	// 出力 : テスト段階が終わればR16に変更予定
	g_outTex[_coord] = float4(_field, 0.0f, 0.0f, 1.0f);

}
