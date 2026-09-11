#pragma once
namespace Engine::Graphics
{
	//==========================================================================================
	// デバイス(ファクトリ・アダプター・デバイス)
	//
	// GraphicsEngine が持つ。作るのは何よりも先、捨てるのは何よりも後。
	// 他のD3Dオブジェクトはすべてこのデバイスの子なので、
	// 解放は GraphicsEngine::ReleaseDevice() から最後に通すこと
	//==========================================================================================
	class GraphicsDevice
	{
	public:

		GraphicsDevice() = default;
		~GraphicsDevice() { Release(); }

		// 作成
		void Create(bool a_isDebug);

		// 解放 : 二度呼んでも何もしない(デストラクタからも通る)
		void Release();

		// ---- アクセサ ----
		D3D12::Device* RefDevice() { return m_cpDevice.Get(); }
		D3D12::Factory* RefFactory() { return m_cpFactory.Get(); }
		D3D12::Adapter* RefAdapter() { return m_cpAdapter.Get(); }

		bool IsDynamicResourceSupported() const { return m_isDynamicResourceSupported; }

	private:

		// ---- D3Dオブジェクト作成 ----
		// デバイス関係
		void CreateDxgiFactory();	// DXGIファクトリ作成
		void FindAdapter();			// GPU検索
		void CreateDevice();		// デバイス作成

		// 優先度順デバイスメーカー
		enum class GPUTier
		{
			NVIDIA,
			Amd,
			Intel,
			Arm,
			Qualcomm,
			Kind,
		};

	private:

		// デバイス関係
		ComPtr<D3D12::Device>	m_cpDevice = nullptr;		// ドライバインスタンス
		ComPtr<D3D12::Factory>	m_cpFactory = nullptr;		// ファクトリー
		ComPtr<D3D12::Adapter>	m_cpAdapter = nullptr;		// GPU実体

		// プロジェクト仕様
		bool m_isDebug = false;								// デバッグモードで起動されたかどうか
		bool m_isDynamicResourceSupported = false;			// ダイナミックリソースが使えるかどうか
	};
}
