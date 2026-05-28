// インクルードガード
#ifndef CB_CAMERA_HLSLI
#define CB_CAMERA_HLSLI

struct CBCamera
{
	float3 cameraPos; // カメラ位置
	float pad;
	
	// 現在フレームのデータ
	float4x4 view;		// ビュー行列
	float4x4 proj;		// 投影行列
	float4x4 invView;	// ビュー行列
	float4x4 invProj;	// 投影行列の逆行列

	// １フレーム前のデータ
	float4x4 prevView;
	float4x4 prevProj;
	float4x4 prevViewProj;
};

// カメラの定数バッファ
cbuffer camera : register(b0)
{
	CBCamera g_camera;
}

#endif

// 共通定数バッファ
#define RS_CAMERA_CB "CBV(b0,visibility = SHADER_VISIBILITY_ALL)"
