#pragma once
namespace Engine::Graphics::Pipeline
{
	/// <summary>
	/// メモリ共有用リソース
	/// </summary>
	class AliasingResource
	{
	public:

		/// <summary>
		/// レンダーグラフがヒープを確保しているのでそこに確保
		/// </summary>
		void Create(D3D12::Device* a_pDevice,ID3D12Heap* a_pHeap,UINT64 a_width,UINT a_height,DXGI_FORMAT a_maxFormat);

	private:

		ComPtr<ID3D12Resource> m_cpResouce = nullptr;



	};
}