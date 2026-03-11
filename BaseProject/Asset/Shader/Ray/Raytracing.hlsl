// カメラの定数バッファ
cbuffer cbCamera : register(b0)
{
	float4x4 viewMat;		// ビュー行列
	float4x4 viewInvMat;	// ビュー行列
	float4x4 projMat;		// 投影行列
	float4x4 projInvMat;	// 投影行列の逆行列

	float4 cameraPos;		// カメラ位置
}

RaytracingAccelerationStructure g_raytracingWorld : register(t0);		// レイトレワールド
