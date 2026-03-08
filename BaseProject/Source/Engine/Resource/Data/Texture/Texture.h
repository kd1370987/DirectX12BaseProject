#pragma once

namespace Engine::Resource
{


	class TextureRes
	{
	public:
		TextureRes() = default;
		~TextureRes() = default;

		// テクスチャ生成
		void Import(
			const std::string& a_filePath,
			const DirectX::XMFLOAT4& a_defoltData = { 255,255,255,255 }
		);
		void Create(
			const UINT64& a_width,
			const UINT& a_height,
			const DXGI_FORMAT& a_format,
			const TextureUsage& a_usage
		);
		
		// 名前変更
		void SetName(const std::string& a_name);

		// リソース取得
		ID3D12Resource* GetResource();

	private:

		// リソース
		std::string m_name = "none";						// テクスチャの名前
		ComPtr<ID3D12Resource> m_cpResource = nullptr;		// テクスチャ本体			
		D3D12_RESOURCE_DESC m_desc;							// テクスチャの仕様書
		TextureUsage m_useFlg = TextureUsage::None;			// テクスチャの使用方法

		// 使用方法ごとのハンドル
		RTVHandle		 m_rtvHandle{};
		DSVHandle		 m_dsvHandle{};
		Storage::Range	 m_srvHandle;
		Storage::Range	 m_uavHandle;

		// ImGui用ハンドル
		Storage::Range m_imguiSRVHandle{};
	};
}