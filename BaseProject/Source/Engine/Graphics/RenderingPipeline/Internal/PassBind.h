#pragma once
//==========================================================================================
//
// PassBind (Engine::Graphics::Pipeline)
//
// 焼き込み済みのバインド1件。
// 積むのも張るのも RenderGraph なので、この階層の外へは出さない
//
//==========================================================================================
namespace Engine::Graphics::Pipeline
{
	// 焼き込み済みのバインド1件 : ルート定数1本ぶんのビュー番号の範囲
	struct PassBind
	{
		UINT rootIndex = 0;
		uint16_t firstIndex = 0;	// descriptorIndex への開始添字
		uint16_t count = 1;
	};
}
