#pragma once

class PipelineState
{
public:

	// コンストラクタである程度の設定をする
	PipelineState();
	bool IsValid();		// 生成に成功したかどうか

	void SetInputLayout(D3D12_INPUT_LAYOUT_DESC a_layout);			// 入力レイアウトを設定
	void SetRootSignature(ID3D12RootSignature* a_pRootSignature);	// ルートシグネチャを設定
	void SetVS(std::wstring a_filePath);							// 頂点シェーダーを設定
	void SetVS(D3D12_SHADER_BYTECODE a_byteCode);							// 頂点シェーダーを設定
	void SetPS(std::wstring a_filePath);							// ピクセルシェーダーを設定
	void SetPS(D3D12_SHADER_BYTECODE a_byteCode);							// ピクセルシェーダーを設定
	void Create();													// パイプラインステートを設定

	ID3D12PipelineState* Get();

private:
	bool m_isValid = false;									// 生成に成功したかどうか
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc = {};			// パイプラインステートの仕様書
	ComPtr<ID3D12PipelineState> m_pPipelineState = nullptr;	// パイプラインステート
	ComPtr<ID3DBlob> m_pVSBlob;								// 頂点シェーダー
	ComPtr<ID3DBlob> m_pPSBlob;								// ピクセルシェーダー
};