#include "PipelineState.h"

bool PipelineState::Create(ID3D12Device* a_pDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& a_desc)
{
	// パイプラインステート作成
	auto _hr = a_pDevice->CreateGraphicsPipelineState(
		&a_desc,
		IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "パイプラインステートの生成に失敗\n");
		return false;
	}

	return true;
}

ID3D12PipelineState* PipelineState::Get()
{
	return m_pPipelineState.Get();
}
