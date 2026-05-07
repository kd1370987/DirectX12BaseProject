#pragma once

namespace Engine::Resource
{
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

	class Texture
	{
	public:
		Texture() = default;
		~Texture() = default;

		// テクスチャ生成
		void Import(const std::string& a_filePath,const DirectX::XMFLOAT4& a_defoltData = { 255,255,255,255 });
		void Create(const std::string& a_name, const DirectX::XMFLOAT4& a_defoltData);
		void Create(const TextureCreateDesc& a_desc);

		// ビューの作成
		void CreateView();
		
		// 名前変更
		void SetName(const std::string& a_name);
		const std::string& GetName() const;

		// リソース情報
		ID3D12Resource* GetResource();				// 生データ
		const TextureUsage& GetUsage() const;		// 使用フラグ
		const D3D12_RESOURCE_DESC& GetDesc() const;	// テクスチャ設定

		// ステート
		D3D12_RESOURCE_STATES GetState() { return m_currentSutate; }
		void ChangeState(ID3D12GraphicsCommandList* a_pCmdList,D3D12_RESOURCE_STATES a_state);

		// クリアバリュー
		const DXSM::Color& GetClearColor() { return m_clearValue; }

		// ビュー情報取得
		const Engine::Resource::Handle<D3D12::RTV>& GetRTV() const;
		const Engine::Resource::Handle<D3D12::DSV>& GetDSV() const;
		const Engine::Resource::Handle<D3D12::SRV>& GetSRV() const;
		const Engine::Resource::Handle<D3D12::UAV>& GetUAV() const;
		const Engine::Resource::Handle<D3D12::SRV>& GetImGuiSRV() const;

		// ビュー情報セット
		void SetRTV(const Engine::Resource::Handle<D3D12::RTV>& a_handle);
		void SetDSV(const Engine::Resource::Handle<D3D12::DSV>& a_handle);
		void SetSRV(const Engine::Resource::Handle<D3D12::SRV>& a_handle);
		void SetUAV(const Engine::Resource::Handle<D3D12::UAV>& a_handle);
		void SetImGuiSRV(const Engine::Resource::Handle<D3D12::SRV>& a_handle);

	private:

		// リソース
		std::string m_name = "none";						// テクスチャの名前
		ComPtr<ID3D12Resource> m_cpResource = nullptr;		// テクスチャ本体			
		D3D12_RESOURCE_DESC m_desc;							// テクスチャの仕様書
		TextureUsage m_useFlg = TextureUsage::None;			// テクスチャの使用方法

		// 実行中のステート
		D3D12_RESOURCE_STATES m_currentSutate = D3D12_RESOURCE_STATE_GENERIC_READ;


		// 使用方法ごとのハンドル
		Engine::Resource::Handle<D3D12::RTV>	 m_rtvHandle{};
		Engine::Resource::Handle<D3D12::DSV>	 m_dsvHandle{};
		Engine::Resource::Handle<D3D12::SRV>	 m_srvHandle{};
		Engine::Resource::Handle<D3D12::UAV>	 m_uavHandle{};

		// ImGui用ハンドル
		Engine::Resource::Handle<D3D12::SRV>	 m_imguiSRVHandle{};

		// 色
		DXSM::Color m_clearValue = {0,0,0,1};
	};
}