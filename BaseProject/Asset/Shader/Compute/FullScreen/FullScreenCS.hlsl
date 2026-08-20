// ==========================================================================================
// フルスクリーン提示パス(コンピュート版)
//
// TAA後の最終カラー(HDR)にACESフィルミックトーンマッピングを適用し、
// 提示用のUAVテクスチャへ書き出す。
// 以前は SV_VertexID でフルスクリーン三角形を描くVS + トーンマップPS だったが、
// コンピュートシェーダーに置き換えた。1:1解像度なのでフィルタは不要で、Loadで直接読む。
// ==========================================================================================

// ルートシグネチャ
//   param0 : SRV テーブル (t0) … 入力カラー(AfterTAAColor)
//   param1 : UAV テーブル (u0) … 出力カラー(PresentColor)
#define FULLSCREEN_ROOT_SIG \
"RootFlags(0)," \
"DescriptorTable(SRV(t0, numDescriptors=1))," \
"DescriptorTable(UAV(u0, numDescriptors=1))"

Texture2D<float4>   g_input  : register(t0);	// トーンマップ前のHDRカラー
RWTexture2D<float4> g_output : register(u0);	// 提示用カラー

// ACESフィルミックトーンマッピング
//
// 彩度や色味が変わって見える、ハイライトを滑らかに圧縮
float3 ACESFilm(float3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Reinhardフィルター
//
// 白があまり白くならない、全体的に少し暗い、派手さが減る場合がある
float3 Reinhard(float3 a_color)
{
	return a_color / (1.0f + a_color);
}

// ExtendedReinhardフィルター : 白点を指定できる
float3 ReinhardExtended(float3 a_color, float a_whitePoint)
{
	return a_color * (1.0f + a_color / (a_whitePoint * a_whitePoint)) / (1.0f + a_color);

}

// Uncharted 2 Filmic
//
// ハイライトが自然、強いブルームと相性がいい
float3 Uncharted2Tonemap(float3 a_color)
{
	float _a = 0.15f;
	float _b = 0.50f;
	float _c = 0.10f;
	float _d = 0.20f;
	float _e = 0.02f;
	float _f = 0.30f;

	return ((a_color * (_a * a_color + _c * _b) + _d * _e) / (a_color * (_a * a_color + _b) + _d * _f)) - _e / _f;
}



[RootSignature(FULLSCREEN_ROOT_SIG)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力サイズを取得して画面外スレッドを早期リターン
	uint _width, _height;
	g_output.GetDimensions(_width, _height);
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _coord = int2(DTid.xy);

	// 入力カラーを取得してトーンマッピング
	float3 _color = g_input.Load(int3(_coord, 0)).rgb;
	_color = ACESFilm(_color);

	g_output[_coord] = float4(_color, 1.0);
}
