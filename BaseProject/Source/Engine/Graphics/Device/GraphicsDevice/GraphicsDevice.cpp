#include "GraphicsDevice.h"
namespace Engine::Graphics
{
	void GraphicsDevice::Create(bool a_isDebug)
	{
		m_isDebug = a_isDebug;

		// GPUリソース初期化
		CreateDxgiFactory();	// ファクトリ作成
		FindAdapter();			// アダプター検索	
		CreateDevice();			// デバイス作成
	}
	void GraphicsDevice::Release()
	{
		// 解放済み : デストラクタから二度目が来ても何もしない
		if (!m_cpDevice) return;

		// アダプターファクトリー解放
		m_cpAdapter.Reset();
		m_cpFactory.Reset();

		// リーク調査 : デバイスがまだ生きているこのタイミングで
		// ID3D12DebugDevice のレポートを出す。
		// SetName で付けた名前が出るので、どのリソースが残っているか特定できる。
		// D3D12_RLDO_IGNORE_INTERNAL でランタイム内部の参照だけのものは除く
		if (m_isDebug)
		{
			ComPtr<ID3D12DebugDevice> _cpDebDev;
			if (SUCCEEDED(m_cpDevice->QueryInterface(IID_PPV_ARGS(&_cpDebDev))))
			{
				_cpDebDev->ReportLiveDeviceObjects(
					D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL
				);
			}
		}

		// 最後にデバイスを解放
		m_cpDevice.Reset();
	}
	void GraphicsDevice::CreateDxgiFactory()
	{
		UINT _flgsDXGI = 0;
		if (m_isDebug)
		{
			// デバッグレイヤーを有効化
			_flgsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
			ENGINE_LOG("DXGIFactoryのデバッグレイヤーON");
		}
		// DXGIファクトリの生成
		HRESULT _hr = CreateDXGIFactory2(
			_flgsDXGI,
			IID_PPV_ARGS(m_cpFactory.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(FAILED(_hr), "DXGIファクトリの生成に失敗");
			return;
		}
	}
	void GraphicsDevice::FindAdapter()
	{
		m_cpAdapter = nullptr;								// クリア
		std::vector<ComPtr<IDXGIAdapter>> _pAdapters;		// 発見したアダプタ群
		std::vector<DXGI_ADAPTER_DESC> _adapterDescs;		// アダプタの説明群

		// 使用中PCにあるGPUドライバーを検索して、アダプタを列挙
		for (UINT _i = 0; 1; ++_i)
		{
			// デバイスを検索し、見つかれば格納
			_pAdapters.push_back(nullptr);
			HRESULT _hr = m_cpFactory->EnumAdapters(_i, &_pAdapters[_i]);

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
				m_cpAdapter = _pAdapters[_i];
				break;
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Amd") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Amd)
				{
					m_cpAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Amd;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Intel") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Intel)
				{
					m_cpAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Intel;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Arm") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Arm)
				{
					m_cpAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Arm;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Qualcomm") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Qualcomm)
				{
					m_cpAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Qualcomm;
				}
			}
		}
	}
	void GraphicsDevice::CreateDevice()
	{
		// 検索用レベル
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
				m_cpAdapter.Get(),
				_level,
				IID_PPV_ARGS(m_cpDevice.ReleaseAndGetAddressOf())
			);
			if (SUCCEEDED(_hr))
			{
				// 成功したら抜ける
				break;
			}
		}
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(FAILED(_hr), "デバイス生成に失敗");
			return;
		}


		// DynamicResourceBindが対応されているかのチェック
		m_isDynamicResourceSupported = false;

		D3D12_FEATURE_DATA_D3D12_OPTIONS	_featureOptions = {};
		D3D12_FEATURE_DATA_SHADER_MODEL		_shaderModel = {};
		_shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		if (SUCCEEDED(m_cpDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &_featureOptions, sizeof(_featureOptions)))
			&& SUCCEEDED(m_cpDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &_shaderModel, sizeof(_shaderModel))))
		{
			bool _isTier3 = _featureOptions.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3;
			m_isDynamicResourceSupported = _isTier3;
		}
		ENGINE_ERRLOG(m_isDynamicResourceSupported, "動的リソースがサポートされていないGPUが選択されました");

		// デバッグ設定
		if (m_isDebug)
		{
			ComPtr<ID3D12DebugDevice> _debDev;
			if (SUCCEEDED(m_cpDevice->QueryInterface(IID_PPV_ARGS(&_debDev))))
			{
				_debDev->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
			}
			ENGINE_LOG("Dviceのデバッグ設定ON");
		}

		return;
	}
}