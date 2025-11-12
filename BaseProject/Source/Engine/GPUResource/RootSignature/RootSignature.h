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

	bool IsValid();					// ルートシグネチャの生成に成功しているか
	ID3D12RootSignature* Get();		// ルートシグネチャを返す

private:

	bool m_isValid = false;				// ルートシグネチャの生成に成功したか
	ComPtr<ID3D12RootSignature> m_pRootSignatrue = nullptr;		// ルートシグネチャ

};