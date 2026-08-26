// ライト配列(StructuredBuffer)の1要素と、その要素数。
// ※ CPU 側 Engine::Graphics::DirectionalLight / PointLight と並びを合わせること。
//    StructuredBuffer は cbuffer と違って密に詰められるので、
//    C++ 側の並びをそのまま写せば一致する(パディングを足さないこと)
#ifndef ROOTPARAM_LIGHT_DATA_HLSLI
#define ROOTPARAM_LIGHT_DATA_HLSLI

// ディレクショナルライト
struct DirectionalLight
{
	float3 dir;			// 方向(光の進む向き)
	float brightness;	// color に掛ける強さ
	float4 color;		// 色
};

// ポイントライト
struct PointLight
{
	float3 pos;			// 座標(ワールド空間)
	float brightness;	// color に掛ける強さ
	float4 color;		// 色
	float range;		// 光の届く距離 : ここで減衰が0になる
};

// 主光源 : レイトレの影とGIが使う1つぶん。
//
// あちらは平行光へレイを1本しか飛ばさない(影マスクも1チャンネルしかない)ので、
// 配列ではなく先頭の1つだけをこの形で受け取る。
// ※ CPU 側 Engine::Graphics::SunLightCB と並びを合わせること。
//    cbuffer なので16バイト行に揃える
struct SunLightData
{
	float3 dir;			// 方向(光の進む向き)
	float brightness;	// color に掛ける強さ
	float4 color;		// 色
	uint enable;		// 0 なら平行光なし
	uint3 pad;
};

// 今フレームのライト数。
// StructuredBuffer は要素数を持たないので、ループの上限はこれをCBで受け取る。
// こちらは cbuffer なので16バイト行に揃えること
struct LightCountData
{
	uint directionalNum;
	uint pointNum;
	uint2 pad;
};

#endif
