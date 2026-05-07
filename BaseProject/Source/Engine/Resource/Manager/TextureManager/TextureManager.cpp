#include "TextureManager.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Resource
{
	Handle<Texture> TextureManager::LoadTexture(const std::string& a_path, const DirectX::XMFLOAT4& a_data)
	{
		// 登録されているかのチェック
		if (Has(a_path))
		{
			return m_nameMap[a_path];
		}

		// テクスチャ作成
		Texture _texture = {};
		_texture.Import(a_path,a_data);
		_texture.SetName(a_path);

		// ハンドルマップを追加
		auto _handle = Add(_texture);

		// 登録
		m_nameMap.emplace(a_path, _handle);

		// ビュー作成
		//CreateView(_handle);

		return _handle;
	}

	Handle<Texture> TextureManager::CreateTexture(const TextureCreateDesc& a_init)
	{
		// 登録されているかのチェック
		if (Has(a_init.name))
		{
			return m_nameMap[a_init.name];
		}

		// テクスチャ作成
		Texture _texture;
		_texture.Create(a_init);
		_texture.SetName(a_init.name);

		// ハンドルマップを追加
		auto _handle = Add(_texture);

		// 登録
		//CreateView(_handle);

		m_nameMap.emplace(a_init.name, _handle);

		return _handle;
	}

	const Texture& TextureManager::GetTexture(const Handle<Texture>& a_handle)
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			return m_texData[a_handle.idx];
		}
	}

	const Texture& TextureManager::GetTexture(const std::string& a_name)
	{
		if (Has(a_name))
		{
			auto _handle = m_nameMap[a_name];
			return GetTexture(_handle);
		}
	}

	Texture& TextureManager::RefTexture(const Handle<Texture>& a_handle)
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			return m_texData[a_handle.idx];
		}
	}

	Texture& TextureManager::RefTexture(const std::string& a_name)
	{
		if (Has(a_name))
		{
			auto _handle = m_nameMap[a_name];
			return RefTexture(_handle);
		}
	}

	std::unordered_map<std::string, Handle<Texture>>& TextureManager::RefAllTex()
	{
		return m_nameMap;
	}

	std::vector<Texture>& TextureManager::GetAllTex()
	{
		return m_texData;
	}

	const Handle<Texture>& TextureManager::GetHandle(const std::string& a_name)
	{
		if (Has(a_name))
		{
			return m_nameMap[a_name];
		}
		return {};
	}

	Handle<Texture> TextureManager::Add(const Texture& a_texture)
	{
		// ハンドルをもらう
		auto _handle = m_handleStorage.Allocate();
		if (_handle.idx >= m_texData.size())
		{
			m_texData.resize(_handle.idx + 1);
		}
		m_texData[_handle.idx] = a_texture;
		
		return _handle;
	}

	void TextureManager::Subtract(const Handle<Texture>& a_handle)
	{
		// ハンドルが有効かどうか
		if (!m_handleStorage.IsValid(a_handle))
		{
			return;
		}

		// データ削除
		m_handleStorage.Remove(a_handle);
		m_nameMap.erase(m_texData[a_handle.idx].GetName());
	}

	void TextureManager::CreateView(const Handle<Texture>& a_outTex)
	{
		auto* _pDev = Engine::D3D12::D3D12Wrapper::Instance().GetDevice();
		auto& _tex = RefTexture(a_outTex);
		auto _usage = _tex.GetUsage();
		if (HasFlag(_usage, TextureUsage::RTV))
		{
			auto& _desc = _tex.GetDesc();
			// レンダーターゲット作成
			D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
			_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			_rtvDesc.Format = _desc.Format;
			_tex.SetRTV(D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::RTV>(_pDev,_tex.GetResource(), &_rtvDesc));
		}
		if (HasFlag(_usage, TextureUsage::DSV))
		{
			auto& _desc = _tex.GetDesc();
			// 深度ステンシル作成
			D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
			_dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			_dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			_tex.SetDSV(D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::DSV>(_pDev, _tex.GetResource(), &_dsvDesc));
		}
		if (HasFlag(_usage, TextureUsage::UAV))
		{
			auto& _desc = _tex.GetDesc();
			_tex.SetUAV(D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::UAV>(_pDev, _tex.GetResource(), nullptr));
		}
		if (HasFlag(_usage, TextureUsage::SRV))
		{
			auto& _desc = _tex.GetDesc();
			D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
			// シェーダーリソースビュー作成
			_srvDesc.Format = HasFlag(_usage, TextureUsage::DSV) ? DXGI_FORMAT_R32_FLOAT : _desc.Format;
			_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			_srvDesc.Texture2D.MipLevels = _desc.MipLevels;
			_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			// 登録
			_tex.SetSRV(D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::SRV>(_pDev, _tex.GetResource(), &_srvDesc));

			// 同時にImGuiも登録
			_tex.SetImGuiSRV(D3D12::DescriptorHeapManager::Instance().AllocateImGuiSRV(_tex.GetResource(), &_srvDesc));
		}
	}


	bool TextureManager::Has(const std::string& a_name) const
	{
		auto _it = m_nameMap.find(a_name);
		if (_it != m_nameMap.end())
		{
			return true;
		}
		return false;
	}

	TextureManager::TextureManager()
	{}

	TextureManager::~TextureManager()
	{}
}