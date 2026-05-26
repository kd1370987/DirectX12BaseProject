// カメラの定数バッファ
struct Camera
{
	float3 pos;						// カメラ座標
	float pad;						

	float4x4 view;					// ビュー行列
	float4x4 proj;					// プロジェクション行列

	float4x4 invView;				// 逆ビュー行列
	float4x4 invProj;				// 逆プロジェクション行列

	float4x4 invViewProj;			// 逆ビュープロジェクション行列
};
cbuffer cbCamera : register(b0)
{
	Camera g_camera;
}

RaytracingAccelerationStructure g_raytracingWorld : register(t0);	// レイトレワールド
RWTexture2D<float4> gOutPut : register(u0);							// カラー出力先
sampler gSamp : register(s0);

