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
float3 ACESFilm(float3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
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
