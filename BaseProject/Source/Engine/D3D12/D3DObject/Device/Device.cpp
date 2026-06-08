#include "Device.h"
namespace Engine::D3D12
{
	bool Device::Init(bool a_isDebug, bool a_isDynamic)
	{
		// デバッグフラグ
		m_isDebug = a_isDebug;

		// ファクトリを生成して、アダプタをみつけて、デバイスを生成する
		if (!CreateDxgiFactory())
		{
			assert(0 && "DXGIファクトリの生成に失敗");
			return false;
		}
		if (!CreateDevice(a_isDynamic))
		{
			assert(0 && "デバイスの生成に失敗");
			return false;
		}

		if (m_isDebug)
		{
			ComPtr<ID3D12DebugDevice> _debDev;
			if (SUCCEEDED(m_cpDevice5->QueryInterface(IID_PPV_ARGS(&_debDev))))
			{
				_debDev->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
			}
		}

		return true;
	}

	void Device::Release()
	{
		if (m_isDebug)
		{
			ComPtr<ID3D12DebugDevice> _dd;
			m_cpDevice5->QueryInterface(IID_PPV_ARGS(&_dd));
			_dd->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		}

		if (m_cpDXGIAdapter)
		{
			m_cpDXGIAdapter.Reset();
		}
		if (m_cpDxgFactory)
		{
			m_cpDxgFactory.Reset();
		}
		if (m_cpDevice5)
		{
			m_cpDevice5.Reset();
		}
	}


	ID3D12Device5* Device::GetDevice()
	{
		return m_cpDevice5.Get();
	}
	IDXGIFactory6* Device::GetDxgiFactory()
	{
		return m_cpDxgFactory.Get();
	}

	IDXGIAdapter* Device::GetAdapter()
	{
		return m_cpDXGIAdapter.Get();
	}


	bool Device::CreateDevice(bool a_isDynamic)
	{
		m_cpDXGIAdapter = nullptr;							// 選択アダプタ
		std::vector<ComPtr<IDXGIAdapter>> _pAdapters;		// 発見したアダプタ群
		std::vector<DXGI_ADAPTER_DESC> _adapterDescs;		// アダプタの説明群

		// 使用中PCにあるGPUドライバーを検索して、アダプタを列挙
		for (UINT _i = 0; 1; ++_i)
		{
			// デバイスを検索し、見つかれば格納
			_pAdapters.push_back(nullptr);
			HRESULT _hr = m_cpDxgFactory->EnumAdapters(_i, &_pAdapters[_i]);

			// デバイスが見つからなければ終了
			if (_hr == DXGI_ERROR_NOT_FOUND) break;

			// 見つけたデバイスの説明を取得
			_adapterDescs.push_back({});
			_pAdapters[_i]->GetDesc(&_adapterDescs[_i]);
		}

		// 優先度の高いGPUドライバーを使用する
		GPUTier _guiTier = GPUTier::Kind;
		for (int _i = 0; _i < _adapterDescs.size(); ++_i)
		{
			if (std::wstring(_adapterDescs[_i].Description).find(L"NVIDIA") != std::wstring::npos)
			{
				// NVIDIAが見つかったら即決
				m_cpDXGIAdapter = _pAdapters[_i];
				break;
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Amd") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Amd)
				{
					m_cpDXGIAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Amd;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Intel") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Intel)
				{
					m_cpDXGIAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Intel;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Arm") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Arm)
				{
					m_cpDXGIAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Arm;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Qualcomm") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Qualcomm)
				{
					m_cpDXGIAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Qualcomm;
				}
			}
		}

		D3D_FEATURE_LEVEL _levels[] =
		{
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};

		// デバイスの生成
		HRESULT _hr = S_FALSE;
		for (auto _level : _levels)
		{
			// 生成可能なレベルで試す
			_hr = D3D12CreateDevice(
				m_cpDXGIAdapter.Get(),
				_level,
				IID_PPV_ARGS(m_cpDevice5.ReleaseAndGetAddressOf())
			);
			if (SUCCEEDED(_hr))
			{
				// 成功したら抜ける
				break;
			}
		}
		if (FAILED(_hr))
		{
			assert(0 && "デバイスの生成に失敗");
			return false;
		}

		//m_cpDevice.As(&m_cpDevice5);

		m_isDynamicResourceSupported = false;
		if (a_isDynamic)
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS _featureOptions = {};
			D3D12_FEATURE_DATA_SHADER_MODEL _shaderModel = {};
			_shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
			if (SUCCEEDED(m_cpDevice5.Get()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &_featureOptions, sizeof(_featureOptions)))
				&& SUCCEEDED(m_cpDevice5.Get()->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &_shaderModel, sizeof(_shaderModel))))
			{
				bool _isTier3 = _featureOptions.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3;
				m_isDynamicResourceSupported = _isTier3;
			}
		}
		if (!m_isDynamicResourceSupported)
		{
			assert(0 && "動的リソースがサポートされていないGPUが選択されました。");
		}


		return true;
	}
	bool Device::CreateDxgiFactory()
	{
		UINT _flgsDXGI = 0;
#ifdef _DEBUG
		// デバッグレイヤーを有効化
		_flgsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
#endif

		// DXGIファクトリの生成
		HRESULT _hr = CreateDXGIFactory2(
			_flgsDXGI,
			IID_PPV_ARGS(m_cpDxgFactory.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			assert(0 && "DXGIファクトリの生成に失敗");
			return false;
		}

		// 成功
		return true;
	}
}