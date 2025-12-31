#include "RootSignature.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

RootSignature::RootSignature()
{
	
}

bool RootSignature::Create(std::vector<RangeType> a_rangeTypeVec)
{
	// 指定したレンジ数
	int _rangeCount = a_rangeTypeVec.size();

	// アプリケーションの入力アセンブラ使用
	auto _flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	//std::vector<D3D12_ROOT_PARAMETER> _rootParams(_rangeCount);
	//std::vector<D3D12_DESCRIPTOR_RANGE> _ranges(_rangeCount);
	_rootParams.resize(_rangeCount);
	_ranges.resize(_rangeCount);

	UINT _cbvCount = 0;
	UINT _srvCount = 0;
	UINT _uavCount = 0;

	// ルートパラメーター・レンジ作成
	for (size_t _i = 0; _i < _rangeCount; ++_i)
	{
		_rootParams[_i] = {};
		_ranges[_i] = {};

		_ranges[_i].NumDescriptors = 1;
		_ranges[_i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		switch (a_rangeTypeVec[_i])
		{
		case RangeType::CBV:		// 定数バッファビュー
			_ranges[_i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			_ranges[_i].BaseShaderRegister = _cbvCount;
			_ranges[_i].RegisterSpace = 0;

			/*_rootParams[_i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			_rootParams[_i].Descriptor.ShaderRegister = _cbvCount;
			_rootParams[_i].Descriptor.RegisterSpace = 0;*/

			_rootParams[_i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			_rootParams[_i].DescriptorTable.pDescriptorRanges = &_ranges[_i];
			_rootParams[_i].DescriptorTable.NumDescriptorRanges = 1;
			++_cbvCount;
			break;
		case RangeType::SRV:		// シェーダーリソースビュー
			_ranges[_i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			_ranges[_i].BaseShaderRegister = _srvCount;
			_ranges[_i].RegisterSpace = 0;

			_rootParams[_i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			_rootParams[_i].DescriptorTable.pDescriptorRanges = &_ranges[_i];
			_rootParams[_i].DescriptorTable.NumDescriptorRanges = 1;
			++_srvCount;
			break;
		case RangeType::UAV:		// アンオーダーアクセスビュー
			_ranges[_i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			_ranges[_i].BaseShaderRegister = _uavCount;
			_ranges[_i].RegisterSpace = 0;
			_rootParams[_i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			_rootParams[_i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			_rootParams[_i].DescriptorTable.pDescriptorRanges = &_ranges[_i];
			_rootParams[_i].DescriptorTable.NumDescriptorRanges = 1;
			++_uavCount;
			break;
		default:
			break;
		}


		
	}

	// スタティックサンプラーの設定
	auto _sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	// ルートシグネチャの設定（設定したいルートパラメーターとスタティックサンプラーを入れる）
	D3D12_ROOT_SIGNATURE_DESC _desc = {};
	_desc.NumParameters = std::size(_rootParams);	// ルートパラメーターの個数を入れる
	_desc.NumStaticSamplers = 1;								// サンプラーの個数を入れる
	_desc.pParameters = _rootParams.data();				// ルートパラメーターのポインタを入れる
	_desc.pStaticSamplers = &_sampler;						// サンプラーのポインタを入れる
	_desc.Flags = _flag;												// フラグを設定

	// バイナリデータを保持するための汎用バッファ
	ComPtr<ID3DBlob> _pBlob;		// シリアライズ済みルートシグネチャ(GPUに渡す最終バイナリ)を受け取るためのバッファ
	ComPtr<ID3DBlob> _pErrorBlob;	// シリアライズに失敗したときのエラーメッセージが入るバッファ	

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
	_hr = RenderingEngine::Instance().GetDevice()->CreateRootSignature(
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
