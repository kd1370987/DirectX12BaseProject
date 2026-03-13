#include "TextureManager.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

Engine::Resource::Handle<Engine::Resource::Texture> Engine::Resource::TextureManager::LoadTexture(
	const std::string& a_path
)
{
	auto _it = m_handleMap.find(a_path);
	if (_it != m_handleMap.end())
	{
		return _it->second;
	}

	Engine::Resource::Texture _texture = {};
	m_handleMap.emplace(a_path, Engine::Resource::Handle<Engine::Resource::Texture>{});
	_texture.Import(a_path);

	m_handleMap[a_path] = Add(_texture);

	// ビュー作成
	CreateView({ m_handleMap[a_path] });

	return m_handleMap[a_path];
}

std::vector<Engine::Resource::Handle<Engine::Resource::Texture>>
Engine::Resource::TextureManager::LoadTextureRange(const std::vector<TextureInit>& a_initVec)
{
	// 変数準備
	std::vector<Engine::Resource::Handle<Engine::Resource::Texture>> _result = {};

	// すべてのテクスチャをインポート
	for (auto& _init : a_initVec)
	{
		auto _path = _init.pathName;
		// ハンドルマップを追加
		m_handleMap.emplace(_path, Engine::Resource::Handle<Engine::Resource::Texture>{});

		// テクスチャ作成
		Engine::Resource::Texture _texture = {};
		_texture.Import(_path, _init.data);

		// 登録
		auto _handle = Add(_texture);
		m_handleMap[_path] = _handle;
		_result.push_back(_handle);

	}

	// 配列を一括でビューを作成。連続した領域になる
	CreateView(_result);


	return _result;
}

Engine::Resource::Handle<Engine::Resource::Texture> Engine::Resource::TextureManager::CreateTexture(
	const std::string& a_name,
	const UINT64& a_width,
	const UINT& a_height,
	const DXGI_FORMAT& a_format,
	const TextureUsage& a_usage
)
{
	auto _it = m_handleMap.find(a_name);
	if (_it != m_handleMap.end())
	{
		return _it->second;
	}

	m_handleMap.emplace(a_name, Engine::Resource::Handle<Engine::Resource::Texture>{});
	Engine::Resource::Texture _texture;
	_texture.Create(
		a_width,
		a_height,
		a_format,
		a_usage
	);
	_texture.SetName(a_name);

	m_handleMap[a_name] = Add(_texture);

	CreateView({m_handleMap[a_name]});

	return m_handleMap[a_name];
}

const Engine::Resource::Texture& Engine::Resource::TextureManager::GetTexture(const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle)
{
	if (GenCheck(a_handle))
	{
		return m_slotStorage[a_handle.idx].data;
	}
}

Engine::Resource::Texture& Engine::Resource::TextureManager::RefTexture(const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle)
{
	if (GenCheck(a_handle))
	{
		return m_slotStorage[a_handle.idx].data;
	}
}

std::unordered_map<std::string, Engine::Resource::Handle<Engine::Resource::Texture>>& Engine::Resource::TextureManager::RefAllTex()
{
	return m_handleMap;
}

std::vector<Engine::Resource::SharedSlot<Engine::Resource::Texture>>& Engine::Resource::TextureManager::GetAllTex()
{
	return m_slotStorage;
}

Engine::Resource::Handle<Engine::Resource::Texture> Engine::Resource::TextureManager::Add(const Texture& a_texture)
{
	// キューが空なら、サイズを広げる
	if (m_indexQueue.empty())
	{
		m_indexQueue.push(m_indexQueueMaxSize);
		m_indexQueueMaxSize++;
	}

	// キューからインデックスをもらう
	Engine::Resource::Index _idx = m_indexQueue.front();
	m_indexQueue.pop();

	// ストレージに追加
	if (_idx >= m_slotStorage.size())
	{
		m_slotStorage.resize(_idx + 1);
	}
	m_slotStorage[_idx].data = a_texture;
	m_slotStorage[_idx].gen = 0;
	m_slotStorage[_idx].sharedCount = 1;

	// ハンドルを作成して返す
	Engine::Resource::Handle<Engine::Resource::Texture> _handle = {};
	_handle.gen = 0;
	_handle.idx = _idx;

	return _handle;
}

void Engine::Resource::TextureManager::Subtract(const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle)
{
	// 安全チェック
	if (a_handle.idx >= m_slotStorage.size())
		return;
	if (m_slotStorage[a_handle.idx].gen != a_handle.gen)
		return;

	// 空のデータを入れる
	m_slotStorage[a_handle.idx].data = Engine::Resource::Texture{};
	m_slotStorage[a_handle.idx].gen++;

	// インデックスをキューに返還
	m_indexQueue.push(a_handle.idx);
}

void Engine::Resource::TextureManager::CreateView(
	const std::vector<Engine::Resource::Handle<Engine::Resource::Texture>>& a_outTex
)
{
	// インデックス収集
	std::vector<int> _rtvIdx = {};
	std::vector<int> _dsvIdx = {};
	std::vector<int> _srvIdx = {};
	std::vector<int> _uavIdx = {};
	for (UINT _i = 0 ; _i < a_outTex.size(); ++_i)
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
		for(UINT _i = 0; _i < _srvIdx.size(); ++_i)
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
		for(UINT _i = 0; _i < _srvIdx.size(); ++_i)
		{
			int _idx = _srvIdx[_i];
			auto& _tex = RefTexture(a_outTex[_idx]);
			_tex.SetSRV(_range[_i]);
			_tex.SetImGuiSRV(_imgRange[_i]);
		}
	}
}

bool Engine::Resource::TextureManager::GenCheck(const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle) const
{
	if (m_slotStorage[a_handle.idx].gen == a_handle.gen)
	{
		return true;
	}
	return false;
}

Engine::Resource::TextureManager::TextureManager()
{
}

Engine::Resource::TextureManager::~TextureManager()
{
}
