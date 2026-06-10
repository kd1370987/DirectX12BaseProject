// カメラの定数バッファ
//struct Camera
//{
//	float3 pos;						// カメラ座標
//	float pad;						

//	float4x4 view;					// ビュー行列
//	float4x4 proj;					// プロジェクション行列

//	float4x4 invView;				// 逆ビュー行列
//	float4x4 invProj;				// 逆プロジェクション行列

//	float4x4 invViewProj;			// 逆ビュープロジェクション行列
//};
//cbuffer cbCamera : register(b0)
//{
//	Camera g_camera;
//}

#include "../Common/CB/CBCamera.hlsli"
RaytracingAccelerationStructure g_raytracingWorld : register(t0);	// レイトレワールド
RWTexture2D<float4> gOutPut : register(u0);							// カラー出力先
sampler gSamp : register(s0);
// 定数
static const float PI = 3.1415926f;

// 補助関数
// ある法線ベクトルに対して必ず直行するベクトルを作る
float3 AnyPerpendicular(float3 a_n)
{
	return (abs(a_n.x) > 0.9f) ? cross(a_n, float3(0, 1, 0)) : cross(a_n, float3(1, 0, 0));
}

// 法線方向を中心に半球の中からランダムに方向を返す
// サーフェイスノーマルを上向きとした半球サンプリング
float3 SampleHemisphereCosine(float3 a_normal,float2 a_rand)
{
	float _phi = 2.0f * PI * a_rand.y;
	float _cosTheta = sqrt(1.0f - a_rand.x);
	float _sinTheta = sqrt(a_rand.x);

	float3 _localDir;
	_localDir.x = cos(_phi) * _sinTheta;
	_localDir.y = sin(_phi) * _sinTheta;
	_localDir.z = _cosTheta;

	float3 _T = normalize(AnyPerpendicular(a_normal));
	float3 _B = cross(a_normal,_T);

	// TBNを作成して返す
	return normalize(
		_T * _localDir.x + 
		_B * _localDir.y + 
		a_normal * _localDir.z
	);
}

// ハッシュ関数
uint Hash(uint a_x)
{
	a_x ^= a_x >> 16;
	a_x *= 0x7feb352d;
	a_x ^= a_x >> 15;
	a_x *= 0x846ca68b;
	a_x ^= a_x >> 16;
	return a_x;
}
// ランダム取得関数
float Random01(uint a_seed)
{
	return Hash(a_seed) / 4294967296.0;
}
