// インクルードガード
#ifndef CB_CAMERA_HLSLI
#define CB_CAMERA_HLSLI

// カメラの定数バッファ
cbuffer camera : register(b0)
{
	float4x4 cView; // ビュー行列
	float4x4 cViewInv; // ビュー行列
	float4x4 cProj; // 投影行列
	float4x4 cProjInv; // 投影行列の逆行列

	float4 cCameraPos; // カメラ位置
}

#endif
