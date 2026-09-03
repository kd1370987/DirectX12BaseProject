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
	// 焼き込み済みのバインド1件
	struct PassBind
	{
		enum class EType : uint8_t { SrvTable, Uav };

		EType type = EType::SrvTable;
		UINT rootIndex = 0;
		uint16_t firstHandle = 0;	// descriptorTable への開始添字
		uint16_t count = 1;
	};
}
