#include "RootSignature.h"

#include "../../../Engine/Core/RenderingEngine.h"

RootSignature::RootSignature()
{
	auto _flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	// アプリケーションの入力アセンブラ使用
	// ルートシグネチャへんアクセスを拒否する
	_flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;			// ドメインシェーダー
	_flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;			// ハルシェーダー
	_flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;		// ジオメトリシェーダー

	CD3DX12_ROOT_PARAMETER _rootParam[1] = {};
	// b0の定数バッファを設定、全てのシェーダーから見えるようにする
	_rootParam[0].InitAsConstantBufferView(0,0,D3D12_SHADER_VISIBILITY_ALL);

	// スタティックサンプラーの設定
	auto _sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	// ルートシグネチャの設定（設定したいルートパラメーターとスタティックサンプラーを入れる）
	D3D12_ROOT_SIGNATURE_DESC _desc = {};
	_desc.NumParameters = std::size(_rootParam);	// ルートパラメーターの個数を入れる
	_desc.NumStaticSamplers = 1;					// サンプラーの個数を入れる
	_desc.pParameters = _rootParam;					// ルートパラメーターのポインタを入れる
	_desc.pStaticSamplers = &_sampler;				// サンプラーのポインタを入れる
	_desc.Flags = _flag;							// フラグを設定

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
		printf("ルートシグネチャシリアライズに失敗");
		return;
	}

	// ルートシグネチャ生成
	_hr = RenderingEngine::Instance().Device()->CreateRootSignature(
		0,												// GPUが複数ある場合のノード（基本一個想定でいいから0）
		_pBlob->GetBufferPointer(),						// シリアライズしたデータのポインタ
		_pBlob->GetBufferSize(),						// シリアライズしたデータのサイズ
		IID_PPV_ARGS(m_pRootSignatrue.GetAddressOf())	// ルートシグネチャ格納先ポインタ
	);
	if (FAILED(_hr))
	{
		printf("ルートシグネチャの生成に失敗\n");
		return;
	}

	m_isValid = true;
}

bool RootSignature::IsValid()
{
	return m_isValid;
}

ID3D12RootSignature* RootSignature::Get()
{
	return m_pRootSignatrue.Get();
}
