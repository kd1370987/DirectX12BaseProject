// カメラ行列一式。
// ※ CPU 側 Engine::Graphics::CameraData と並びを合わせること
#ifndef ROOTPARAM_CAMERA_DATA_HLSLI
#define ROOTPARAM_CAMERA_DATA_HLSLI

struct CameraData
{
	// 現在フレーム。proj 系にはTAAのジッターが入っている
	float4x4 view;
	float4x4 proj;
	float4x4 invView;
	float4x4 invProj;
	float4x4 viewProj;
	float4x4 invViewProj;

	// ジッターなし。モーションベクターはこちらで求める
	// (ジッター込みで求めると速度に毎フレームの揺らしが混ざり、TAAが二重に補正してしまう)
	float4x4 nonJitteredProj;
	float4x4 nonJitteredViewProj;
	float4x4 nonJitteredInvViewProj;

	// 1フレーム前
	float4x4 prevView;
	float4x4 prevProj;
	float4x4 prevViewProj;

	float4 cameraPos;

	float2 jitterOffset;
	float2 prevJitterOffset;

	// 視錐台6面(ワールド空間)。xyz = 法線 / w = 原点からの距離
	float4 frustumPlanes[6];
};

#endif
