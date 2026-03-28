/*
* ルートシグネチャ
* 
* シェーダー内で使用する定数バッファやテクスチャ、サンプラーなどのリソースのレイアウト（メモリ上にどう並んでいるか）
* を定義するオブジェクト
* ルートシグネチャを使って、シェーダー内のどのレジスターとGPUのどのメモリの内容を紐づけするかを決定する
*/
#pragma once

struct ID3D12RootSignature;

class RootSignature
{
public:

	// コンストラクタでルートシグネチャを生成
	RootSignature();

	// 作成
	bool Create(
		const std::vector<std::pair<RootParameterType, std::vector<RangeType>>>& a_rootParamsVec,
		bool a_isUseStaticSampler = true,
		const D3D12_ROOT_SIGNATURE_FLAGS* a_pFlags = nullptr
	);
	bool Create(
		const std::vector<std::pair<RootParameterType, std::vector<RangeType>>>& a_rootParamsVec,
		D3D12_ROOT_SIGNATURE_FLAGS a_flags,
		bool a_isUseStaticSampler = true
	);
	bool Create(
		RootSigInit a_init
	);

	bool IsValid();					// ルートシグネチャの生成に成功しているか
	ID3D12RootSignature* Get();		// ルートシグネチャを返す

private:

	bool m_isValid = false;				// ルートシグネチャの生成に成功したか
	ComPtr<ID3D12RootSignature> m_pRootSignatrue = nullptr;		// ルートシグネチャ

	std::vector<D3D12_ROOT_PARAMETER> m_rootParams;

	std::vector<std::pair<D3D12_ROOT_PARAMETER, std::vector<D3D12_DESCRIPTOR_RANGE>>> m_rootParameters;

};