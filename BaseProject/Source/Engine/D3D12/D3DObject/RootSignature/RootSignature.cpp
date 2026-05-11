#include "RootSignature.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
namespace Engine::D3D12
{
	RootSignature::RootSignature()
	{

	}
	bool RootSignature::Create(
		const std::vector<std::pair<RootParameterType, std::vector<RangeType>>>& a_rootParamsVec,
		bool a_isUseStaticSampler,
		const D3D12_ROOT_SIGNATURE_FLAGS* a_pFlags
	)
	{
		// パラメーター数
		int _paramCount = static_cast<int>(a_rootParamsVec.size());

		// アプリケーションの入力アセンブラ使用
		D3D12_ROOT_SIGNATURE_FLAGS _flag;
		if (!a_pFlags)
		{
			_flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		}
		else
		{
			_flag = *a_pFlags;
		}

		m_rootParameters.resize(_paramCount);

		UINT _cbvCount = 0;
		UINT _srvCount = 0;
		UINT _uavCount = 0;
		UINT _samplerCount = 0;

		// ルートパラメーター・レンジ作成
		for (size_t _i = 0; _i < _paramCount; ++_i)
		{
			switch (a_rootParamsVec[_i].first)
			{
			case RootParameterType::RootCBV:
				m_rootParameters[_i].first = {};
				m_rootParameters[_i].first.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				m_rootParameters[_i].first.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				m_rootParameters[_i].first.Descriptor.ShaderRegister = _cbvCount;
				m_rootParameters[_i].first.Descriptor.RegisterSpace = 0;
				++_cbvCount;
				break;
			case RootParameterType::RootSRV:
				m_rootParameters[_i].first = {};
				m_rootParameters[_i].first.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				m_rootParameters[_i].first.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
				m_rootParameters[_i].first.Descriptor.ShaderRegister = _srvCount;
				m_rootParameters[_i].first.Descriptor.RegisterSpace = 0;
				++_srvCount;
				break;
			case RootParameterType::DescriptorTable:
			{
				std::vector<D3D12_DESCRIPTOR_RANGE> _ranges(a_rootParamsVec[_i].second.size());
				// レンジ作成
				for (size_t j = 0; j < a_rootParamsVec[_i].second.size(); ++j)
				{
					_ranges[j] = {};
					_ranges[j].NumDescriptors = 1;
					_ranges[j].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
					switch (a_rootParamsVec[_i].second[j])
					{
					case RangeType::CBV:		// 定数バッファビュー
						_ranges[j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						_ranges[j].BaseShaderRegister = _cbvCount;
						_ranges[j].RegisterSpace = 0;
						++_cbvCount;
						break;
					case RangeType::SRV:		// シェーダーリソースビュー
						_ranges[j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						_ranges[j].BaseShaderRegister = _srvCount;
						_ranges[j].RegisterSpace = 0;
						++_srvCount;
						break;
					case RangeType::UAV:		// アンオーダーアクセスビュー
						_ranges[j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						_ranges[j].BaseShaderRegister = _uavCount;
						_ranges[j].RegisterSpace = 0;
						++_uavCount;
						break;
					case RangeType::Sampler:
						_ranges[j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
						_ranges[j].BaseShaderRegister = _samplerCount;
						_ranges[j].RegisterSpace = 0;
						++_samplerCount;
						break;
					default:
						break;
					}
				}
				// ルートパラメーター設定
				D3D12_ROOT_PARAMETER _param = {};
				m_rootParameters[_i].second = _ranges;
				m_rootParameters[_i].first.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				m_rootParameters[_i].first.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				m_rootParameters[_i].first.DescriptorTable.pDescriptorRanges =
					m_rootParameters[_i].second.data();
				m_rootParameters[_i].first.DescriptorTable.NumDescriptorRanges =
					static_cast<UINT>(m_rootParameters[_i].second.size());
			}
			break;
			default:
				break;
			}
		}

		for (auto& param : m_rootParameters)
		{
			// ルートパラメーター配列に追加
			m_rootParams.push_back(param.first);
		}

		// スタティックサンプラーの設定
		auto _sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		// ルートシグネチャの設定（設定したいルートパラメーターとスタティックサンプラーを入れる）
		D3D12_ROOT_SIGNATURE_DESC _desc = {};

		// スタティックサンプラーを使うかどうか
		if (a_isUseStaticSampler)
		{
			_desc.NumParameters = static_cast<UINT>(std::size(m_rootParams));	// ルートパラメーターの個数
			_desc.pParameters = m_rootParams.data();							// ルートパラメーターのポインタを入れる
			_desc.NumStaticSamplers = static_cast<UINT>(1);							// サンプラーの個数を入れる
			_desc.pStaticSamplers = &_sampler;									// サンプラーのポインタを入れる
			_desc.Flags = _flag;										// フラグを設定
		}
		else
		{
			_desc.NumParameters = static_cast<UINT>(std::size(m_rootParams));	// ルートパラメーターの個数
			_desc.pParameters = m_rootParams.data();							// ルートパラメーターのポインタを入れる
			_desc.NumStaticSamplers = 0;											// サンプラーの個数を入れる
			_desc.pStaticSamplers = nullptr;										// サンプラーのポインタを入れる
			_desc.Flags = _flag;										// フラグを設定
		}

		// バイナリデータを保持するための汎用バッファ
		ComPtr<ID3DBlob> _pBlob = nullptr;		// シリアライズ済みルートシグネチャ(GPUに渡す最終バイナリ)を受け取るためのバッファ
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
			return false;
		}

		// ルートシグネチャ生成
		_hr = D3D12Wrapper::Instance().GetDevice()->CreateRootSignature(
			0,												// GPUが複数ある場合のノード（基本一個想定でいいから0）
			_pBlob->GetBufferPointer(),						// シリアライズしたデータのポインタ
			_pBlob->GetBufferSize(),						// シリアライズしたデータのサイズ
			IID_PPV_ARGS(m_pRootSignatrue.GetAddressOf())	// ルートシグネチャ格納先ポインタ
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャの生成に失敗\n");
			return false;
		}

		return true;
	}

	bool RootSignature::Create(const std::vector<std::pair<RootParameterType, std::vector<RangeType>>>& a_rootParamsVec, D3D12_ROOT_SIGNATURE_FLAGS a_flags, bool a_isUseStaticSampler)
	{
		D3D12_ROOT_SIGNATURE_FLAGS _flags;
		_flags = a_flags;

		return Create(
			a_rootParamsVec,
			a_isUseStaticSampler,
			&_flags
		);
	}

	bool RootSignature::Create(RootSigInit a_init)
	{
		// パラメーター数
		int _paramCount = static_cast<int>(a_init.paramVec.size());

		// アプリケーションの入力アセンブラ使用
		D3D12_ROOT_SIGNATURE_FLAGS _flag = a_init.flags;

		m_rootParameters.resize(_paramCount);

		// ルートパラメーター・レンジ作成
		for (size_t _i = 0; _i < _paramCount; ++_i)
		{
			// 参照
			RootParamInit& _param = a_init.paramVec[_i];

			// パラメタータイプによってパラメターを作成
			switch (_param.paramType)
			{
			case RootParameterType::RootCBV:			// ルート定数
				m_rootParameters[_i].first = {};
				m_rootParameters[_i].first.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				m_rootParameters[_i].first.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				m_rootParameters[_i].first.Descriptor.ShaderRegister = _param.shaderRegisterIndex;
				m_rootParameters[_i].first.Descriptor.RegisterSpace = 0;
				break;
			case RootParameterType::RootSRV:			// ルートSRV
				m_rootParameters[_i].first = {};
				m_rootParameters[_i].first.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				m_rootParameters[_i].first.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
				m_rootParameters[_i].first.Descriptor.ShaderRegister = _param.shaderRegisterIndex;
				m_rootParameters[_i].first.Descriptor.RegisterSpace = 0;
				break;
			case RootParameterType::DescriptorTable:	// ディスクリプタテーブル
			{
				std::vector<D3D12_DESCRIPTOR_RANGE> _ranges(_param.rangeVec.size());
				// レンジ作成
				for (size_t j = 0; j < _param.rangeVec.size(); ++j)
				{
					_ranges[j] = {};
					_ranges[j].NumDescriptors = 1;
					_ranges[j].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
					// 参照
					RootRangeInit& _rangeInit = _param.rangeVec[j];

					// レンジタイプごとにレンジ構造体を作成
					switch (_rangeInit.type)
					{
					case RangeType::CBV:		// 定数バッファビュー
						_ranges[j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						_ranges[j].BaseShaderRegister = _rangeInit.shaderRegisterIndex;
						_ranges[j].RegisterSpace = 0;
						break;
					case RangeType::SRV:		// シェーダーリソースビュー
						_ranges[j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						_ranges[j].BaseShaderRegister = _rangeInit.shaderRegisterIndex;
						_ranges[j].RegisterSpace = 0;
						break;
					case RangeType::UAV:		// アンオーダーアクセスビュー
						_ranges[j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						_ranges[j].BaseShaderRegister = _rangeInit.shaderRegisterIndex;
						_ranges[j].RegisterSpace = 0;
						break;
					case RangeType::Sampler:	// サンプラー
						_ranges[j].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
						_ranges[j].BaseShaderRegister = _rangeInit.shaderRegisterIndex;
						_ranges[j].RegisterSpace = 0;
						break;
					default:
						break;
					}
				}
				// ルートパラメーター設定
				D3D12_ROOT_PARAMETER _param = {};
				m_rootParameters[_i].second = _ranges;
				m_rootParameters[_i].first.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				m_rootParameters[_i].first.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				m_rootParameters[_i].first.DescriptorTable.pDescriptorRanges = m_rootParameters[_i].second.data();
				m_rootParameters[_i].first.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(m_rootParameters[_i].second.size());
				break;
			}
			default:
				break;
			}
		}

		for (auto& param : m_rootParameters)
		{
			// ルートパラメーター配列に追加
			m_rootParams.push_back(param.first);
		}

		// スタティックサンプラーの設定
		auto _sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		// ルートシグネチャの設定（設定したいルートパラメーターとスタティックサンプラーを入れる）
		D3D12_ROOT_SIGNATURE_DESC _desc = {};

		// スタティックサンプラーを使うかどうか
		if (a_init.isUseStaticSampler)
		{
			_desc.NumParameters = static_cast<UINT>(std::size(m_rootParams));	// ルートパラメーターの個数
			_desc.pParameters = m_rootParams.data();							// ルートパラメーターのポインタを入れる
			_desc.NumStaticSamplers = static_cast<UINT>(1);							// サンプラーの個数を入れる
			_desc.pStaticSamplers = &_sampler;									// サンプラーのポインタを入れる
			_desc.Flags = _flag;										// フラグを設定
		}
		else
		{
			_desc.NumParameters = static_cast<UINT>(std::size(m_rootParams));	// ルートパラメーターの個数
			_desc.pParameters = m_rootParams.data();							// ルートパラメーターのポインタを入れる
			_desc.NumStaticSamplers = 0;											// サンプラーの個数を入れる
			_desc.pStaticSamplers = nullptr;										// サンプラーのポインタを入れる
			_desc.Flags = _flag;										// フラグを設定
		}

		// バイナリデータを保持するための汎用バッファ
		ComPtr<ID3DBlob> _pBlob = nullptr;		// シリアライズ済みルートシグネチャ(GPUに渡す最終バイナリ)を受け取るためのバッファ
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
			return false;
		}

		// ルートシグネチャ生成
		_hr = D3D12Wrapper::Instance().GetDevice()->CreateRootSignature(
			0,												// GPUが複数ある場合のノード（基本一個想定でいいから0）
			_pBlob->GetBufferPointer(),						// シリアライズしたデータのポインタ
			_pBlob->GetBufferSize(),						// シリアライズしたデータのサイズ
			IID_PPV_ARGS(m_pRootSignatrue.GetAddressOf())	// ルートシグネチャ格納先ポインタ
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャの生成に失敗\n");
			return false;
		}

		return true;
	}

	
	bool RootSignature::IsValid()
	{
		return m_isValid;
	}

	ID3D12RootSignature* RootSignature::Get()
	{
		return m_pRootSignatrue.Get();
	}
}