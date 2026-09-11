#pragma once

namespace Engine::D3D12
{
	class DescriptorHeapManager;
}

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

		/// <summary>
		/// SRVで読むときの成分の並び替え(スウィズル)
		/// </summary>
		/// <remarks>
		/// 1成分だけのテクスチャ(R8_UNORMのフォントアトラスなど)を
		/// シェーダー側で rgba すべてに配りたいときに使う。
		/// 既定はそのまま(R,G,B,A)。
		/// 例: D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0,0,0,0) で (r,r,r,r) になる
		/// </remarks>
		UINT srvComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		std::optional<Math::Color> opClerValue;
	};

	class Texture : public D3D12::GPUResource
	{
	public:
		Texture() = default;
		~Texture() = default;
		NON_COPYABLE_MOVABLE(Texture);

		//--------------------------------------------------------------------------------------------
		// テクスチャ生成
		//
		// 先頭のディスクリプタヒープは、このテクスチャのビュー(SRV/RTV/DSV/UAV)を置く先。
		// 実体は GraphicsEngine が持っているので、呼び出し側はコンテキストから受け取ったものを渡す。
		// 渡したものは控えられ、Release() で同じところへ返る
		//--------------------------------------------------------------------------------------------
		// 読み込みと既定色の生成は GPU への転送を伴うので、ビルドコンテキストを受け取る
		// (転送の依頼先とビューの置き場はコンテキストが持っている)
		void Import(const ResourceBuildContext& a_ctx,const std::string& a_filePath,const Math::Color& a_defoltData = { 255,255,255,255 });
		void Create(const ResourceBuildContext& a_ctx,const std::string& a_name, const Math::Color& a_defoltData);
		void Create(D3D12::DescriptorHeapManager* a_pHeapManager,const TextureCreateDesc& a_desc);
		void Create(D3D12::DescriptorHeapManager* a_pHeapManager,IDXGISwapChain* a_pSwapChain,UINT a_backBufferIndex,TextureUsage a_texUsage = TextureUsage::RTV);

		/// <summary>
		/// 指定のヒープ上に作成する(placed)
		/// </summary>
		/// <remarks>
		/// 中身は committed 版とまったく同じで、実体をどこに置くかだけが違う。
		/// 席は呼び手が確保しておくこと。
		/// 大きさと詰め方は BuildTextureResourceDesc() から起こした仕様書を
		/// GetResourceAllocationInfo へ渡して求める : 同じ仕様書でないと席に収まらない
		/// </remarks>
		void Create(D3D12::DescriptorHeapManager* a_pHeapManager,ID3D12Heap* a_pHeap,UINT64 a_heapOffset,const TextureCreateDesc& a_desc);

		/// <summary>
		/// 元のPNGなどのパスを基準に横にDDSテクスチャを作成する
		/// </summary>
		void Save(const std::string& a_srcPath);

		// 解放
		void Release();
		
		// 名前変更
		void SetName(const std::string& a_name);
		const std::string& GetName() const;

		// リソース情報
		const TextureUsage& GetUsage() const;		// 使用フラグ
		const D3D12_RESOURCE_DESC& GetDesc() const;	// テクスチャ設定

		// クリアバリュー
		const Math::Color& GetClearColor() { return m_clearValue; }

	private:

		// 実体が出来たあとの共通処理 : 要件の控え・デバッグ名・ビューの登録。
		// committed / placed のどちらから来ても同じでないといけないので1箇所に寄せる
		void SetupFromDesc(D3D12::DescriptorHeapManager* a_pHeapManager, const TextureCreateDesc& a_desc);

		// ビューの作成
		void CreateView(D3D12::DescriptorHeapManager* a_pHeapManager);
	private:

		// リソース
		std::string m_name = "none";						// テクスチャの名前	
		D3D12_RESOURCE_DESC m_desc;							// テクスチャの仕様書
		TextureUsage m_useFlg = TextureUsage::None;			// テクスチャの使用方法

		// SRVの成分並び替え : 1成分テクスチャを rgba へ配るときに使う
		UINT m_srvComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// クリアカラー
		Math::Color m_clearValue = {0,0,0,0};
	};
}