#pragma once

namespace Engine::Resource
{


	class Texture
	{
	public:
		Texture() = default;
		~Texture() = default;

		// テクスチャ生成
		void Import(
			const std::string& a_filePath,
			const DirectX::XMFLOAT4& a_defoltData = { 255,255,255,255 }
		);
		void Create(
			const std::string& a_name,
			const UINT64& a_width,
			const UINT& a_height,
			const DXGI_FORMAT& a_format,
			const TextureUsage& a_usage
		);
		
		// 名前変更
		void SetName(const std::string& a_name);
		const std::string& GetName();

		// リソース情報
		ID3D12Resource* GetResource();				// 生データ
		const TextureUsage& GetUsage() const;		// 使用フラグ
		const D3D12_RESOURCE_DESC& GetDesc() const;	// テクスチャ設定

		// ステート
		D3D12_RESOURCE_STATES GetState() { return m_currentSutate; }
		void ChangeState(D3D12_RESOURCE_STATES a_state) { m_currentSutate = a_state; }

		// ビュー情報取得
		const Engine::Resource::Handle<RTV>& GetRTV() const;
		const Engine::Resource::Handle<DSV>& GetDSV() const;
		const Engine::Resource::Handle<SRV>& GetSRV() const;
		const Engine::Resource::Handle<UAV>& GetUAV() const;
		const Engine::Resource::Handle<SRV>& GetImGuiSRV() const;

		// ビュー情報セット
		void SetRTV(const Engine::Resource::Handle<RTV>& a_handle);
		void SetDSV(const Engine::Resource::Handle<DSV>& a_handle);
		void SetSRV(const Engine::Resource::Handle<SRV>& a_handle);
		void SetUAV(const Engine::Resource::Handle<UAV>& a_handle);
		void SetImGuiSRV(const Engine::Resource::Handle<SRV>& a_handle);

	private:

		// リソース
		std::string m_name = "none";						// テクスチャの名前
		ComPtr<ID3D12Resource> m_cpResource = nullptr;		// テクスチャ本体			
		D3D12_RESOURCE_DESC m_desc;							// テクスチャの仕様書
		TextureUsage m_useFlg = TextureUsage::None;			// テクスチャの使用方法

		// 実行中のステート
		D3D12_RESOURCE_STATES m_currentSutate = D3D12_RESOURCE_STATE_GENERIC_READ;


		// 使用方法ごとのハンドル
		Engine::Resource::Handle<RTV>	 m_rtvHandle{};
		Engine::Resource::Handle<DSV>	 m_dsvHandle{};
		Engine::Resource::Handle<SRV>	 m_srvHandle{};
		Engine::Resource::Handle<UAV>	 m_uavHandle{};

		// ImGui用ハンドル
		Engine::Resource::Handle<SRV>	 m_imguiSRVHandle{};
	};
}