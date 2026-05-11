#include "RootSignatureBuilder.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
namespace Engine::D3D12
{
	D3D12_ROOT_SIGNATURE_DESC RootSignatureBuilder::CreateDesc(const RootSignatureDesc& a_desc)
	{
		// 変数準備
		int _paramCount = static_cast<int>(a_desc.paramVec.size());						// パラメーター数
		std::vector<D3D12_ROOT_PARAMETER> _rootParams(_paramCount);						// パラメーター変数
		std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> _rangeStoreage(_paramCount);	// レンジ配列
		std::vector<std::pair<D3D12_ROOT_PARAMETER, std::vector<D3D12_DESCRIPTOR_RANGE>>> _rootParameters;
		_rootParameters.resize(_paramCount);

		// ルートパラメーター・レンジ作成
		for (size_t _i = 0; _i < _paramCount; ++_i)
		{
			// 全部に共通する設定
			const auto& _paramInit = a_desc.paramVec[_i];
			_rootParams[_i] = {};
			_rootParams[_i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			// パラメタータイプによってパラメターを作成
			switch (_paramInit.paramType)
			{
			case RootParameterType::RootCBV:			// ルート定数
				_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				_rootParams[_i].Descriptor.ShaderRegister = _paramInit.shaderRegisterIndex;
				_rootParams[_i].Descriptor.RegisterSpace = 0;
				break;
			case RootParameterType::RootSRV:			// ルートSRV
				_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
				_rootParams[_i].Descriptor.ShaderRegister = _paramInit.shaderRegisterIndex;
				_rootParams[_i].Descriptor.RegisterSpace = 0;
				break;
			case RootParameterType::DescriptorTable:	// ディスクリプタテーブル
			{
				// レンジ配列参照
				auto& _ranges = _rangeStoreage[_i];
				_ranges.resize(_paramInit.rangeVec.size());

				// レンジ作成
				for (size_t _j = 0; _j < _paramInit.rangeVec.size(); ++_j)
				{
					const auto& _rangeInit = _paramInit.rangeVec[_j];
					_ranges[_j] = {};
					_ranges[_j].NumDescriptors = 1;
					_ranges[_j].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
					_ranges[_j].RegisterSpace = 0;
					_ranges[_j].BaseShaderRegister = _rangeInit.shaderRegisterIndex;
					// 参照

					// レンジタイプごとにレンジ構造体を作成
					switch (_rangeInit.type)
					{
					case RangeType::CBV:		// 定数バッファビュー
						_ranges[_j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						break;
					case RangeType::SRV:		// シェーダーリソースビュー
						_ranges[_j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						break;
					case RangeType::UAV:		// アンオーダーアクセスビュー
						_ranges[_j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						break;
					case RangeType::Sampler:	// サンプラー
						_ranges[_j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
						break;
					}
				}
				// パラメーターにレンジ配列の先頭ポインタをセット
				_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				_rootParams[_i].DescriptorTable.pDescriptorRanges = _ranges.data();
				_rootParams[_i].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(_ranges.size());
				break;
			}
			default:
				break;
			}
		}

		// スタティックサンプラーの設定
		auto _sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		// ルートシグネチャの設定（設定したいルートパラメーターとスタティックサンプラーを入れる）
		D3D12_ROOT_SIGNATURE_DESC _desc = {};
		_desc.NumParameters = static_cast<UINT>(_rootParams.size());
		_desc.pParameters = _rootParams.empty() ? nullptr : _rootParams.data();
		_desc.Flags = a_desc.flags;
		// スタティックサンプラーを使うかどうか
		if (a_desc.isUseStaticSampler)
		{
			_desc.NumStaticSamplers = 1;
			_desc.pStaticSamplers = &_sampler;
		}
		else
		{
			_desc.NumStaticSamplers = 0;
			_desc.pStaticSamplers = nullptr;
		}
		return _desc;
	}
	ComPtr<ID3D12RootSignature> RootSignatureBuilder::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& a_desc)
	{
		// バイナリデータを保持するための汎用バッファ
		ComPtr<ID3DBlob> _pBlob = nullptr;		// シリアライズ済みルートシグネチャ(GPUに渡す最終バイナリ)
		ComPtr<ID3DBlob> _pErrorBlob = nullptr;	// シリアライズに失敗したときのエラーメッセージが入るバッファ	

		// シリアライズ
		auto _hr = D3D12SerializeRootSignature(
			&a_desc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			_pBlob.GetAddressOf(),
			_pErrorBlob.GetAddressOf()
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャシリアライズに失敗");
			return nullptr;
		}

		// ルートシグネチャ生成
		ComPtr<ID3D12RootSignature> _pRootSignature = nullptr;
		_hr = D3D12Wrapper::Instance().GetDevice()->CreateRootSignature(
			0,												// GPUが複数ある場合のノード（基本一個想定でいいから0）
			_pBlob->GetBufferPointer(),						// シリアライズしたデータのポインタ
			_pBlob->GetBufferSize(),						// シリアライズしたデータのサイズ
			IID_PPV_ARGS(_pRootSignature.GetAddressOf())	// ルートシグネチャ格納先ポインタ
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャの生成に失敗\n");
			return nullptr;
		}

		return _pRootSignature;
	}
	ComPtr<ID3D12RootSignature> Engine::D3D12::RootSignatureBuilder::Create(const RootSignatureDesc& a_desc)
	{
		// 変数準備
		int _paramCount = static_cast<int>(a_desc.paramVec.size());						// パラメーター数
		std::vector<D3D12_ROOT_PARAMETER> _rootParams(_paramCount);						// パラメーター変数
		std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> _rangeStoreage(_paramCount);	// レンジ配列

		// ルートパラメーター・レンジ作成
		for (size_t _i = 0; _i < _paramCount; ++_i)
		{
			// 全部に共通する設定
			const auto& _paramInit = a_desc.paramVec[_i];
			_rootParams[_i] = {};
			_rootParams[_i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			// パラメタータイプによってパラメターを作成
			switch (_paramInit.paramType)
			{
			case RootParameterType::RootCBV:			// ルート定数
				_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				_rootParams[_i].Descriptor.ShaderRegister = _paramInit.shaderRegisterIndex;
				_rootParams[_i].Descriptor.RegisterSpace = 0;
				break;
			case RootParameterType::RootSRV:			// ルートSRV
				_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
				_rootParams[_i].Descriptor.ShaderRegister = _paramInit.shaderRegisterIndex;
				_rootParams[_i].Descriptor.RegisterSpace = 0;
				break;
			case RootParameterType::DescriptorTable:	// ディスクリプタテーブル
			{
				// レンジ配列参照
				auto& _ranges = _rangeStoreage[_i];
				_ranges.resize(_paramInit.rangeVec.size());

				// レンジ作成
				for (size_t _j = 0; _j < _paramInit.rangeVec.size(); ++_j)
				{
					const auto& _rangeInit = _paramInit.rangeVec[_j];
					_ranges[_j] = {};
					_ranges[_j].NumDescriptors = 1;
					_ranges[_j].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
					_ranges[_j].RegisterSpace = 0;
					_ranges[_j].BaseShaderRegister = _rangeInit.shaderRegisterIndex;
					// 参照
					
					// レンジタイプごとにレンジ構造体を作成
					switch (_rangeInit.type)
					{
					case RangeType::CBV:		// 定数バッファビュー
						_ranges[_j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						break;
					case RangeType::SRV:		// シェーダーリソースビュー
						_ranges[_j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						break;
					case RangeType::UAV:		// アンオーダーアクセスビュー
						_ranges[_j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						break;
					case RangeType::Sampler:	// サンプラー
						_ranges[_j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
						break;
					}
				}
				// パラメーターにレンジ配列の先頭ポインタをセット
				_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				_rootParams[_i].DescriptorTable.pDescriptorRanges = _ranges.data();
				_rootParams[_i].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(_ranges.size());
				break;
			}
			default:
				break;
			}
		}

		// スタティックサンプラーの設定
		auto _sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		// ルートシグネチャの設定（設定したいルートパラメーターとスタティックサンプラーを入れる）
		D3D12_ROOT_SIGNATURE_DESC _desc = {};
		_desc.NumParameters = static_cast<UINT>(_rootParams.size());
		_desc.pParameters = _rootParams.empty() ? nullptr : _rootParams.data();
		_desc.Flags = a_desc.flags;
		// スタティックサンプラーを使うかどうか
		if (a_desc.isUseStaticSampler)
		{
			_desc.NumStaticSamplers = 1;
			_desc.pStaticSamplers = &_sampler;
		}
		else
		{
			_desc.NumStaticSamplers = 0;
			_desc.pStaticSamplers = nullptr;
		}

		// バイナリデータを保持するための汎用バッファ
		ComPtr<ID3DBlob> _pBlob = nullptr;		// シリアライズ済みルートシグネチャ(GPUに渡す最終バイナリ)
		ComPtr<ID3DBlob> _pErrorBlob = nullptr;	// シリアライズに失敗したときのエラーメッセージが入るバッファ	

		// シリアライズ
		auto _hr = D3D12SerializeRootSignature(
			&_desc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			_pBlob.GetAddressOf(),
			_pErrorBlob.GetAddressOf()
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャシリアライズに失敗");
			return nullptr;
		}

		// ルートシグネチャ生成
		ComPtr<ID3D12RootSignature> _pRootSignature = nullptr;
		_hr = D3D12Wrapper::Instance().GetDevice()->CreateRootSignature(
			0,												// GPUが複数ある場合のノード（基本一個想定でいいから0）
			_pBlob->GetBufferPointer(),						// シリアライズしたデータのポインタ
			_pBlob->GetBufferSize(),						// シリアライズしたデータのサイズ
			IID_PPV_ARGS(_pRootSignature.GetAddressOf())	// ルートシグネチャ格納先ポインタ
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャの生成に失敗\n");
			return nullptr;
		}

		return _pRootSignature;
	}
	ComPtr<ID3D12RootSignature> RootSignatureBuilder::Create(const std::string& a_path)
	{
		// バイナリデータを保持するための汎用バッファ
		ComPtr<ID3DBlob> _pBlob = nullptr;		// シリアライズ済みルートシグネチャ(GPUに渡す最終バイナリ)
		ComPtr<ID3DBlob> _pErrorBlob = nullptr;	// シリアライズに失敗したときのエラーメッセージが入るバッファ	

		// シェーダーバイトコードの読み込み
		auto _hr = D3DReadFileToBlob(
			StringUtility::ToWideString(a_path).c_str(),
			_pBlob.ReleaseAndGetAddressOf()
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャ生成用のシェーダーファイル読み込みに失敗");
			return nullptr;
		}


		// ルートシグネチャ生成
		ComPtr<ID3D12RootSignature> _pRootSignature = nullptr;
		_hr = D3D12Wrapper::Instance().GetDevice()->CreateRootSignature(
			0,												// GPUが複数ある場合のノード（基本一個想定でいいから0）
			_pBlob->GetBufferPointer(),						// シリアライズしたデータのポインタ
			_pBlob->GetBufferSize(),						// シリアライズしたデータのサイズ
			IID_PPV_ARGS(_pRootSignature.GetAddressOf())	// ルートシグネチャ格納先ポインタ
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャの生成に失敗\n");
			return nullptr;
		}

		return _pRootSignature;
	}
	ComPtr<ID3D12RootSignature> RootSignatureBuilder::Create(ComPtr<ID3DBlob> a_cpBlob)
	{
		ComPtr<ID3DBlob> _pErrorBlob = nullptr;	// シリアライズに失敗したときのエラーメッセージが入るバッファ	

		// ルートシグネチャ生成
		ComPtr<ID3D12RootSignature> _pRootSignature = nullptr;
		auto _hr = D3D12Wrapper::Instance().GetDevice()->CreateRootSignature(
			0,												// GPUが複数ある場合のノード（基本一個想定でいいから0）
			a_cpBlob->GetBufferPointer(),						// シリアライズしたデータのポインタ
			a_cpBlob->GetBufferSize(),						// シリアライズしたデータのサイズ
			IID_PPV_ARGS(_pRootSignature.GetAddressOf())	// ルートシグネチャ格納先ポインタ
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャの生成に失敗\n");
			return nullptr;
		}

		return _pRootSignature;
	}
}