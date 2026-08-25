//==========================================================================================
//
// BloomExtractShader
//
// 川瀬式ブルームの入口。ディファードライティング後のHDRカラーから、
// しきい値を超えた高輝度成分だけを取り出す。
//
// しきい値でスパッと切ると、輝度がしきい値をまたぐ画素がフレーム間でチカチカするため、
// softKnee でしきい値付近を二次曲線でなめらかにつないでいる（softKnee = 0 でハードカット）。
//
// 出力はこの後 1/2 → 1/16 まで縮小しながらガウシアンブラーを掛けていくので、
// トーンマップ前のHDRレンジを保てるよう R16G16B16A16_FLOAT で受ける。
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/BloomOptionData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b13)        ブルーム設定
//   1 : SRVテーブル(t0) メインカラー(HDR)
//   2 : UAVテーブル(u0) 高輝度成分
//==========================================================================================
#define BLOOM_EXTRACT_RS \
"RootFlags(0)," \
"CBV(b13, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
"DescriptorTable(UAV(u0, numDescriptors=1))"

cbuffer CBBloomOption : register(b13)
{
	BloomOptionData g_bloom;
}

// 入力 : メインカラー（ディファードライティングの結果）
Texture2D<float4> g_colorTex : register(t0);

// 出力 : 高輝度成分
RWTexture2D<float4> g_outTex : register(u0);

[RootSignature(BLOOM_EXTRACT_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 画像の縦横サイズを取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height) return;

	// 無効なら黒で埋める
	// （後段のブラー・合成はそのまま走るが、足されるのは0なので絵は変わらない）
	if (g_bloom.enable == 0)
	{
		g_outTex[DTid.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
		return;
	}

	float3 _color = g_colorTex[DTid.xy].rgb;
	float _luminance = dot(_color, float3(0.2126f, 0.7152f, 0.0722f));

	// しきい値の境目をなめらかにする幅
	float _knee = max(g_bloom.threshold * g_bloom.softKnee, 1e-5f);

	// 境目付近（threshold ± knee）だけ二次曲線で立ち上げる
	float _soft = clamp(_luminance - g_bloom.threshold + _knee, 0.0f, _knee * 2.0f);
	_soft = (_soft * _soft) / (4.0f * _knee);

	// しきい値を超えたぶんが元の色に占める割合。
	// 色相を変えたくないので、RGBを一律この割合で減衰させる
	float _contribution = max(_soft, _luminance - g_bloom.threshold) / max(_luminance, 1e-5f);

	g_outTex[DTid.xy] = float4(_color * _contribution, 1.0f);
}
