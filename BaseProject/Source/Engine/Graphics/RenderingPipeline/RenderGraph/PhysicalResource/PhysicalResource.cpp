#include "PhysicalResource.h"

// ヘッダーでは前方宣言にしてあるので、実体はここで揃える
#include "../Resource/VirtualResource/VirtualResource.h"

namespace Engine::Graphics::Pipeline
{
	bool PhysicalResource::Create(D3D12::Device* a_pDevice, const VirtualResource& a_virtual)
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

		Resource::TextureCreateDesc _desc = {};
		_desc.name = a_virtual.GetName();
		_desc.width = m_width;
		_desc.height = m_height;
		_desc.format = m_format;
		_desc.usage = m_usage;

		// RTV / DSV はクリアバリューを作成時に渡しておかないと、
		// クリアのたびにドライバ側で最適化が効かず警告も出る
		if (a_virtual.HasUsage(Resource::TextureUsage::RTV) ||
			a_virtual.HasUsage(Resource::TextureUsage::DSV))
		{
			_desc.opClerValue = a_virtual.GetClearColor();
		}

		m_upTexture->Create(_desc);

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

	bool PhysicalResource::IsMatch(const VirtualResource& a_virtual) const
	{
		// 実体を持っていなければ作るしかない
		if (!m_pResource) return false;

		// 外部参照と自前生成は入れ替えられない
		if (m_isOutsideResource != a_virtual.IsImported()) return false;

		// 外部参照なら中身はこちらの管轄外
		if (m_isOutsideResource) return true;

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

		m_format = DXGI_FORMAT_UNKNOWN;
		m_width = 0;
		m_height = 0;
		m_usage = Resource::TextureUsage::None;
	}
}
