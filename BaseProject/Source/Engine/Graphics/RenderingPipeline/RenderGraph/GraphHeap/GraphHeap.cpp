#include "GraphHeap.h"
namespace Engine::Graphics::Pipeline
{
	bool GraphHeap::Create(D3D12::Device* a_pDevice, UINT64 a_maxHeapSize)
	{
		if (!a_pDevice) return false;

		// 置くものが無いならヒープも要らない。
		// サイズ0で CreateHeap を呼ぶと失敗するので、ここで抜ける
		if (a_maxHeapSize == 0)
		{
			Release();
			return false;
		}

		// 大きさが変わっていなければそのまま使う。
		// 作り直すと、GPUがまだ読んでいる実体の下からメモリが消える
		if (m_cpHeap && m_maxHeapSize == a_maxHeapSize) return true;

		// 混ぜられないハードならエイリアシング自体を諦める
		if (!IsAliasingSupported(a_pDevice))
		{
			Release();
			return false;
		}

		// 古い実体を先に手放す。
		// ここに置いたリソースは参照を握っているので、
		// まだ生きていれば古いヒープはそちらが道連れに保つ
		Release();

		// ヒープ設定作成
		D3D12_HEAP_DESC _heapDesc = {};
		_heapDesc.SizeInBytes = a_maxHeapSize;
		_heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		_heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		_heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		_heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		// 全種類を許可 : Tier 2 以上であることは上で確かめてある
		_heapDesc.Flags = D3D12_HEAP_FLAG_NONE;

		// ヒープ作成
		HRESULT _hr = a_pDevice->CreateHeap(&_heapDesc, IID_PPV_ARGS(&m_cpHeap));
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(false, "グラフヒープの作成に失敗 : %llu バイト", a_maxHeapSize);
			m_cpHeap.Reset();
			return false;
		}

		m_maxHeapSize = a_maxHeapSize;
		return true;
	}

	void GraphHeap::Release()
	{
		m_cpHeap.Reset();
		m_maxHeapSize = 0;
	}

	bool GraphHeap::IsAliasingSupported(D3D12::Device* a_pDevice)
	{
		if (!a_pDevice) return false;

		D3D12_FEATURE_DATA_D3D12_OPTIONS _options = {};
		HRESULT _hr = a_pDevice->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS, &_options, sizeof(_options));

		if (FAILED(_hr)) return false;

		const bool _isSupported = (_options.ResourceHeapTier >= D3D12_RESOURCE_HEAP_TIER_2);

		// コンパイルのたびに出ると鬱陶しいので、一度だけ知らせる
		static bool s_isReported = false;
		if (!_isSupported && !s_isReported)
		{
			s_isReported = true;
			ENGINE_WARNING(
				"[GraphHeap] Resource Heap Tier 1 のため、リソースの使い回しを行いません。"
				"各リソースを個別に作成します");
		}

		return _isSupported;
	}
}