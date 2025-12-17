#include "PSOManager.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

#include "Engine/GPUResource/RootSignature/RootSignature.h"
#include "Engine/GPUResource/PipeLineState/PipelineState.h"

void PSOManager::Init()
{
	m_spRootSignature = std::make_shared<RootSignature>();
	if (!m_spRootSignature->Create({ RangeType::CBV,RangeType::CBV,RangeType::SRV }))
	//if (!m_spRootSignature->IsValid())
	{
		assert( 0 && "ルートシグネチャの生成に失敗");
		return;
	}

	// パイプラインステート生成
	std::shared_ptr<PipelineState> _spPipelineState = std::make_shared<PipelineState>();

	// 頂点シェーダーに渡す情報を設定
	const int _inputElementCount = 5;										// 入力情報数を指定
	const D3D12_INPUT_ELEMENT_DESC _inputElements[_inputElementCount] =		// 入力セマンティクス設定
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のPOSITION
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のNORMAL
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float2のTEXCOORD
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のTANGENT
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float4のCOLOR
	};
	const D3D12_INPUT_LAYOUT_DESC _inputLayout =
	{
		_inputElements,
		_inputElementCount
	};
	_spPipelineState->SetInputLayout(_inputLayout);
	_spPipelineState->SetRootSignature(m_spRootSignature->Get());
	// シェーダー指定
#ifdef _DEBUG
	_spPipelineState->SetVS(L"x64/Debug/SimpleVS.cso");
	_spPipelineState->SetPS(L"x64/Debug/SimplePS.cso");
#else
	_spPipeLineState->SetVS(L"x64/Release/SimpleVS.cso");
	_spPipeLineState->SetPS(L"x64/Release/SimplePS.cso");
#endif
	_spPipelineState->Create();
	if (!_spPipelineState->IsValid())
	{
		printf("パイプラインステートの生成に失敗\n");
		return;
	}

	// 追加
	m_pipelineMap["SimplePipeline"] = _spPipelineState;
}

ID3D12PipelineState* PSOManager::GetPipelineState(const std::string& a_name)
{
	auto _it = m_pipelineMap.find(a_name);
	if (_it != m_pipelineMap.end())
	{
		return _it->second->Get();
	}
	return nullptr;
}

void PSOManager::SetPipelienStaet(const std::string& a_name)
{
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	_cmdList->SetGraphicsRootSignature(m_spRootSignature->Get());
	_cmdList->SetPipelineState(GetPipelineState(a_name));
}
