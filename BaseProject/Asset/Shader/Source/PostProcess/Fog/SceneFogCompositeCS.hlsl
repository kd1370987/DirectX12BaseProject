//==========================================================================================
//
// SceneFogCompositeCS
//
// メインカラーへ、SceneVolumetricFogCS が書いたフォグを重ねる。
//
//   出力 = lerp(メインカラー, フォグの色, フォグの濃さ * intensity)
//
// トーンマップ前のHDR段階で重ねる(フォグの色もHDRのまま扱う)。
// 無効時はメインカラーをそのまま素通しするので、絵は変わらない。
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            合成の設定
//   1 : SRVの番号(t0-t1) メインカラー + フォグ
//   2 : UAVの番号(u0)    合成結果
//==========================================================================================
#define SCENE_FOG_COMPOSITE_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=2, b100), " \
"RootConstants(num32BitConstants=1, b101), " \
RS_STATIC_SAMPLER_CLAMP

// 合成の設定
// ※ CPU 側 SceneFogCompositePass::CompositeCB と並びを合わせること
struct SceneFogCompositeData
{
	float intensity;	// フォグの濃さに掛ける倍率
	int enable;			// 0 なら重ねずにそのまま通す
	float2 pad0;
};

cbuffer CBSceneFogComposite : register(b0)
{
	SceneFogCompositeData g_composite;
}

// 入力
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_colorTexIndex;
	uint g_fogTexIndex;
}

Texture2D<float4> Get_colorTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_colorTexIndex]; return _r; }	// メインカラー
#define g_colorTex Get_colorTex()
Texture2D<float4> Get_fogTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_fogTexIndex]; return _r; }		// フォグ(rgb = 色 / a = 濃さ)
#define g_fogTex Get_fogTex()

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outTexIndex;
}

RWTexture2D<float4> Get_outTex() { RWTexture2D<float4> _r = ResourceDescriptorHeap[g_outTexIndex]; return _r; }
#define g_outTex Get_outTex()

// サンプラー
SamplerState g_samp : register(s0);

[RootSignature(SCENE_FOG_COMPOSITE_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力画像の解像度を取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外チェック
	if (DTid.x >= _width || DTid.y >= _height) return;

	float4 _mainColor = g_colorTex[DTid.xy];

	// 無効ならそのまま通す
	if (g_composite.enable == 0)
	{
		g_outTex[DTid.xy] = _mainColor;
		return;
	}

	// フォグはUVで引く : フォグだけ縮小解像度で回しても合わせられるように
	float2 _uv = (float2(DTid.xy) + 0.5f) / float2(_width, _height);
	float4 _fog = g_fogTex.SampleLevel(g_samp, _uv, 0);

	float _amount = saturate(_fog.a * g_composite.intensity);

	// アルファはメインカラーのものを保つ(後段のUIやコピーが見ている)
	g_outTex[DTid.xy] = float4(lerp(_mainColor.rgb, _fog.rgb, _amount), _mainColor.a);
}
