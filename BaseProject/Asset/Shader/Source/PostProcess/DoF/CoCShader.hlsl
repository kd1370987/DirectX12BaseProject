//==========================================================================================
//
// CoCShader
//
// 深度バッファから CoC(Circle of Confusion : 錯乱円 = ボケの大きさ)を作る。
// 出力は1チャンネル。符号でボケの向きを持たせている。
//
//    -1 …… 手前側の最大ボケ
//     0 …… ピントが合っている
//    +1 …… 奥側の最大ボケ
//
// 実際のボケ半径(ピクセル)は DoF パス側で abs(CoC) * maxBlurRadius として使う。
// 符号を残しておくと「手前ボケか奥ボケか」を後段で判定できる。
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/DoFOptionData.hlsli"
#include "../../../Common/RootParameters/SkyData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)         カメラ
//   1 : CBV(b12)        被写界深度の設定
//   2 : SRVテーブル(t0) 深度
//   3 : UAVテーブル(u0) CoC
//   4 : CBV(b15)        スカイ設定(空にボケを掛けるかの判定に使う)
//
// スカイ設定は末尾へ足してある。間に挟むと既存の番号が全部ずれる
//==========================================================================================
#define COC_ROOT_SIG \
"RootFlags(0)," \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b12, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
"CBV(b15, visibility = SHADER_VISIBILITY_ALL)," \
RS_STATIC_SAMPLER

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBDoFOption : register(b12)
{
	DoFOptionData g_dof;
}

cbuffer CBSky : register(b15)
{
	SkyData g_sky;
}

// 入力
Texture2D<float> g_depthTex : register(t0);		// 深度

// 出力 : CoC(1チャンネル)
RWTexture2D<float> g_outputCoC : register(u0);

// サンプラー
SamplerState g_samp : register(s0);

// 深度からビュー空間の位置を復元する
float3 ReconstructViewPos(float2 a_uv, float a_depth)
{
	float4 _clip = float4(a_uv.x * 2.0f - 1.0f, 1.0f - a_uv.y * 2.0f, a_depth, 1.0f);
	float4 _view = mul(_clip, g_camera.invProj);
	return _view.xyz / _view.w;
}

[RootSignature(COC_ROOT_SIG)]

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 画像の解像度を取得
	uint _width, _height;
	g_outputCoC.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height)
		return;

	int2 _coord = int2(DTid.xy);

	// 無効なら 0(ボケなし)で埋めておく。後段が参照しても安全なようにクリアはしておく
	if (g_dof.enable == 0)
	{
		g_outputCoC[_coord] = 0.0f;
		return;
	}

	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);
	float _depth = g_depthTex.Load(int3(_coord, 0)).r;

	//--------------------------------------------------------------------------------------
	// 空の扱い
	//
	// 空はメッシュを持たないので深度が far(1.0)のまま残る。そのまま距離で判定すると
	// 「一番遠いもの」として最大の奥ボケが掛かり、空だけがべったり滲む。
	// レンズの話としても、無限遠へピントを合わせていない限り空は必ずボケる……のだが、
	// 絵として欲しいのは「手前の被写体が浮き上がること」であって空の滲みではない。
	//
	// どちらが欲しいかはシーン次第なので、シーンのアンビエント設定
	// (SceneAmbientObject → SkyData)から受け取って切り替える。
	//   isSkyDof = 0 … 空だけ CoC を 0 にして素通し
	//   isSkyDof = 1 … 通常どおり計算したうえで dofScale を掛ける
	//
	// CoC を 0 にしておけば、DoF パス側のギャザーはこの画素を混ぜに来ない。
	// 手前のボケた被写体が空へ滲み出すほうは、あちらがサンプル自身の CoC で
	// 重み付けしているのでこれまでどおり効く(輪郭が痩せることはない)。
	//--------------------------------------------------------------------------------------
	float _skyCoCScale = 1.0f;
	if (_depth >= 0.999999f)
	{
		if (g_sky.isSkyDof == 0)
		{
			g_outputCoC[_coord] = 0.0f;
			return;
		}

		_skyCoCScale = g_sky.dofScale;
	}

	// 深度 → カメラからの深度(ビュー空間Z)
	float _viewDepth = ReconstructViewPos(_uv, _depth).z;

	// ピントが合う範囲
	float _focusHalf  = g_dof.focusRange * 0.5f;
	float _focusStart = g_dof.focusDistance - _focusHalf;	// これより手前はボケ始める
	float _focusEnd   = g_dof.focusDistance + _focusHalf;	// これより奥はボケ始める

	float _coc = 0.0f;

	if (_viewDepth < _focusStart)
	{
		// 手前側 : nearRange 進むと -1(最大ボケ)
		// 幅が 0 のときは境界を越えた瞬間に最大ボケ
		float _range = g_dof.nearRange;
		_coc = (_range > 1e-4f) ? -saturate((_focusStart - _viewDepth) / _range) : -1.0f;
	}
	else if (_viewDepth > _focusEnd)
	{
		// 奥側 : farRange 進むと +1(最大ボケ)
		float _range = g_dof.farRange;
		_coc = (_range > 1e-4f) ? saturate((_viewDepth - _focusEnd) / _range) : 1.0f;
	}

	// 空のときだけ倍率が掛かる(それ以外は 1.0 なので素通り)
	g_outputCoC[_coord] = _coc * _skyCoCScale;
}
