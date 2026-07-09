#pragma once

namespace Engine::Resource
{
	const std::string WHITE_TEXTURE_GUIDSTR		=	"00000000-0000-0000-0000-000000000001";
	const std::string BLACK_TEXTURE_GUIDSTR		=	"00000000-0000-0000-0000-000000000002";
	const std::string NORMAL_TEXTURE_GUIDSTR	=	"00000000-0000-0000-0000-000000000003";
	const std::string ORM_TEXTURE_GUIDSTR		=	"00000000-0000-0000-0000-000000000004";

	// テクスチャ生成設定
	struct TextureCreateDesc
	{
		std::string name = "Texture";

		UINT64 width = 0;
		UINT height = 0;

		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

		UINT mipLevel = 1;
		UINT sampleCount = 1;

		// テクスチャの使用方法
		TextureUsage usage = TextureUsage::None;

		std::optional<DXSM::Color> opClerValue;
	};

	class Texture : public D3D12::GPUResource
	{
	public:
		~Texture() { ENGINE_LOG("テクスチャが解放されました"); }
		NON_COPYABLE_MOVABLE(Texture);

		// テクスチャ生成
		void Import(const std::string& a_filePath,const DirectX::XMFLOAT4& a_defoltData = { 255,255,255,255 });
		void Create(const std::string& a_name, const DirectX::XMFLOAT4& a_defoltData);
		void Create(const TextureCreateDesc& a_desc);
		void Create(IDXGISwapChain* a_pSwapChain,UINT a_backBufferIndex,TextureUsage a_texUsage = TextureUsage::RTV);

		// 解放
		void Release();
		
		// 名前変更
		void SetName(const std::string& a_name);
		const std::string& GetName() const;

		// リソース情報
		const TextureUsage& GetUsage() const;		// 使用フラグ
		const D3D12_RESOURCE_DESC& GetDesc() const;	// テクスチャ設定

		// クリアバリュー
		const DXSM::Color& GetClearColor() { return m_clearValue; }

	private:

		// ビューの作成
		void CreateView();
	private:

		// リソース
		std::string m_name = "none";						// テクスチャの名前	
		D3D12_RESOURCE_DESC m_desc;							// テクスチャの仕様書
		TextureUsage m_useFlg = TextureUsage::None;			// テクスチャの使用方法

		// クリアカラー
		DXSM::Color m_clearValue = {0,0,0,1};
	};
}