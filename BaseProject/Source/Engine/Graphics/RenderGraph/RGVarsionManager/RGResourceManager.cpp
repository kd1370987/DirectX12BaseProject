#include "RGResourceManager.h"

//#include "../../../Resource/Manager/TextureManager/TextureManager.h"
#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics
{
	// ==========================================================
	// 一時リソース : グラフ内で実体を作成・管理するもの
	// ==========================================================
	void RGResourceManager::DeclareTexture(const std::string& a_name, const DXGI_FORMAT& a_format, const UINT64& a_width, const UINT& a_height, const Resource::TextureUsage& a_usage, const DXSM::Color& a_clearColor)
	{
		if (m_nameMap.find(a_name) != m_nameMap.end()) return;

		LogicalResource _res;
		_res.name = a_name;
		_res.type = ERGResourceType::Texture;
		_res.format = a_format;
		_res.width = a_width;
		_res.height = a_height;
		_res.usage = a_usage;
		_res.clearColor = a_clearColor;

		_res.isImported = false;
		_res.currentVersion = 0;
		_res.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		_res.pPhysicalResource = nullptr;

		m_logicalResourceVec.push_back(_res);
		m_nameMap[a_name] = static_cast<Resource::Index>(m_logicalResourceVec.size() - 1);
	}
	void RGResourceManager::DeclareBuffer(const std::string & a_name, const UINT64 & a_sizeBytes)
	{
		if (m_nameMap.find(a_name) != m_nameMap.end()) return;

		LogicalResource _res;
		_res.name = a_name;
		_res.type = ERGResourceType::Buffer;
		_res.width = a_sizeBytes; // バッファの場合はwidthをサイズとして扱う

		_res.isImported = false;
		_res.currentVersion = 0;
		_res.currentState = D3D12_RESOURCE_STATE_COMMON;
		_res.pPhysicalResource = nullptr;

		m_logicalResourceVec.push_back(_res);
		m_nameMap[a_name] = static_cast<Resource::Index>(m_logicalResourceVec.size() - 1);
	}
	// ==========================================================
	// 外部リソース : グラフ外で作成され、状態遷移のみ管理するもの
	// ==========================================================
	void RGResourceManager::ImportResource(ERGResourceType a_type, const std::string & a_name, D3D12::GPUResource * a_pExternalResource, D3D12_RESOURCE_STATES a_initialState)
	{
		if (m_nameMap.find(a_name) != m_nameMap.end()) return;

		LogicalResource _res;
		_res.name = a_name;
		_res.type = a_type;
		_res.isImported = true;
		_res.pPhysicalResource = a_pExternalResource;

		_res.currentVersion = 0;
		_res.currentState = a_initialState;

		m_logicalResourceVec.push_back(_res);
		m_nameMap[a_name] = static_cast<Resource::Index>(m_logicalResourceVec.size() - 1);
	}
	// ==========================================================
	// 依存関係の構築 (バージョン進行)
	// ==========================================================
	RGResourceHandle RGResourceManager::Read(const std::string & a_name)
	{
		auto _it = m_nameMap.find(a_name);
		assert(_it != m_nameMap.end() && "リソースが宣言またはインポートされていません");

		uint32_t _index = _it->second;
		// Read は状態を変えないので、現在のバージョンをそのまま返す
		return { _index, m_logicalResourceVec[_index].currentVersion };
	}
	RGResourceHandle RGResourceManager::Write(const std::string& a_name)
	{
		auto _it = m_nameMap.find(a_name);
		assert(_it != m_nameMap.end() && "リソースが宣言またはインポートされていません");

		uint32_t _index = _it->second;
		// Write はリソースの中身を書き換えるため、バージョンを進める（++）
		m_logicalResourceVec[_index].currentVersion++;

		return { _index, m_logicalResourceVec[_index].currentVersion };
	}
	// ==========================================================
	// グラフのコンパイルと実行管理
	// ==========================================================
	void RGResourceManager::AllocateResources(D3D12::Device* a_pDevice)
	{
		size_t _texPoolIndex = 0;
		size_t _bufPoolIndex = 0;

		for (auto& _res : m_logicalResourceVec)
		{
			// 外部リソースは既に実体があるのでスキップ
			if (_res.isImported) continue;

			if (_res.type == ERGResourceType::Texture)
			{
				// プールに空きがない場合は新規作成
				if (_texPoolIndex >= m_tempTextures.size())
				{
					// ※あなたのエンジンのTexture生成クラスに合わせて修正してください
					auto _newTex = std::make_unique<Resource::Texture>();
					Resource::TextureCreateDesc _texDesc = {};
					_texDesc.width = _res.width;
					_texDesc.height = _res.height;
					_texDesc.format = _res.format;
					_texDesc.usage = _res.usage;
					_texDesc.name = _res.name;
					_newTex->Create(_texDesc);
					m_tempTextures.push_back(std::move(_newTex));
				}
				else
				{
					// ※本来はプールのテクスチャサイズと要求サイズ(_res.width等)を比較し、
					// 違っていればRecreateする処理をここに入れます
				}

				_res.pPhysicalResource = m_tempTextures[_texPoolIndex].get();
				_texPoolIndex++;
			}
			else if (_res.type == ERGResourceType::Buffer)
			{
				// プールに空きがない場合は新規作成
				if (_bufPoolIndex >= m_tempBuffers.size())
				{
					auto _newBuf = std::make_unique<D3D12::GPUBuffer>();

					D3D12::GPUBufferDesc _bufDesc = {};
					_bufDesc.elementNum = 1;
					_bufDesc.strideSize = static_cast<UINT>(_res.width); // widthに入れたsizeBytesを使う
					_bufDesc.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // UAVとして使うなら必須
					_bufDesc.heapType = D3D12_HEAP_TYPE_DEFAULT;

					_newBuf->Create(a_pDevice, _bufDesc);
					m_tempBuffers.push_back(std::move(_newBuf));
				}
				else
				{
					// ※ ここも要求サイズが変わった場合はRecreateする処理を入れます
				}

				_res.pPhysicalResource = m_tempBuffers[_bufPoolIndex].get();
				_bufPoolIndex++;
			}
		}
	}
	void RGResourceManager::ResetForNextFrame()
	{
		// コンパイルを通した時のみにリセットを入れて構築
		m_nameMap.clear();
		m_logicalResourceVec.clear();
	}
	void RGResourceManager::ReleasePhysicalResources()
	{
		// 各テクスチャ/バッファの ID3D12Resource とディスクリプタを明示的に解放してから
		// コンテナを空にする。
		// (unique_ptr の破棄だけでも ComPtr は解放されるが、Release() を先に呼ぶことで
		//  ディスクリプタヒープのハンドルも確実に返却し、破棄タイミングに依存しないようにする)
		for (auto& _upTex : m_tempTextures)
		{
			if (_upTex) _upTex->Release();
		}
		m_tempTextures.clear();

		for (auto& _upBuf : m_tempBuffers)
		{
			if (_upBuf) _upBuf->Release();
		}
		m_tempBuffers.clear();

		// 物理リソースを指していた論理リソースの割り当ても無効化
		m_nameMap.clear();
		m_logicalResourceVec.clear();
	}
	RGResourceHandle RGResourceManager::GetHandle(const std::string& a_name) const
	{
		auto _it = m_nameMap.find(a_name);
		if (_it != m_nameMap.end())
		{
			uint32_t _idx = _it->second;
			return { _idx, m_logicalResourceVec[_idx].currentVersion };
		}
		// 無効なハンドルを返す
		return { static_cast<uint32_t>(-1), 0 };
	}
	D3D12::GPUResource* RGResourceManager::GetPhysicalResource(RGResourceHandle a_handle) const
	{
		if(!a_handle.IsValid() || a_handle.index > m_logicalResourceVec.size())
		{
			assert(a_handle.IsValid() && a_handle.index < m_logicalResourceVec.size() && "無効なリソースハンドルです");
		}
		return m_logicalResourceVec[a_handle.index].pPhysicalResource;
	}
	D3D12_RESOURCE_STATES RGResourceManager::GetCurrentState(RGResourceHandle a_handle) const
	{
		assert(a_handle.IsValid() && a_handle.index < m_logicalResourceVec.size() && "無効なリソースハンドルです");
		return m_logicalResourceVec[a_handle.index].currentState;
	}
	void RGResourceManager::SetCurrentState(RGResourceHandle a_handle, D3D12_RESOURCE_STATES a_newState)
	{
		assert(a_handle.IsValid() && a_handle.index < m_logicalResourceVec.size() && "無効なリソースハンドルです");
		m_logicalResourceVec[a_handle.index].currentState = a_newState;
	}
	DXGI_FORMAT RGResourceManager::GetDXGIFormat(RGResourceHandle a_handle) const
	{
		assert(a_handle.IsValid() && a_handle.index < m_logicalResourceVec.size() && "無効なリソースハンドルです");
		return m_logicalResourceVec[a_handle.index].format;
	}
	const RGResourceManager::LogicalResource& RGResourceManager::GetRes(RGResourceHandle a_handle) const
	{
		assert(a_handle.IsValid() && a_handle.index < m_logicalResourceVec.size());
		return m_logicalResourceVec[a_handle.index];
	}
	RGResourceManager::LogicalResource& RGResourceManager::RefRes(RGResourceHandle a_handle)
	{
		assert(a_handle.IsValid() && a_handle.index < m_logicalResourceVec.size());
		return m_logicalResourceVec[a_handle.index];
	}

	void RGResourceManager::ResetForNextFrame(D3D12::GraphicsCommandList* a_pCmdList)
	{
		for (auto& _res : m_logicalResourceVec)
		{
			// グラフ内の一時リソースならステートをCOMMONに戻す
			if (!_res.isImported && _res.pPhysicalResource)
			{
				if (_res.currentState != D3D12_RESOURCE_STATE_COMMON)
				{
					_res.pPhysicalResource->Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COMMON);
					_res.currentState = D3D12_RESOURCE_STATE_COMMON;
				}
			}
			// バージョンをリセット（次のフレームのRead/Writeを正しく動かすため）
			_res.currentVersion = 0;
		}
	}
}
