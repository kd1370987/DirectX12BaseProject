#pragma once

namespace Engine::D3D12Buffer
{
	/// <summary>
	/// GPUバッファの基底クラス
	/// </summary>
	class GPUBuffer
	{
	public:
		//=============================================================================
		// 作成
		//=============================================================================
		bool Create(
			D3D12_HEAP_TYPE a_heapType,
			size_t a_bufferSize,
			D3D12_RESOURCE_STATES a_initState = D3D12_RESOURCE_STATE_COMMON,
			const void* a_pInitData = nullptr
		);

		
		//=============================================================================
		// 更新
		//=============================================================================
		// データ更新
		void Update(const void* a_pData);

		// ステート変更
		void ChengeState(ID3D12GraphicsCommandList* a_pCmdList , const D3D12_RESOURCE_STATES& a_nextState);
		
	private:

		ComPtr<ID3D12Resource> m_cpBuffer = nullptr;			// バッファ本体
		size_t m_bufferSize = 0;								// バッファのサイズ
		uint8_t* m_mappedData = nullptr;						// マップされたデータのポインタ

		// 現在のリソースステート
		D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;
	};
}