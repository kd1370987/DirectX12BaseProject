#include "GraphicsPSOManager.h"

#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

void GraphicsPSOManager::Init(
	const UINT& a_slotSize
)
{
	m_psoSlot.Init(a_slotSize);
}

Resource::ID GraphicsPSOManager::Register(
	const std::string& a_key,
	const D3D12_GRAPHICS_PIPELINE_STATE_DESC& a_psoDesc
)
{
	// 作成
	std::shared_ptr<PipelineState> _spPSO = std::make_shared<PipelineState>();
	_spPSO->Create(D3D12Wrapper::Instance().GetDevice(), a_psoDesc);

	// 登録
	return m_psoSlot.Add(a_key, _spPSO);
}

ID3D12PipelineState* GraphicsPSOManager::NGet(const Resource::ID& a_id)
{
	return m_psoSlot.Ref(a_id)->Get();
}

