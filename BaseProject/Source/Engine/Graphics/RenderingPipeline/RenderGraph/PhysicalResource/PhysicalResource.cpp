#include "PhysicalResource.h"

// ヘッダーでは前方宣言にしてあるので、実体はここで揃える
#include "../Resource/VirtualResource/VirtualResource.h"

namespace Engine::Graphics::Pipeline
{
	bool PhysicalResource::ResolvePlacement(const VirtualResource& a_virtual, ID3D12Heap* a_pHeap, uint64_t* a_pOutOffset)
	{
		if (a_pOutOffset) *a_pOutOffset = 0;

		// ヒープが用意できていない(サイズ0・Tier1・作成失敗)
		if (!a_pHeap) return false;

		// 外部リソースの実体はグラフの外の持ち物
		if (a_virtual.IsImported()) return false;

		// バッファはまだ placed の口が無い(GPUBuffer 側が未対応)。
		// 今のところグラフにバッファのスロットを宣言するパスは無い
		if (a_virtual.IsBuffer()) return false;

		// 席に着いていないものは実体を独り占めするので、個別に作る。
		// (履歴つき・配線から外れたもの)
		const AllocationInfo& _info = a_virtual.GetAllocationInfo();
		if (_info.slotIndex == AllocationInfo::INVALID_SLOT_INDEX) return false;

		// 大きさを見積もれていないなら席の大きさも 0 なので、置いても収まらない
		if (a_virtual.GetAllocationSize() == 0) return false;

		if (a_pOutOffset) *a_pOutOffset = _info.offset;
		return true;
	}

	bool PhysicalResource::Create(D3D12::Device* a_pDevice, const VirtualResource& a_virtual, ID3D12Heap* a_pHeap)
	{
		// 作り直しなので、前の実体はここで手放す
		Release();

		m_isOutsideResource = false;
		m_isBuffer = a_virtual.IsBuffer();

		m_format = a_virtual.GetFormat();
		m_width = a_virtual.GetWidth();
		m_height = a_virtual.GetHeight();
		m_usage = a_virtual.GetUsage();

		if (m_isBuffer)
		{
			if (!a_pDevice)
			{
				ENGINE_WARNING("[PhysicalResource] デバイスが無いのでバッファを作れません : %s", a_virtual.GetName().c_str());
				return false;
			}
			if (m_width == 0)
			{
				ENGINE_WARNING("[PhysicalResource] バッファのサイズが0です : %s", a_virtual.GetName().c_str());
				return false;
			}

			m_upBuffer = std::make_unique<D3D12::GPUBuffer>();

			D3D12::GPUBufferDesc _desc = {};
			_desc.elementNum = 1;
			_desc.strideSize = static_cast<size_t>(m_width);	// width にバイト数が入っている
			_desc.heapType = D3D12_HEAP_TYPE_DEFAULT;

			// UAV として触るなら生成時にフラグを立てておく必要がある
			_desc.flags = a_virtual.HasUsage(Resource::TextureUsage::UAV)
				? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
				: D3D12_RESOURCE_FLAG_NONE;

			if (!m_upBuffer->Create(a_pDevice, _desc))
			{
				ENGINE_WARNING("[PhysicalResource] バッファの生成に失敗しました : %s", a_virtual.GetName().c_str());
				m_upBuffer.reset();
				return false;
			}

			m_pResource = m_upBuffer.get();
			return true;
		}

		// ---- テクスチャ ----
		if (m_format == DXGI_FORMAT_UNKNOWN)
		{
			ENGINE_WARNING("[PhysicalResource] フォーマットが決まっていません : %s", a_virtual.GetName().c_str());
			return false;
		}
		if (m_width == 0 || m_height == 0)
		{
			ENGINE_WARNING("[PhysicalResource] サイズが0です : %s", a_virtual.GetName().c_str());
			return false;
		}

		m_upTexture = std::make_unique<Resource::Texture>();

		// 宣言は仮想リソースが持つ。
		// 占有サイズの見積もり(CalcAllocationSize)と同じものを通さないと、
		// 確保した席にリソースが収まらない
		const Resource::TextureCreateDesc _texDesc = a_virtual.ToTextureCreateDesc();

		uint64_t _heapOffset = 0;
		if (ResolvePlacement(a_virtual, a_pHeap, &_heapOffset))
		{
			m_upTexture->Create(a_pHeap, _heapOffset, _texDesc);

			// 置けたときだけ場所を覚える。
			// 失敗したまま覚えると、次の IsMatch が空の実体を使い回してしまう
			if (!m_upTexture->GetResource())
			{
				ENGINE_WARNING("[PhysicalResource] ヒープ上に置けませんでした : %s (offset=%llu)",
					a_virtual.GetName().c_str(), _heapOffset);
				m_upTexture.reset();
				return false;
			}

			m_pHeap = a_pHeap;
			m_heapOffset = _heapOffset;
		}
		else
		{
			m_upTexture->Create(_texDesc);
		}

		m_pResource = m_upTexture.get();
		return true;
	}

	void PhysicalResource::Import(D3D12::GPUResource* a_pResource)
	{
		// 実体を持っていたなら手放してから参照へ切り替える
		Release();

		m_isOutsideResource = true;
		m_pResource = a_pResource;
	}

	bool PhysicalResource::IsMatch(const VirtualResource& a_virtual, ID3D12Heap* a_pHeap) const
	{
		// 実体を持っていなければ作るしかない
		if (!m_pResource) return false;

		// 外部参照と自前生成は入れ替えられない
		if (m_isOutsideResource != a_virtual.IsImported()) return false;

		// 外部参照なら中身はこちらの管轄外
		if (m_isOutsideResource) return true;

		//----------------------------------------------------------------------------------
		// 置き場所
		//
		// 席は再コンパイルのたびに動くので、フォーマットも大きさも同じなのに
		// 別の場所を指したままになることがある。
		// ここを見ないと、古い場所に置いたままの実体を使い回してしまう
		//----------------------------------------------------------------------------------
		uint64_t _heapOffset = 0;
		const bool _isPlaced = ResolvePlacement(a_virtual, a_pHeap, &_heapOffset);

		if (_isPlaced != IsPlaced()) return false;
		if (_isPlaced && (m_pHeap != a_pHeap || m_heapOffset != _heapOffset)) return false;

		if (m_isBuffer != a_virtual.IsBuffer()) return false;

		if (m_isBuffer)
		{
			return m_width == a_virtual.GetWidth()
				&& m_usage == a_virtual.GetUsage();
		}

		return m_format == a_virtual.GetFormat()
			&& m_width == a_virtual.GetWidth()
			&& m_height == a_virtual.GetHeight()
			&& m_usage == a_virtual.GetUsage();
	}

	void PhysicalResource::Release()
	{
		// 外部から借りているだけのものは、こちらで解放してはいけない
		if (!m_isOutsideResource)
		{
			// unique_ptr の破棄でも ComPtr は解放されるが、Release() を先に呼ぶことで
			// ディスクリプタヒープのハンドルも確実に返却し、破棄タイミングに依存しないようにする
			if (m_upTexture) m_upTexture->Release();
			if (m_upBuffer) m_upBuffer->Release();
		}

		m_upTexture.reset();
		m_upBuffer.reset();

		m_pResource = nullptr;
		m_isOutsideResource = false;
		m_isBuffer = false;

		// 置き場所も忘れる : 覚えたままだと IsMatch が古い場所と比べてしまう
		m_pHeap = nullptr;
		m_heapOffset = 0;

		m_format = DXGI_FORMAT_UNKNOWN;
		m_width = 0;
		m_height = 0;
		m_usage = Resource::TextureUsage::None;
	}
}
