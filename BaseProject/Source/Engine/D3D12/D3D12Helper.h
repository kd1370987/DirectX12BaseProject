#pragma once
namespace Engine::D3D12
{
	/// <summary>
	/// リソースバリア
	/// </summary>
	/// <param name="a_pCmdList">実行リスト</param>
	/// <param name="a_pResource">実行リソース</param>
	/// <param name="a_beffor">リソースの現在のステート</param>
	/// <param name="a_affter">遷移後のステート</param>
	inline void ResourceBarrier(
		GraphicsCommandList* a_pCmdList,
		ID3D12Resource* a_pResource,
		D3D12_RESOURCE_STATES a_beffor,
		D3D12_RESOURCE_STATES a_affter
	)
	{
		
		D3D12_RESOURCE_BARRIER _barrier = {};
		_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		_barrier.Transition.pResource = a_pResource;
		_barrier.Transition.StateAfter = a_affter;
		_barrier.Transition.StateBefore = a_beffor;
		_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		a_pCmdList->ResourceBarrier(1, &_barrier);
	}

}
