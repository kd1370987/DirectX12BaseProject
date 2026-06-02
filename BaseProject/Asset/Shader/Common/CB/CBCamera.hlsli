// インクルードガード
#ifndef CB_CAMERA_HLSLI
#define CB_CAMERA_HLSLI

struct CBCamera
{	
	// 現在フレームのデータ
	float4x4 view;		// ビュー行列
	float4x4 proj;		// 投影行列			// ジッターあり
	float4x4 invView;	// ビュー行列
	float4x4 invProj;	// 投影行列の逆行列	// ジッターあり
	float4x4 invViewProj;					// ジッターあり

	// モーションベクター用
	float4x4 nonJitteredProj;		// ジッターなし 投影行列
	float4x4 nonJitteredViewProj;	// ジッターなし ViewProj

	// １フレーム前のデータ
	float4x4 prevView;
	float4x4 prevProj;
	float4x4 prevViewProj;

	// 補助用現在フレームデータ
	float4 cameraPos; // カメラ位置
	
	// TAA用ジッターオフセット
	float2 jitterOffset;
	float2 prevJitterOffset;
};

// カメラの定数バッファ
cbuffer camera : register(b0)
{
	CBCamera g_camera;
}

#endif

// 共通定数バッファ
#define RS_CAMERA_CB "CBV(b0,visibility = SHADER_VISIBILITY_ALL)"
