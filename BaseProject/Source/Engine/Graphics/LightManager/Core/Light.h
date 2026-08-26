#pragma once

// GPU の StructuredBuffer へそのまま流す構造体。
// ※ HLSL 側 Asset/Shader/Common/RootParameters/LightData.hlsli と並びを合わせること。
//    StructuredBuffer は cbuffer と違って密に詰められるので、C++ の並びをそのまま写せば一致する。
//    float3 の後ろに float を置いて16バイト行に揃えてあるのは、後で cbuffer へ移しても
//    壊れないようにするための保険
namespace Engine::Graphics
{
	// ディレクショナルライト
	struct DirectionalLight
	{
		Math::Vector3 dir = {};			// 方向(光の進む向き)
		float brightness = 1.0f;		// カラーに掛ける強さ
		Math::Color color = {};			// 色
	};

	// ポイントライト
	struct PointLight
	{
		Math::Vector3 pos = {};			// 座標(ワールド空間)
		float brightness = 1.0f;		// カラーに掛ける強さ
		Math::Color color = {};			// 色
		float range = 10.0f;			// 光の届く距離 : ここで減衰が0になり、これより外は計算ごと飛ばす
	};
}
