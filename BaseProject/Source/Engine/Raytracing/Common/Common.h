#pragma once

namespace Engine::Raytracing
{
	enum EShader
	{
		Raygeneration,		// カメラレイを生成するシェーダー
		Miss,				// カメラレイがどこにもぶつからなかったときに交差したときに呼ばれるシェーダー
		PBRChs,				// もっとも近いポリゴンとカメラレイが交差したときに呼ばれるシェーダー
		ShadowChs,			// もっとも近いポリゴンとシャドウレイが交差したときに呼ばれるシェーダー
		ShadowMiss,			// シャドウレイがどこにもぶつからなかったときに呼ばれるシェーダー
		Count				// シェーダーの数
	};
}