#include "Engine/Graphics/D3D12/VideoMemoryTracker/VideoMemoryTracker.h"

namespace Engine::Graphics::D3D12
{
	namespace
	{
		constexpr size_t CATEGORY_COUNT = static_cast<size_t>(EVideoMemoryCategory::Count);

		// 鍵は Windows の ::GUID(Core::GUID とは別物)
		// デバイスに付ける集計表の鍵 : {6B2F8C1E-3D4A-4F7B-9E21-5C8A0D3B7F14}
		constexpr ::GUID LEDGER_KEY = { 0x6b2f8c1e, 0x3d4a, 0x4f7b, { 0x9e, 0x21, 0x5c, 0x8a, 0x0d, 0x3b, 0x7f, 0x14 } };

		// リソース・ヒープに付ける札の鍵 : {0D71A5C3-8E2B-4C96-A147-3FE962B805D2}
		constexpr ::GUID TICKET_KEY = { 0x0d71a5c3, 0x8e2b, 0x4c96, { 0xa1, 0x47, 0x3f, 0xe9, 0x62, 0xb8, 0x05, 0xd2 } };

		//--------------------------------------------------------------------------------------
		// 参照カウントだけを持つ IUnknown
		//
		// D3D12 の private data へ預けるには IUnknown である必要がある。
		// 預けた側(デバイス・リソース)は、自分が消えるときに Release してくれる
		//--------------------------------------------------------------------------------------
		class RefCountedUnknown : public IUnknown
		{
		public:

			virtual ~RefCountedUnknown() = default;

			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID a_riid, void** a_ppObject) override
			{
				if (!a_ppObject) return E_POINTER;

				if (a_riid == __uuidof(IUnknown))
				{
					*a_ppObject = static_cast<IUnknown*>(this);
					AddRef();
					return S_OK;
				}

				*a_ppObject = nullptr;
				return E_NOINTERFACE;
			}

			ULONG STDMETHODCALLTYPE AddRef() override
			{
				return ++m_refCount;
			}

			ULONG STDMETHODCALLTYPE Release() override
			{
				const ULONG _count = --m_refCount;
				if (_count == 0) delete this;
				return _count;
			}

		private:

			// 作った側が 1 つ持っている状態から始まる
			std::atomic<ULONG> m_refCount = 1;
		};

		//--------------------------------------------------------------------------------------
		// 集計表 : デバイスに 1 つ
		//
		// リソースはどのスレッドからも作られ、どのスレッドで消えるかも決まっていないので、
		// 数はすべて atomic で持つ
		//--------------------------------------------------------------------------------------
		class Ledger final : public RefCountedUnknown
		{
		public:

			explicit Ledger(bool a_isUMA) : m_isUMA(a_isUMA) {}

			// CPU と GPU が同じメモリを使う構成か
			bool IsUMA() const { return m_isUMA; }

			void Add(EVideoMemoryCategory a_category, uint64_t a_bytes, bool a_isLocal)
			{
				Counter& _counter = m_counters[static_cast<size_t>(a_category)];
				(a_isLocal ? _counter.localBytes : _counter.nonLocalBytes) += a_bytes;
				++_counter.objectCount;
			}

			void Subtract(EVideoMemoryCategory a_category, uint64_t a_bytes, bool a_isLocal)
			{
				Counter& _counter = m_counters[static_cast<size_t>(a_category)];
				(a_isLocal ? _counter.localBytes : _counter.nonLocalBytes) -= a_bytes;
				--_counter.objectCount;
			}

			VideoMemoryCategoryUsage Get(EVideoMemoryCategory a_category) const
			{
				const Counter& _counter = m_counters[static_cast<size_t>(a_category)];

				VideoMemoryCategoryUsage _usage = {};
				_usage.localBytes = _counter.localBytes.load();
				_usage.nonLocalBytes = _counter.nonLocalBytes.load();
				_usage.objectCount = _counter.objectCount.load();
				return _usage;
			}

		private:

			struct Counter
			{
				std::atomic<uint64_t> localBytes = 0;
				std::atomic<uint64_t> nonLocalBytes = 0;
				std::atomic<uint32_t> objectCount = 0;
			};

			std::array<Counter, CATEGORY_COUNT> m_counters;
			const bool m_isUMA;
		};

		//--------------------------------------------------------------------------------------
		// 札 : 数えたリソース・ヒープ 1 つに 1 枚
		//
		// 作ったときに足し、持ち主が消えて Release されたときに引く。
		// 集計表を握っているので、デバイスが先に消えても引く先は残っている
		//--------------------------------------------------------------------------------------
		class Ticket final : public RefCountedUnknown
		{
		public:

			Ticket(Ledger* a_pLedger, EVideoMemoryCategory a_category, uint64_t a_bytes, bool a_isLocal)
				: m_cpLedger(a_pLedger)
				, m_category(a_category)
				, m_bytes(a_bytes)
				, m_isLocal(a_isLocal)
			{
				m_cpLedger->Add(m_category, m_bytes, m_isLocal);
			}

			~Ticket() override
			{
				m_cpLedger->Subtract(m_category, m_bytes, m_isLocal);
			}

		private:

			ComPtr<Ledger> m_cpLedger = nullptr;
			EVideoMemoryCategory m_category = EVideoMemoryCategory::Other;
			uint64_t m_bytes = 0;
			bool m_isLocal = true;
		};

		// デバイスに付けた集計表を引く。付いていなければ nullptr
		ComPtr<Ledger> FindLedger(ID3D12Device* a_pDevice)
		{
			IUnknown* _pUnknown = nullptr;
			UINT _size = sizeof(_pUnknown);
			if (FAILED(a_pDevice->GetPrivateData(LEDGER_KEY, &_size, &_pUnknown)) || !_pUnknown) return nullptr;

			// GetPrivateData が参照を 1 つ足して返すので、それをそのまま引き取る。
			// この鍵で預けているのは Ledger だけなので、型は決まっている
			ComPtr<Ledger> _cpLedger = nullptr;
			_cpLedger.Attach(static_cast<Ledger*>(_pUnknown));
			return _cpLedger;
		}

		// そのヒープが VRAM 側にあるか
		bool IsLocalHeap(const D3D12_HEAP_PROPERTIES& a_props, bool a_isUMA)
		{
			// CPU と GPU が同じメモリなら、DXGI はすべてを Local として数える
			if (a_isUMA) return true;

			switch (a_props.Type)
			{
			case D3D12_HEAP_TYPE_UPLOAD:
			case D3D12_HEAP_TYPE_READBACK:
				return false;
			case D3D12_HEAP_TYPE_CUSTOM:
				return a_props.MemoryPoolPreference == D3D12_MEMORY_POOL_L1;
			default:
				return true;
			}
		}

		// 札を付ける。前の札が付いていれば、ここで外れてその用途から引かれる
		void AttachTicket(ID3D12Object* a_pObject, Ledger* a_pLedger, EVideoMemoryCategory a_category, uint64_t a_bytes, bool a_isLocal)
		{
			ComPtr<Ticket> _cpTicket = nullptr;
			_cpTicket.Attach(new Ticket(a_pLedger, a_category, a_bytes, a_isLocal));

			// 預けた側が参照を 1 つ足すので、こちらの参照は抜けるときに手放してよい
			a_pObject->SetPrivateDataInterface(TICKET_KEY, _cpTicket.Get());
		}
	}

	const char* ToString(EVideoMemoryCategory a_category)
	{
		switch (a_category)
		{
		case EVideoMemoryCategory::Other:			return "Other";
		case EVideoMemoryCategory::MeshBuffer:		return "Mesh Buffer";
		case EVideoMemoryCategory::Texture:			return "Texture";
		case EVideoMemoryCategory::RenderTarget:	return "Render Target";
		case EVideoMemoryCategory::RenderGraph:		return "Render Graph";
		case EVideoMemoryCategory::BackBuffer:		return "Back Buffer";
		case EVideoMemoryCategory::BLAS:			return "BLAS";
		case EVideoMemoryCategory::BLASScratch:		return "BLAS Scratch";
		case EVideoMemoryCategory::RayWorld:		return "Ray World";
		case EVideoMemoryCategory::Particle:		return "Particle";
		case EVideoMemoryCategory::ConstantBuffer:	return "Constant Buffer";
		case EVideoMemoryCategory::Upload:			return "Upload";
		default:									return "Unknown";
		}
	}

	void VideoMemoryTracker::AttachTo(Device* a_pDevice)
	{
		if (!a_pDevice) return;

		// UMA なら VRAM とシステムメモリの区別が無い
		D3D12_FEATURE_DATA_ARCHITECTURE _architecture = {};
		const bool _isUMA =
			SUCCEEDED(a_pDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &_architecture, sizeof(_architecture)))
			&& _architecture.UMA;

		ComPtr<Ledger> _cpLedger = nullptr;
		_cpLedger.Attach(new Ledger(_isUMA));

		// 持ち主はデバイス : デバイスが消えるときに手放される
		a_pDevice->SetPrivateDataInterface(LEDGER_KEY, _cpLedger.Get());
	}

	void VideoMemoryTracker::TrackResource(ID3D12Resource* a_pResource, EVideoMemoryCategory a_category)
	{
		if (!a_pResource) return;

		// どのヒープ種別に作られたか。予約リソース(タイル)は引けないので数えない
		D3D12_HEAP_PROPERTIES _props = {};
		D3D12_HEAP_FLAGS _heapFlags = D3D12_HEAP_FLAG_NONE;
		if (FAILED(a_pResource->GetHeapProperties(&_props, &_heapFlags))) return;

		ComPtr<ID3D12Device> _cpDevice = nullptr;
		if (FAILED(a_pResource->GetDevice(IID_PPV_ARGS(&_cpDevice)))) return;

		ComPtr<Ledger> _cpLedger = FindLedger(_cpDevice.Get());
		if (!_cpLedger) return;

		// 実際に確保される大きさ(アライメントで切り上がったもの)で数える
		const D3D12_RESOURCE_DESC _desc = a_pResource->GetDesc();
		const D3D12_RESOURCE_ALLOCATION_INFO _info = _cpDevice->GetResourceAllocationInfo(0, 1, &_desc);
		if (_info.SizeInBytes == UINT64_MAX) return;

		AttachTicket(a_pResource, _cpLedger.Get(), a_category, _info.SizeInBytes, IsLocalHeap(_props, _cpLedger->IsUMA()));
	}

	void VideoMemoryTracker::TrackHeap(ID3D12Heap* a_pHeap, EVideoMemoryCategory a_category)
	{
		if (!a_pHeap) return;

		ComPtr<ID3D12Device> _cpDevice = nullptr;
		if (FAILED(a_pHeap->GetDevice(IID_PPV_ARGS(&_cpDevice)))) return;

		ComPtr<Ledger> _cpLedger = FindLedger(_cpDevice.Get());
		if (!_cpLedger) return;

		const D3D12_HEAP_DESC _desc = a_pHeap->GetDesc();
		AttachTicket(a_pHeap, _cpLedger.Get(), a_category, _desc.SizeInBytes, IsLocalHeap(_desc.Properties, _cpLedger->IsUMA()));
	}

	VideoMemoryBreakdown VideoMemoryTracker::Collect(Device* a_pDevice)
	{
		VideoMemoryBreakdown _out = {};
		if (!a_pDevice) return _out;

		ComPtr<Ledger> _cpLedger = FindLedger(a_pDevice);
		if (!_cpLedger) return _out;

		_out.isValid = true;
		for (size_t _i = 0; _i < CATEGORY_COUNT; ++_i)
		{
			_out.categories[_i] = _cpLedger->Get(static_cast<EVideoMemoryCategory>(_i));
		}
		return _out;
	}
}
