#pragma once

class PipelineState
{
public:

	bool Create(ID3D12Device* a_pDevice,const D3D12_GRAPHICS_PIPELINE_STATE_DESC& a_desc);

	ID3D12PipelineState* Get();

private:

	ComPtr<ID3D12PipelineState> m_pPipelineState = nullptr;	// パイプラインステート

};