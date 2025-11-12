#pragma once

class Device
{
public:

	Device(){}
	~Device(){}

	// デバイスの初期化
	bool Init();

	// ゲッター
	ID3D12Device8* GetDevice();			// デバイス取得
	IDXGIFactory6* GetDxgiFactory();	// DXGIファクトリ取得


private:

	bool CreateDevice();				// デバイスの生成
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

	ComPtr<ID3D12Device8> m_pDevice = nullptr;		// GPUデバイス
	ComPtr<IDXGIFactory6> m_pDxgFactory = nullptr;	// DXGIファクトリ

};