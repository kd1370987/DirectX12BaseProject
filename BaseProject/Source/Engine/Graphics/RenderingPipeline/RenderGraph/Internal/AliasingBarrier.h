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
		// 同じ席を直前に使っていたリソース。
		//
		// 空のままなら、実体を引いたときに nullptr になる。
		// D3D12 ではこれが「このヒープのどれかが前の使い手」という保守的な指定になり、
		// 席の一人目(前のフレームの最後の使い手から引き継ぐところ)で使う
		ResourceID before = {};

		// この継ぎ目から席を使い始めるリソース
		ResourceID after = {};

		// 実体化後に焼きこむ
		D3D12::GPUResource* pBeforeResource[2] = { nullptr,nullptr };
		D3D12::GPUResource* pAffterResource[2] = { nullptr,nullptr };
	};
}