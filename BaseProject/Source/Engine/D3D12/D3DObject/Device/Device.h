#pragma once

class Device
{
public:

	Device(){}
	~Device(){}

	// デバイスの初期化
	bool Init(bool a_isDebug,bool a_isDynamic = true);
	void Release();

	// ゲッター
	ID3D12Device5* GetDevice();			// デバイス取得
	IDXGIFactory6* GetDxgiFactory();	// DXGIファクトリ取得


private:

	bool CreateDevice(bool a_isDynamic);				// デバイスの生成
	bool CreateDxgiFactory();			// DXGIファクトリの生成

private:

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

	ComPtr<ID3D12Device5> m_cpDevice5 = nullptr;	// GPUデバイス
	ComPtr<IDXGIFactory6> m_cpDxgFactory = nullptr;	// DXGIファクトリ


	bool m_isDebug = false;

	bool m_isDynamicResourceSupported = false;		// 動的リソースがサポートされているか
};