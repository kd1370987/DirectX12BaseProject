#include "PSOManager.h"

#include "Engine/GPUResource/RootSignature/RootSignature.h"
#include "Engine/GPUResource/PipeLineState/PipelineState.h"

void PSOManager::Init()
{
	// ルートシグネチャ生成
	m_spRootSignature = std::make_shared<RootSignature>();


	// パイプラインステート生成
	m_spPipelineState = std::make_shared<PipelineState>();

	// 頂点シェーダーに渡す情報を設定
	const int _inputElementCount = 5;										// 入力情報数を指定
	const D3D12_INPUT_ELEMENT_DESC _inputElements[_inputElementCount] =		// 入力セマンティクス設定
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のPOSITION
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float2のTEXCOORD
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // UNORM4のCOLOR
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のNORMAL 
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のTANGENT
	};
	const D3D12_INPUT_LAYOUT_DESC _inputLayout =
	{
		_inputElements,
		_inputElementCount
	};
	m_spPipelineState->SetInputLayout(_inputLayout);
	m_spPipelineState->SetRootSignature(m_spRootSignature->Get());
	// シェーダー指定
#ifdef _DEBUG
	m_spPipelineState->SetVS(L"x64/Debug/SimpleVS.cso");
	m_spPipelineState->SetPS(L"x64/Debug/SimplePS.cso");
#else
	m_spPipeLineState->SetVS(L"x64/Release/SimpleVS.cso");
	m_spPipeLineState->SetPS(L"x64/Release/SimplePS.cso");
#endif
	m_spPipelineState->Create();
	if (!m_spPipelineState->IsValid())
	{
		printf("パイプラインステートの生成に失敗\n");
		return;
	}



}
