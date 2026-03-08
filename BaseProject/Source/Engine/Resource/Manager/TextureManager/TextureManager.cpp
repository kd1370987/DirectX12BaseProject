#include "TextureManager.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

Engine::Resource::Handle<Engine::Resource::TextureRes> Engine::Resource::TextureManager::LoadTexture(
	const std::string& a_path
)
{
	auto _it = m_handleMap.find(a_path);
	if (_it != m_handleMap.end())
	{
		return _it->second;
	}

	m_handleMap.emplace(a_path);
	Engine::Resource::TextureRes _texture = {};
	_texture.Import(a_path);
	SRVViewInit _init = { _texture.GetResource(),nullptr};
	DescriptorHeapManager::Instance().AllocateSRVRange({ _init });

	m_handleMap[a_path] = Add(_texture);
	return m_handleMap[a_path];
}

Engine::Resource::HandleRange<Engine::Resource::TextureRes> Engine::Resource::TextureManager::LoadTextureRange(
	const std::vector<std::string>& a_pathVec
)
{
	Engine::Resource::HandleRange<Engine::Resource::TextureRes> _result = {};
	for (auto& _path : a_pathVec)
	{
		_result.value.push_back(LoadTexture(_path));
	}
	return _result;
}

Engine::Resource::Handle<Engine::Resource::TextureRes> Engine::Resource::TextureManager::CreateTexture(
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

	m_handleMap.emplace(a_name);
	Engine::Resource::TextureRes _texture;
	_texture.Create(
		a_width,
		a_height,
		a_format,
		a_usage
	);
	_texture.SetName(a_name);

	m_handleMap[a_name] = Add(_texture);
	return m_handleMap[a_name];
}

Engine::Resource::Handle<Engine::Resource::TextureRes> Engine::Resource::TextureManager::Add(const TextureRes& a_texture)
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
	Engine::Resource::Handle<Engine::Resource::TextureRes> _handle = {};
	_handle.gen = 0;
	_handle.idx = _idx;

	return _handle;
}

void Engine::Resource::TextureManager::Subtract(const Engine::Resource::Handle<Engine::Resource::TextureRes>& a_handle)
{
	// 安全チェック
	if (a_handle.idx >= m_slotStorage.size())
		return;
	if (m_slotStorage[a_handle.idx].gen != a_handle.gen)
		return;

	// 空のデータを入れる
	m_slotStorage[a_handle.idx].data = Engine::Resource::TextureRes{};
	m_slotStorage[a_handle.idx].gen++;

	// インデックスをキューに返還
	m_indexQueue.push(a_handle.idx);
}

Engine::Resource::TextureManager::TextureManager()
{
}

Engine::Resource::TextureManager::~TextureManager()
{
}
