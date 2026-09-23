#pragma once

#include "Allocator/HeapAllocator.h"

namespace Engine::D3D12
{
	// 前方宣言
	class SamplerAllocator;

	// ディスクリプタヒープを管理
	//
	// 実体は GraphicsEngine が unique_ptr で1つだけ持つ。
	// 参照する側はシングルトンを引かず、コンテキスト経由で受け取ったポインタを使うこと
	// (PassContext / ResourceBuildContext / EditorContext、
	//  下位のD3D12層は生成時に受け取ったポインタを保持する)
	class DescriptorHeapManager
	{
	public:

		DescriptorHeapManager();
		~DescriptorHeapManager();
		NON_COPYABLE_MOVABLE(DescriptorHeapManager);

		// 初期化と解放
		//
		// デバイスはここで受け取って保持する(持ち主は GraphicsEngine)。
		// どこかのシングルトンから引くと、持ち主との間で循環になる
		bool Init(
			D3D12::Device* a_pDevice,
			UINT a_cbvCount,
			UINT a_srvCount,
			UINT a_uavCount,
			UINT a_rtvCount,
			UINT a_dsvCount
		);
		void Release();

		// ビューを作るのに使っているデバイス(借り物)。
		// ヒープを受け取ってリソースを作る側(テクスチャ・板ポリなど)は、
		// デバイスを別に引かずここから借りる
		D3D12::Device* RefDevice() const { return m_pDevice; }

		// リソースのビュー作成
		template<IsHeapType T>
		Handle<T> Allocate(D3D12::Device* a_pDevice,ID3D12Resource* a_pResource,const typename T::DescType* a_desc);

		// ビューの解放
		template<IsHeapType T>
		void Free(const Handle<T>& a_handle);

		// ハンドルの取得
		template<IsHeapType T>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(Handle<T> a_handle);
		template<IsHeapType T>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(Handle<T> a_handle);

		// ヒープ取得
		UINT GetCBVSRVUAVHeapSize();
		ID3D12DescriptorHeap* GetCBVSRVUAVHeap();

		//==========================================================================================
		//
		// ImGui
		//
		//==========================================================================================

		// ImGuiヒープの先頭に確保しておくバックエンド専用ディスクリプタ数
		//
		// ImGuiのDX12バックエンドは自分でSRVを作るので、その置き場を
		// アプリ側(m_ImGuiSRVAllocator)の払い出し範囲の外に隔離しておく必要がある。
		// ここを共有すると、テクスチャを確保し続けたときに
		// フォントアトラスのディスクリプタが上書きされて文字が化ける。
		static constexpr UINT IMGUI_BACKEND_DESCRIPTOR_COUNT = 8;

		// ImGui初期設定用
		ID3D12DescriptorHeap* GetImGuiHeap() const;

		// ImGuiバックエンドが使うディスクリプタの確保／解放
		// (ImGui_ImplDX12_InitInfo の SrvDescriptorAllocFn / SrvDescriptorFreeFn から呼ぶ)
		bool AllocateImGuiBackendDescriptor(
			D3D12_CPU_DESCRIPTOR_HANDLE* a_pOutCPU,
			D3D12_GPU_DESCRIPTOR_HANDLE* a_pOutGPU
		);
		void FreeImGuiBackendDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);

		// 一括でSRVを確保
		Handle<ImGuiSRV> AllocateImGuiSRV(ID3D12Resource* a_pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* a_desc);


		// 解放
		void FreeImGuiSRV(const Handle<ImGuiSRV>& a_handle);

		// ImGuiのSRVハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiSRVCPUHandle(Engine::Handle<ImGuiSRV> a_range);
		D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiSRVGPUHandle(Engine::Handle<ImGuiSRV> a_range);

		//==========================================================================================
		// 
		// SAMPLER
		// 
		//==========================================================================================
		// 作成
		Engine::Handle<SAMPLER> CreateSampler(
			D3D12::Device* a_pDevice,
			const D3D12_SAMPLER_DESC& a_desc
		);

		// 取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetLinearWrap();
		D3D12_GPU_DESCRIPTOR_HANDLE GetPointClamp();
		D3D12_GPU_DESCRIPTOR_HANDLE GetShadow();

		// ヒープ
		ID3D12DescriptorHeap* RefSamplerHeap();


	private:

		// ビューの種類に合ったアロケーターを引く(定義はこのヘッダーの末尾)
		template<IsHeapType T>
		HeapAllocator<T>& RefAllocator();

		// 借り物。Init で受け取ったものを持ち続ける
		D3D12::Device* m_pDevice = nullptr;

		// ヒープ本体
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_cbv_srv_uavHeap;
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>			m_dsvHeap;
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>			m_rtvHeap;

		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>		m_samplerHeap;
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_imguiHeap;

		// ヒープアロケーター
		HeapAllocator<CBV>		m_CBVAllocator;
		HeapAllocator<SRV>		m_SRVAllocator;
		HeapAllocator<UAV>		m_UAVAllocator;

		HeapAllocator<RTV>		m_RTVAllocator;
		HeapAllocator<DSV>		m_DSVAllocator;
		
		std::unique_ptr<SamplerAllocator>	m_upSamplerAllocator = nullptr;
		HeapAllocator<ImGuiSRV>				m_ImGuiSRVAllocator;

		// ImGuiバックエンド専用ディスクリプタ(ヒープ先頭の予約領域)の空きインデックス
		std::vector<UINT>					m_imguiBackendFreeIndices;

		// サンプラー
		Engine::Handle<SAMPLER> m_linearWrap;
		Engine::Handle<SAMPLER> m_pointClamp;
		Engine::Handle<SAMPLER> m_shadow;
	};
	//==========================================================================================
	// ビューの種類 → アロケーター
	//
	// Allocate / Free / GetCPU / GetGPU はどれも「種類に合ったアロケーターへ回す」だけなので、
	// 振り分けはここ1か所に置く。対応していない種類はコンパイル時に弾く
	//==========================================================================================
	namespace Internal
	{
		template<typename>
		inline constexpr bool kAlwaysFalse = false;
	}

	template<IsHeapType T>
	inline HeapAllocator<T>& DescriptorHeapManager::RefAllocator()
	{
		if constexpr (std::is_same_v<T, CBV>)			return m_CBVAllocator;
		else if constexpr (std::is_same_v<T, SRV>)		return m_SRVAllocator;
		else if constexpr (std::is_same_v<T, UAV>)		return m_UAVAllocator;
		else if constexpr (std::is_same_v<T, RTV>)		return m_RTVAllocator;
		else if constexpr (std::is_same_v<T, DSV>)		return m_DSVAllocator;
		else if constexpr (std::is_same_v<T, ImGuiSRV>)	return m_ImGuiSRVAllocator;
		else static_assert(Internal::kAlwaysFalse<T>, "DescriptorHeapManager : 対応していないビューの種類です");
	}

	template<IsHeapType T>
	inline Handle<T> DescriptorHeapManager::Allocate(D3D12::Device* a_pDevice, ID3D12Resource* a_pResource, const typename T::DescType* a_desc)
	{
		return RefAllocator<T>().Allocate(a_pDevice, a_pResource, a_desc);
	}

	template<IsHeapType T>
	inline void DescriptorHeapManager::Free(const Handle<T>& a_handle)
	{
		RefAllocator<T>().Remove(a_handle);
	}

	template<IsHeapType T>
	inline D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetCPU(Handle<T> a_handle)
	{
		return RefAllocator<T>().GetCPU(a_handle);
	}

	template<IsHeapType T>
	inline D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetGPU(Handle<T> a_handle)
	{
		// RTV / DSV のヒープはシェーダーから見えないので、GPUハンドルは存在しない
		static_assert(!std::is_same_v<T, RTV> && !std::is_same_v<T, DSV>,
			"DescriptorHeapManager::GetGPU : RTV / DSV はシェーダーから見えないヒープにあります");
		return RefAllocator<T>().GetGPU(a_handle);
	}
}
