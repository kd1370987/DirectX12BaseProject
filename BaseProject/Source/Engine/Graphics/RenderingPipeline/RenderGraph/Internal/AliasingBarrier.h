#pragma once
#include "../../Core/ResourceID.h"

namespace Engine::D3D12
{
	class GPUResource;
}

namespace Engine::Graphics::Pipeline
{
	// =====================================================================================
	// リソースバリア(コンパイル時に計算済みのもの)
	// =====================================================================================
	struct AliasingBarrier
	{
		ResourceID before = {};
		ResourceID after = {};

		// 実体化後に焼きこむ
		D3D12::GPUResource* pBeforeResource[2] = { nullptr,nullptr };
		D3D12::GPUResource* pAffterResource[2] = { nullptr,nullptr };
	};
}