#pragma once
namespace Engine::D3D12
{
	class Device
	{
	public:

		Device() {}
		~Device() { Release(); }

		// デバイスの初期化
		bool Init(bool a_isDebug, bool a_isDynamic = true);
		void Release();

		// ゲッター
		ID3D12Device5* GetDevice();			// デバイス取得
		IDXGIFactory6* GetDxgiFactory();	// DXGIファクトリ取得
		IDXGIAdapter* GetAdapter();		// アダプタ取得


	private:

		bool CreateDevice(bool a_isDynamic);				// デバイスの生成
		bool CreateDxgiFactory();			// DXGIファクトリの生成

	private:

		

		ComPtr<ID3D12Device5> m_cpDevice5 = nullptr;	// GPUデバイス
		ComPtr<IDXGIFactory6> m_cpDxgFactory = nullptr;	// DXGIファクトリ
		ComPtr<IDXGIAdapter> m_cpDXGIAdapter = nullptr;	// 選択したアダプタ

		bool m_isDebug = false;

		bool m_isDynamicResourceSupported = false;		// 動的リソースがサポートされているか
	};
}