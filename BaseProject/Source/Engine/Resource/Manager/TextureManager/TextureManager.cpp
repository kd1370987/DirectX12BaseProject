#include "TextureManager.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Resource
{
	Handle<Texture> TextureManager::LoadTexture(
		const std::string& a_path
	)
	{
		// 登録されているかのチェック
		if (Has(a_path))
		{
			return m_nameMap[a_path];
		}

		// テクスチャ作成
		Texture _texture = {};
		_texture.Import(a_path);

		// ハンドルマップを追加
		auto _handle = Add(_texture);

		// 登録
		m_nameMap.emplace(a_path, _handle);

		// ビュー作成
		CreateView({ _handle });

		return _handle;
	}

	std::vector<Handle<Texture>>
		TextureManager::LoadTextureRange(const std::vector<TextureInit>& a_initVec)
	{
		// 変数準備
		std::vector<Handle<Texture>> _result = {};

		// すべてのテクスチャをインポート
		for (auto& _init : a_initVec)
		{
			auto _path = _init.pathName;
			// ハンドルマップを追加
			m_nameMap.emplace(_path, Handle<Texture>{});

			// テクスチャ作成
			Texture _texture = {};
			_texture.Import(_path, _init.data);

			// 登録
			auto _handle = Add(_texture);
			m_nameMap[_path] = _handle;
			_result.push_back(_handle);

		}

		// 配列を一括でビューを作成。連続した領域になる
		CreateView(_result);


		return _result;
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
		CreateView({ _handle });

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

	Handle<Texture> TextureManager::Add(const Texture& a_texture)
	{
		// ハンドルをもらう
		auto _handle = m_handleStorage.Allocate();
		if (_handle.idx >= m_texData.size())
		{
			m_texData.resize(_handle.idx + 1);
			m_sharedCount.resize(_handle.idx + 1);
		}
		m_texData[_handle.idx] = a_texture;
		m_sharedCount[_handle.idx]++;

		return _handle;
	}

	void TextureManager::Subtract(const Handle<Texture>& a_handle)
	{
		// ハンドルが有効かどうか
		if (!m_handleStorage.IsValid(a_handle))
		{
			return;
		}

		// シェアード数を減らす
		if (m_sharedCount[a_handle.idx] > 0)
		{
			m_sharedCount[a_handle.idx]--;
			return;
		}

		// データ削除
		m_handleStorage.Remove(a_handle);
		m_nameMap.erase(m_texData[a_handle.idx].GetName());
	}

	void TextureManager::CreateView(
		const std::vector<Handle<Texture>>& a_outTex
	)
	{
		// インデックス収集
		std::vector<int> _rtvIdx = {};
		std::vector<int> _dsvIdx = {};
		std::vector<int> _srvIdx = {};
		std::vector<int> _uavIdx = {};
		for (UINT _i = 0; _i < a_outTex.size(); ++_i)
		{
			auto& _tex = GetTexture(a_outTex[_i]);
			auto _usage = _tex.GetUsage();
			if (HasFlag(_usage, TextureUsage::RTV))
			{
				_rtvIdx.push_back(_i);
			}
			if (HasFlag(_usage, TextureUsage::DSV))
			{
				_dsvIdx.push_back(_i);
			}
			if (HasFlag(_usage, TextureUsage::UAV))
			{
				_uavIdx.push_back(_i);
			}
			if (HasFlag(_usage, TextureUsage::SRV))
			{
				_srvIdx.push_back(_i);
			}
		}

		// RTV作成
		for (auto& _idx : _rtvIdx)
		{
			auto& _tex = RefTexture(a_outTex[_idx]);
			auto& _desc = _tex.GetDesc();
			// レンダーターゲット作成
			D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
			_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			_rtvDesc.Format = _desc.Format;
			_tex.SetRTV(DescriptorHeapManager::Instance().AllocateRTV(_tex.GetResource(), &_rtvDesc));
		}

		// DSV作成
		for (auto& _idx : _dsvIdx)
		{
			auto& _tex = RefTexture(a_outTex[_idx]);
			auto& _desc = _tex.GetDesc();
			// 深度ステンシル作成
			D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
			_dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			_dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			_tex.SetDSV(DescriptorHeapManager::Instance().AllocateDSV(_tex.GetResource(), &_dsvDesc));
		}

		// UAV作成
		if (_uavIdx.size() > 0)
		{
			// ビュー作成情報
			std::vector<UAVViewInit> _viewInitVec = {};
			_viewInitVec.resize(_uavIdx.size());
			for (UINT _i = 0; _i < _srvIdx.size(); ++_i)
			{
				int _idx = _srvIdx[_i];
				auto& _tex = RefTexture(a_outTex[_idx]);

				// 作成情報を入れる
				_viewInitVec[_i].pResource = _tex.GetResource();
				_viewInitVec[_i].pDesc = nullptr;
			}

			// UAVをレンジで確保
			auto _range = DescriptorHeapManager::Instance().AllocateUAVRange(_viewInitVec);
			for (UINT _i = 0; _i < _srvIdx.size(); ++_i)
			{
				int _idx = _srvIdx[_i];
				auto& _tex = RefTexture(a_outTex[_idx]);
				_tex.SetUAV(_range[_i]);
			}
		}

		// SRV作成
		if (_srvIdx.size() > 0)
		{
			// ビュー作成情報
			std::vector<SRVViewInit>						_viewInitVec = {};
			std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>	_viewDescVec = {};
			_viewInitVec.resize(_srvIdx.size());
			_viewDescVec.resize(_srvIdx.size());
			//for(auto& _i : _srvIdx)
			for (UINT _i = 0; _i < _srvIdx.size(); ++_i)
			{
				int _idx = _srvIdx[_i];
				auto& _tex = RefTexture(a_outTex[_idx]);
				auto _usage = _tex.GetUsage();
				auto& _desc = _tex.GetDesc();
				// シェーダーリソースビュー作成
				_viewDescVec[_i].Format = HasFlag(_usage, TextureUsage::DSV) ? DXGI_FORMAT_R32_FLOAT : _desc.Format;
				_viewDescVec[_i].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				_viewDescVec[_i].Texture2D.MipLevels = _desc.MipLevels;
				_viewDescVec[_i].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				// 作成情報を入れる
				_viewInitVec[_i].pResource = _tex.GetResource();
				_viewInitVec[_i].pDesc = &_viewDescVec[_i];
			}

			// SRVをレンジで確保
			auto _range = DescriptorHeapManager::Instance().AllocateSRVRange(_viewInitVec);
			auto _imgRange = DescriptorHeapManager::Instance().AllocateImGuiSRVRange(_viewInitVec);
			for (UINT _i = 0; _i < _srvIdx.size(); ++_i)
			{
				int _idx = _srvIdx[_i];
				auto& _tex = RefTexture(a_outTex[_idx]);
				_tex.SetSRV(_range[_i]);
				_tex.SetImGuiSRV(_imgRange[_i]);
			}
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