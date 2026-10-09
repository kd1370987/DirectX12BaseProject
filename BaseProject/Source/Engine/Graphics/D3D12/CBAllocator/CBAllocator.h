#pragma once

#include "Core/Debug/DebugLog.h"


namespace Engine::Graphics::D3D12
{
	class CBAllocator
	{
	public:
	
		// 解放
		void Release();

		void RootCBVCreate(Engine::Graphics::D3D12::Device* a_device, size_t a_memSize);
	
		// 計測用 : 今フレームに使ったバイト数と容量(グラフィック用 / コンピュート用)
		size_t GetUsedBytes() const { return static_cast<size_t>(m_usedCount) * 256; }
		size_t GetCapacityBytes() const { return m_capacity; }
		size_t GetComputeUsedBytes() const { return static_cast<size_t>(m_useComputeCount) * 256; }
		size_t GetComputeCapacityBytes() const { return m_computeCapacity; }

		// 使用リセット
		void ResetUse()
		{
			m_usedCount = 0;
			m_useComputeCount = 0;
		}

		// データをバインドして転送
		void BindAndAttachDataRootCBV(Engine::Graphics::D3D12::GraphicsCommandList* a_pCmdList, int a_descIndex, const void* a_data, size_t a_size);
		template<typename T>
		void BindAndAttachDataRootCBV(Engine::Graphics::D3D12::GraphicsCommandList* a_pCmdList, int a_descIndex, const T& a_data);

		// データをバインドして転送
		template<typename T>
		void BindAndAttachDataComputeRootCBV(Engine::Graphics::D3D12::GraphicsCommandList* a_pCmdList, int a_descIndex, const T& a_data);

	private:

		// コンピュート用リソースデータ作成
		void CreateCompute(size_t a_memSize);

	private:
		// デバイス
		Engine::Graphics::D3D12::Device* m_pDevice = nullptr;

		// グラフィック用
		UINT m_usedCount = 0;
		ComPtr<ID3D12Resource> m_spResource = nullptr;
		struct { uint8_t buff[256]; }*m_pMappedData = nullptr;
		size_t m_capacity = 0;

		// コンピュート用
		UINT m_useComputeCount = 0;
		ComPtr<ID3D12Resource> m_spComputeResource = nullptr;
		struct { uint8_t buff[256]; }*m_pComputeMappedData = nullptr;
		size_t m_computeCapacity = 0;
	};

	template<typename T>
	inline void CBAllocator::BindAndAttachDataRootCBV(
		Engine::Graphics::D3D12::GraphicsCommandList* a_pCmdList,
		int a_descIndex,
		const T& a_data
	)
	{
		size_t _dataSize = (sizeof(T) + 0xff) & ~0xff; // 256バイトアライメント

		int _useValue = static_cast<int>(_dataSize / 0x100);
		if ((m_usedCount + _useValue) * 256 > m_capacity)
		{
			// ヒープに登録できる数を超えた
			ENGINE_ERRLOG(false, "アップロードヒープの上限を迎えました");
			return;
		}

		// アドレス位置
		int _top = m_usedCount;

		// データ転送
		std::memcpy(&m_pMappedData[_top].buff, &a_data, sizeof(T));

		// コマンドリストにセット
		a_pCmdList->SetGraphicsRootConstantBufferView(
			a_descIndex,
			m_spResource->GetGPUVirtualAddress() + (static_cast<UINT64>(_top) * 0x100)
		);

		m_usedCount += _useValue;
	}

	template<typename T>
	inline void CBAllocator::BindAndAttachDataComputeRootCBV(Engine::Graphics::D3D12::GraphicsCommandList* a_pCmdList, int a_regiIdx, const T& a_data)
	{
		size_t _dataSize = (sizeof(T) + 0xff) & ~0xff; // 256バイトアライメント

		int _useValue = static_cast<int>(_dataSize / 0x100);
		if ((m_useComputeCount + _useValue) * 256 > m_computeCapacity)
		{
			// ヒープに登録できる数を超えた
			ENGINE_ERRLOG(false, "アップロードヒープの上限を迎えました");
			return;
		}

		// アドレス位置
		int _top = m_useComputeCount;

		// データ転送
		std::memcpy(&m_pComputeMappedData[_top].buff, &a_data, sizeof(T));

		// コマンドリストにセット
		a_pCmdList->SetComputeRootConstantBufferView(
			a_regiIdx,
			m_spComputeResource->GetGPUVirtualAddress() + (static_cast<UINT64>(_top) * 0x100)
		);

		m_useComputeCount += _useValue;
	}
}


