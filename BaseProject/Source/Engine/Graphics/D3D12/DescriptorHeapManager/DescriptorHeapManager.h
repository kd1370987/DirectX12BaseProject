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
		//
		// CBV / SRV / UAV の席は、すぐには空きへ戻さない。
		// シェーダーは可視ヒープの席を番号で直接引くので、まだ走っているフレームが
		// 読んでいる席を使い回すと、そのフレームの描画が別のビューを読んでしまう。
		// 「今記録しているフレームが終わる値」を付けて預かり、GPUがそこまで進んだら戻す
		// (ProcessDeferredFrees)。RTV / DSV は記録の時点で読まれるのですぐ戻す
		template<IsHeapType T>
		void Free(const Handle<T>& a_handle);

		//--------------------------------------------------------------------------------------------
		// 解放の遅延
		//--------------------------------------------------------------------------------------------
		// 「今記録しているフレームが終わるときのフェンス値」を返す関数を受け取る。
		// 渡されていない間(フレームを回す前・ツールなど)は、解放はその場で行う
		void SetNextFenceValueProvider(std::function<UINT64()> a_provider);

		// GPUが a_completedFenceValue まで進んだので、それ以前に預かった席を空きへ戻す。
		// フレームの頭(前のフレームの完了を待った後)に呼ぶ
		void ProcessDeferredFrees(UINT64 a_completedFenceValue);

		// ハンドルの取得
		template<IsHeapType T>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(Handle<T> a_handle);
		template<IsHeapType T>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(Handle<T> a_handle);

		// ヒープ取得
		//
		// CBV / SRV / UAV は同じ番号の席を2枚のヒープに持つ。
		//   GetCBVSRVUAVHeap              … CPU専用。ビューを作る先で、コピー元と
		//                                   ClearUnorderedAccessView のCPUハンドルに使う
		//   RefShaderVisibleCBVSRVUAVHeap … シェーダー可視。SetDescriptorHeaps で張って、
		//                                   シェーダーから番号(ResourceDescriptorHeap[i])で引く
		// GetCPU は前者、GetGPU は後者のハンドルを返す
		UINT GetCBVSRVUAVHeapSize();
		ID3D12DescriptorHeap* GetCBVSRVUAVHeap();
		ID3D12DescriptorHeap* RefShaderVisibleCBVSRVUAVHeap();

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
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_cbv_srv_uavHeap;					// CPU専用(ビューを作る先)
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_cbv_srv_uavShaderVisibleHeap;	// シェーダー可視(同じ番号の写し)
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

		//--------------------------------------------------------------------------------------------
		// 解放の遅延
		//--------------------------------------------------------------------------------------------
		struct PendingFree
		{
			UINT64 fenceValue = 0;				// GPUがこの値まで進んだら戻してよい
			std::function<void()> release;		// 空きへ戻す処理(種類ごとのアロケーターへ)
		};

		// 預かり中の席 : 解放はワーカースレッドから来ることもあるのでロックで守る
		std::vector<PendingFree> m_pendingFrees;
		std::mutex m_pendingMutex;

		// 今記録しているフレームが終わるときのフェンス値を返す(GraphicsEngine が渡す)
		std::function<UINT64()> m_nextFenceValueProvider = nullptr;
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
		if (!a_handle.IsValid()) return;

		// シェーダーが番号で引く種類だけ、GPUが使い終わるまで預かる
		constexpr bool _isShaderIndexed =
			std::is_same_v<T, CBV> || std::is_same_v<T, SRV> || std::is_same_v<T, UAV>;

		if constexpr (_isShaderIndexed)
		{
			if (m_nextFenceValueProvider)
			{
				const UINT64 _fenceValue = m_nextFenceValueProvider();
				std::lock_guard<std::mutex> _lock(m_pendingMutex);
				m_pendingFrees.push_back({ _fenceValue, [this, a_handle]() { RefAllocator<T>().Remove(a_handle); } });
				return;
			}
		}

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
