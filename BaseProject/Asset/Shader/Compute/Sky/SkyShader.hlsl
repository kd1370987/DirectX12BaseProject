//==========================================================================================
//
// SkyShader
//
// 何も描かれていないピクセル(深度が far のまま残っているピクセル)を空で埋める。
//
// スカイドームのメッシュは置かない。代わりに、
//   1. そのピクセルがワールドのどちらを向いているかを深度バッファ抜きで復元し、
//   2. その視線を「仮想ドーム」(中心 = カメラのXZ・高さ horizonHeight、半径 radius)へ飛ばし、
//   3. 交点の方向を正距円筒(緯度経度)UVへ直してスカイテクスチャを引く
// という手順で色を決める。
//
// ドームを実際に置いたのと同じ見え方になるので、カメラが地平線の高さから離れると
// 地平線が上下に動く。半径が小さいほどその動きは強くなり、十分大きく取ると
// ほぼ無限遠のスカイボックスと同じになる。
//
// モーションベクターも書く。GBuffer を通らない以上ここで書かないと空の速度が
// 0 のままになり、カメラを振ったときに TAA が「動いていない」と判断して空が尾を引く。
// 空は無限遠として扱いたいので、方向ベクトル(w=0)を射影して平行移動成分を落とす。
//
//==========================================================================================
#include "../../Source/RootSignatureLayout.hlsli"
#include "../../Common/CB/CBCamera.hlsli"
#include "../../Common/CB/CBSky.hlsli"

// ルートシグネチャ
//   param0 : CBV (b0)            … カメラ
//   param1 : CBV (b15)           … スカイ設定
//   param2 : SRV テーブル (t0)   … 深度(レンダーグラフが張る)
//   param3 : SRV テーブル (t1)   … スカイテクスチャ(パスが張る)
//   param4 : UAV テーブル (u0)   … 出力カラー(AfterLighting / HDR)
//   param5 : UAV テーブル (u1)   … 出力モーションベクター(GBufferVelocity)
#define SKY_ROOT_SIG \
"RootFlags(0)," \
RS_CAMERA_CB "," \
RS_SKY_CB "," \
"DescriptorTable(SRV(t0, numDescriptors=1))," \
"DescriptorTable(SRV(t1, numDescriptors=1))," \
"DescriptorTable(UAV(u0, numDescriptors=1))," \
"DescriptorTable(UAV(u1, numDescriptors=1))," \
RS_STATIC_SAMPLER_SKY

Texture2D<float4>   g_depthTex    : register(t0);	// 深度
Texture2D<float4>   g_skyTex      : register(t1);	// スカイ(正距円筒)

RWTexture2D<float4> g_outColor    : register(u0);	// AfterLighting
RWTexture2D<float2> g_outVelocity : register(u1);	// GBufferVelocity

SamplerState g_samp : register(s0);

static const float PI = 3.14159265358979323846f;

//------------------------------------------------------------------------------------------
// このピクセルが見ているワールド方向
//
// 深度は使わない。far 面上の点を1つ復元して、カメラ位置からの向きを取るだけ。
// ジッターの掛かった invProj / invView を使う(GBufferと同じ揺らし方にしないと
// TAA が空だけ別の場所を見てしまう)
//------------------------------------------------------------------------------------------
float3 ReconstructViewDir(float2 a_uv)
{
	float4 _clip = float4(a_uv.x * 2.0f - 1.0f, 1.0f - a_uv.y * 2.0f, 1.0f, 1.0f);

	float4 _view = mul(_clip, g_camera.invProj);
	float3 _viewPos = _view.xyz / _view.w;

	float4 _world = mul(float4(_viewPos, 1.0f), g_camera.invView);
	float3 _worldPos = _world.xyz / _world.w;

	return normalize(_worldPos - g_camera.cameraPos.xyz);
}

//------------------------------------------------------------------------------------------
// 視線を仮想ドームへ飛ばし、ドーム中心から見た方向を返す
//
// ドームの中心はカメラの真下(または真上)にあるので、カメラから中心へのベクトルは
// (0, h, 0) だけになる。おかげで二次方程式が b = D.y * h / c = h^2 - R^2 まで潰れる。
//
// カメラがドームの外に出ている(|h| >= R)場合は交点が取れないので、
// そのときは視線の向きをそのまま使う(= 無限遠のスカイボックスとして扱う)
//------------------------------------------------------------------------------------------
float3 DomeDirection(float3 a_dir)
{
	const float _radius = max(g_sky.radius, 1e-3f);
	const float _h = g_camera.cameraPos.y - g_sky.horizonHeight;

	const float _b = a_dir.y * _h;
	const float _c = _h * _h - _radius * _radius;
	const float _disc = _b * _b - _c;

	if (_disc <= 0.0f) return a_dir;

	const float _t = -_b + sqrt(_disc);
	if (_t <= 0.0f) return a_dir;

	// 交点 - ドーム中心 = (t*D.x, h + t*D.y, t*D.z)
	return normalize(float3(_t * a_dir.x, _h + _t * a_dir.y, _t * a_dir.z));
}

//------------------------------------------------------------------------------------------
// 方向 → 正距円筒(緯度経度)UV
//
//   U : 方位角。テクスチャは一周してつながるのでサンプラーの WRAP に任せる
//   V : 天頂角。0 が真上、0.5 が地平線、1 が真下
//------------------------------------------------------------------------------------------
float2 DirectionToEquirectUV(float3 a_dir)
{
	const float _yaw = atan2(a_dir.x, a_dir.z);

	float2 _uv;
	_uv.x = _yaw / (2.0f * PI) + 0.5f + (g_sky.rotationDeg / 360.0f);
	_uv.y = acos(clamp(a_dir.y, -1.0f, 1.0f)) / PI;

	return _uv;
}

//------------------------------------------------------------------------------------------
// 空のモーションベクター
//
// w=0 で射影すると行列の平行移動成分が効かないので、そのまま
// 「無限遠にある点をカメラの回転だけで見比べる」ことになる。
// 出力の並びは GBuffer のモーションベクターに合わせる(UV空間・Y反転込み)
//------------------------------------------------------------------------------------------
float2 CalcSkyVelocity(float3 a_dir)
{
	float4 _curClip = mul(float4(a_dir, 0.0f), g_camera.viewProj);
	float4 _prevClip = mul(float4(a_dir, 0.0f), g_camera.prevViewProj);

	// 真横を向いた瞬間などに 0 除算しないよう保険を掛ける
	if (abs(_curClip.w) < 1e-6f || abs(_prevClip.w) < 1e-6f) return float2(0.0f, 0.0f);

	const float2 _curNDC = _curClip.xy / _curClip.w;
	const float2 _prevNDC = _prevClip.xy / _prevClip.w;

	return (_curNDC - _prevNDC) * float2(0.5f, -0.5f);
}

[RootSignature(SKY_ROOT_SIG)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	uint _width, _height;
	g_outColor.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height) return;

	const int2 _coord = int2(DTid.xy);

	// 何か描かれているピクセルは触らない。
	// 深度は 1.0(far)でクリアされているので、そのまま残っている所だけが空
	const float _depth = g_depthTex.Load(int3(_coord, 0)).r;
	if (_depth < 0.999999f) return;

	const float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);

	// 視線 → ドームの交点方向 → 正距円筒UV
	const float3 _dir = ReconstructViewDir(_uv);
	//const float3 _domeDir = DomeDirection(_dir);
	//const float2 _skyUV = DirectionToEquirectUV(_domeDir);
	const float2 _skyUV = DirectionToEquirectUV(_dir);

	// HDR のまま出す。1.0 を超えた分はブルームに乗り、最後にトーンマップで落ちる
	const float3 _sky = g_skyTex.SampleLevel(g_samp, _skyUV, 0).rgb * g_sky.exposure;

	g_outColor[_coord] = float4(_sky, 1.0f);
	g_outVelocity[_coord] = CalcSkyVelocity(_dir);
}
