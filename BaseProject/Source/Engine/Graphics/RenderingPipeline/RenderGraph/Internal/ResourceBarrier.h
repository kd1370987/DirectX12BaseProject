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
	struct ResourceBarrier
	{
		ResourceHandle handle = {};								// どの仮想リソースか

		// このバリアが触るスライス([0]=Current/書く側 [1]=Previous/読む側)。
		// Temporal でないリソースは常に 0
		uint32_t slice = 0;

		// 実体 : AllocateResources 後に埋まる。
		// Temporal は偶数フレームと奇数フレームで別の物理を触るので、両方を焼き込んでおく
		D3D12::GPUResource* pResource[2] = { nullptr, nullptr };

		D3D12_RESOURCE_STATES before = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_COMMON;

		// UAV -> UAV : ステートは変わらないが、前の書き込みの完了を待たせる必要がある
		bool isUAVBarrier = false;
	};
}