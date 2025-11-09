#include "PipelineState.h"

#include "../../../Engine/Core/RenderingEngine.h"

PipelineState::PipelineState()
{
	// パイプラインステート
	// 描画に使うGPUの設定を一つのオブジェクトにまとめておく

	// パイプラインステートの設定
	m_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	m_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;					// カリングなし
	m_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);					// ブレンドステートもデフォルト
	m_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);	// 深度ステンシルはデフォルトを使用
	m_desc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	m_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// 三角形を描画
	m_desc.NumRenderTargets = 1;											// 描画対象数
	m_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;					// カラーフォーマット
	m_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 深度フォーマット（Zバッファの精度）
	m_desc.SampleDesc.Count = 1;											// サンプラーは１
	m_desc.SampleDesc.Quality = 0;
}

bool PipelineState::IsValid()
{
	return m_isValid;
}

void PipelineState::SetInputLayout(D3D12_INPUT_LAYOUT_DESC a_layout)
{
	m_desc.InputLayout = a_layout;
}

void PipelineState::SetRootSignature(ID3D12RootSignature* a_pRootSignature)
{
	m_desc.pRootSignature = a_pRootSignature;
}

void PipelineState::SetVS(std::wstring a_filePath)
{
	// 頂点シェーダー読み込み（バイナリにシリアライズしてる）
	auto _hr = D3DReadFileToBlob(a_filePath.c_str(),m_pVSBlob.GetAddressOf());
	if (FAILED(_hr))
	{
		printf("頂点シェーダの読み込みに失敗");
		return;
	}

	m_desc.VS = CD3DX12_SHADER_BYTECODE(m_pVSBlob.Get());
}

void PipelineState::SetPS(std::wstring a_filePath)
{
	// ピクセルシェーダー読み込み
	auto _hr = D3DReadFileToBlob(a_filePath.c_str(),m_pPSBlob.GetAddressOf());
	if (FAILED(_hr))
	{
		printf("ピクセルシェーダーの読込に失敗");
		return;
	}

	m_desc.PS = CD3DX12_SHADER_BYTECODE(m_pPSBlob.Get());
}

void PipelineState::Create()
{
	// デバッグ情報
	wchar_t buf[1024];
	swprintf_s(buf, L"DEBUG PSO CHECK:\nRootSignature: %p\nVSBytecode: %p Len:%llu\nPSBytecode: %p Len:%llu\nNumRenderTargets: %u\nRTV0: %d\nDSV: %d\nInputLayout.NumElements: %u\nPrimitiveTopologyType: %d\nSampleCount: %u\n",
		m_desc.pRootSignature,
		m_desc.VS.pShaderBytecode, m_desc.VS.BytecodeLength,
		m_desc.PS.pShaderBytecode, m_desc.PS.BytecodeLength,
		m_desc.NumRenderTargets,
		(int)m_desc.RTVFormats[0],
		(int)m_desc.DSVFormat,
		m_desc.InputLayout.NumElements,
		(int)m_desc.PrimitiveTopologyType,
		m_desc.SampleDesc.Count
	);
	OutputDebugStringW(buf);


	// パイプラインステート
	auto _hr = RenderingEngine::Instance().Device()->CreateGraphicsPipelineState(
		&m_desc, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		_com_error err(_hr);
		OutputDebugStringW(L"パイプラインステートの生成に失敗: ");
		OutputDebugStringW(err.ErrorMessage());
		OutputDebugStringW(L"\n");
		// 各項目を簡易チェック
		if (!m_desc.pRootSignature) OutputDebugStringW(L"RootSignature が NULL\n");
		if (!m_desc.VS.pShaderBytecode) OutputDebugStringW(L"VS が設定されていない\n");
		if (!m_desc.PS.pShaderBytecode) OutputDebugStringW(L"PS が設定されていない\n");
		if (m_desc.NumRenderTargets == 0) OutputDebugStringW(L"RenderTarget が 0\n");

#if defined(_DEBUG)
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(RenderingEngine::Instance().Device()->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		}
#endif


		return;
	}

	m_isValid = true;
}

ID3D12PipelineState* PipelineState::Get()
{
	return m_pPipelineState.Get();
}
