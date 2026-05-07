#include "TextureLoader.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Resource
{
	std::unordered_map<Engine::GUID, Engine::Resource::Handle<Engine::Resource::Texture>>
	Engine::Resource::TextureLoader::m_cache;
	std::unordered_map<std::string, Engine::Resource::Handle<Engine::Resource::Texture>>
	Engine::Resource::TextureLoader::m_nameCache;
	

	Handle<Texture> Engine::Resource::TextureLoader::Load(const Engine::GUID& a_guid)
	{
		// すでに読み込み済みならそのハンドルを返す
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return _it->second;
		}

		// なければパスを検索してロードする
		auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		if (_path == "NoFilePath")
		{
			// パスが見つからなければデフォルトテクスチャを返す
			return RequestDefaultTex();
		}

		// パスが見つかればテクスチャを読み込む
		Texture _Texture = {};
		_Texture.Import(_path);

		//リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(_Texture);

		// ビュー作成
		CreateView(_handle);

		// キャッシュに登録
		m_cache.emplace(a_guid, _handle);

		return _handle;
	}
	Handle<Texture> TextureLoader::Request(const std::string& a_path)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			return Load(_guid);
		}

		// GUIDがなければ
		// フォルダ以下をもう一度探す処理を入れる予定だが、とりあえず、エラー値を返す
		assert(0 && "検索したテクスチャがありません");
		return Handle<Texture>();
	}
	Handle<Texture> TextureLoader::Create(const TextureCreateDesc& a_initData)
	{
		// 作成済みかのチェック
		auto _it = m_nameCache.find(a_initData.name);
		if (_it != m_nameCache.end())
		{
			return _it->second;
		}

		// テクスチャ作成
		Texture _tex;
		_tex.Create(a_initData);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(_tex);

		// ビューの作成
		CreateView(_handle);

		// ハンドルキャッシュを追加
		m_nameCache.emplace(a_initData.name,_handle);

		return _handle;
	}
	const std::unordered_map<Engine::GUID, Handle<Texture>>& TextureLoader::GetAllCache()
	{
		return m_cache;
	}
	const std::unordered_map<std::string, Handle<Texture>>& TextureLoader::GetAllNameCache()
	{
		return m_nameCache;
	}
	Handle<Texture> TextureLoader::RequestDefaultTex()
	{
		TextureCreateDesc _desc = {};
		_desc.name = "DefaultWhiteTex";
		_desc.width = 4;
		_desc.height = 4;
		_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		_desc.usage = TextureUsage::SRV;
		return Create(_desc);
	}
	void TextureLoader::CreateView(const Handle<Texture>& a_handle)
	{
		// デバイスの取得
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
		if (!_pDevice) { assert(0 && "Not fined Device"); return; }
		// テクスチャの取得
		auto* _pTex = ResourceManager::Instance().Ref(a_handle);
		if (!_pTex) return;
		auto _usage = _pTex->GetUsage();
		
		// テクスチャの使用方法にRTが含まれているのならRTVを登録
		if (HasFlag(_usage, TextureUsage::RTV))
		{
			// レンダーターゲットビュー情報の作成
			D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
			_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			_rtvDesc.Format = _pTex->GetDesc().Format;

			// ディスクリプタヒープに登録
			_pTex->SetRTV(
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::RTV>(_pDevice, _pTex->GetResource(), &_rtvDesc)
			);
		}

		// テクスチャの使用方法にDSが含まれているのなら
		if (HasFlag(_usage, TextureUsage::DSV))
		{
			// 深度ステンシルビュー作成
			D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
			_dsvDesc.Format = _pTex->GetDesc().Format;
			_dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			// ディスクリプタヒープに登録
			_pTex->SetDSV(
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::DSV>(_pDevice, _pTex->GetResource(), &_dsvDesc)
			);
		}

		// テクスチャの使用方法にUAが含まれているのなら
		if (HasFlag(_usage, TextureUsage::UAV))
		{
			_pTex->SetUAV(
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::UAV>(_pDevice, _pTex->GetResource(), nullptr)
			);
		}

		// テクスチャの仕様方法SRが含まれているのなら
		if (HasFlag(_usage, TextureUsage::SRV))
		{
			// シェーダーリソースビュー作成
			D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
			_srvDesc.Format = HasFlag(_usage, TextureUsage::DSV) ? DXGI_FORMAT_R32_FLOAT : _pTex->GetDesc().Format;
			_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			_srvDesc.Texture2D.MipLevels = _pTex->GetDesc().MipLevels;
			_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			// 登録
			_pTex->SetSRV(
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::SRV>(_pDevice, _pTex->GetResource(), &_srvDesc)
			);

			// 同時にImGuiも登録
			_pTex->SetImGuiSRV(
				D3D12::DescriptorHeapManager::Instance().AllocateImGuiSRV(_pTex->GetResource(), &_srvDesc)
			);
		}

	}
}