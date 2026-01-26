#include "StandardPassBase.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/ShaderManager/ShaderManager.h"

void StandardPassBase::Init(
	ShaderManager* a_pShaderMana,
	RootSignatureManager* a_pRootSigMana,
	GraphicsPSOManager* a_pPSOMana
)
{
	m_pGraphicPSOManager = a_pPSOMana;
	m_pRootSignatureManager = a_pRootSigMana;
	m_pShaderManager = a_pShaderMana;

	CreatePass();
}

void StandardPassBase::Begin()
{
	auto* _pCmdList = RenderingEngine::Instance().GetCommandList();

	_pCmdList->SetGraphicsRootSignature(m_pRootSignatureManager->NGet(m_rootSigID));
	_pCmdList->SetPipelineState(m_pGraphicPSOManager->NGet(m_psoID));
	_pCmdList->IASetPrimitiveTopology(m_primitive);
}

void StandardPassBase::End()
{
}
