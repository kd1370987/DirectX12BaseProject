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
#include "../../../Source/RootSignatureLayout.hlsli"

#include "../../../Common/CB/CBCamera.hlsli"
#include "../../../Common/CB/CBDoFOption.hlsli"

// ルートシグネチャデータ
#define COC_ROOT_SIG \
"RootFlags(0)," \
RS_CAMERA_CB "," \
RS_DOF_OPTION_CB "," \
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_STATIC_SAMPLER

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
	// 空はボカさない
	//
	// 空はメッシュを持たないので深度が far(1.0)のまま残る。そのまま距離で判定すると
	// 「一番遠いもの」として最大の奥ボケが掛かり、空だけがべったり滲む。
	// レンズの話としても、無限遠へピントを合わせていない限り空は必ずボケる……のだが、
	// 絵として欲しいのは「手前の被写体が浮き上がること」であって空の滲みではない。
	//
	// CoC を 0 にしておけば、DoF パス側のギャザーはこの画素を混ぜに来ない。
	// 手前のボケた被写体が空へ滲み出すほうは、あちらがサンプル自身の CoC で
	// 重み付けしているのでこれまでどおり効く(輪郭が痩せることはない)。
	//--------------------------------------------------------------------------------------
	if (_depth >= 0.999999f)
	{
		g_outputCoC[_coord] = 0.0f;
		return;
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

	g_outputCoC[_coord] = _coc;
}
