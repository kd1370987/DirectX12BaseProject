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


	/// <summary>
	/// レンダーターゲットのクリア
	/// </summary>
	/// <param name="a_pCmdList"></param>
	/// <param name="a_renderTargetView"></param>
	/// <param name="a_colorRGBA"></param>
	/// <param name="a_numRects"></param>
	/// <param name="a_pRects"></param>
	inline void ClearRenderTargetView(
		GraphicsCommandList* a_pCmdList,
		D3D12_CPU_DESCRIPTOR_HANDLE a_renderTargetView,
		DirectX::XMFLOAT4 a_colorRGBA = { 0.0f,0.0f,0.0f,1.0f },
		UINT a_numRects = 0,
		const D3D12_RECT* a_pRects = nullptr
	)
	{
		const float _color[] = { a_colorRGBA.x,a_colorRGBA.y ,a_colorRGBA.z ,a_colorRGBA.w };
		a_pCmdList->ClearRenderTargetView(
			a_renderTargetView,
			_color,
			a_numRects,
			a_pRects
		);
	}

	/// <summary>
	/// 深度ステンシルビューをクリア
	/// </summary>
	/// <param name="a_depthStencilView">指定対象のハンドル</param>
	/// <param name="a_clearFlags">クリアフラグ</param>
	/// <param name="a_depth">奥(1.0)</param>
	/// <param name="a_stencil">手前(0.0)</param>
	/// <param name="a_numRects">矩形数</param>
	/// <param name="a_pRects">矩形ポインタ</param>
	inline void ClearDepthStencilView(
		GraphicsCommandList* a_pCmdList,
		D3D12_CPU_DESCRIPTOR_HANDLE a_depthStencilView,
		D3D12_CLEAR_FLAGS a_clearFlags = D3D12_CLEAR_FLAG_DEPTH,
		float a_depth = 1.0f,
		float a_stencil = 0.0f,
		UINT a_numRects = 0,
		const D3D12_RECT* a_pRects = nullptr
	)
	{
		a_pCmdList->ClearDepthStencilView(
			a_depthStencilView,
			a_clearFlags,
			a_depth,
			a_stencil,
			a_numRects,
			a_pRects
		);
	}
}
