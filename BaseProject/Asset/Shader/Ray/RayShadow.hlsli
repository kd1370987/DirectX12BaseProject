// ルートパラメター
#include "../Common/CB/CBCamera.hlsli"
#include "../Common/RootParameters/AmbientData.hlsli"

// 関数ヘルパー
#include "Raytracing.hlsli"
#include "../Source/CalcNormal.hlsli"

// レジスター登録
RaytracingAccelerationStructure g_raytracingWorld	: register(t0); // レイトレワールド
RWTexture2D<float4>				gOutPut				: register(u0); // カラー出力先
sampler							gSamp				: register(s0);
