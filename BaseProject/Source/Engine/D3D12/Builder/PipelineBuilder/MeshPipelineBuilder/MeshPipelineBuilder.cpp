#include "MeshPipelineBuilder.h"
namespace Engine::D3D12
{
	MeshPipelineBuilder::MeshPipelineBuilder()
	{
		// クリア
		ZeroMemory(&desc, sizeof(desc));
		desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		desc.SampleMask = UINT_MAX;												// 全てのサンプルを有効にする
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// プリミティブトポロジ設定

		// スタティックサンプラー
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		desc.NumRenderTargets = 0;

		desc.MS = { nullptr, 0 };
		desc.PS = { nullptr, 0 };

		m_renderTargetCount = 0;
	}

	MeshPipelineBuilder::~MeshPipelineBuilder()
	{}

	void MeshPipelineBuilder::SetName(const std::string& a_name)
	{
		name = a_name;
	}

	void MeshPipelineBuilder::SetRootSignature(ID3D12RootSignature* a_pSig)
	{
		desc.pRootSignature = a_pSig;
	}

	void MeshPipelineBuilder::SetMS(ID3DBlob* a_pBlob)
	{
		if (a_pBlob)
		{
			desc.MS.pShaderBytecode = a_pBlob->GetBufferPointer();
			desc.MS.BytecodeLength = a_pBlob->GetBufferSize();
		}
	}

	void MeshPipelineBuilder::SetPS(ID3DBlob* a_pBlob)
	{
		if (a_pBlob)
		{
			desc.PS.pShaderBytecode = a_pBlob->GetBufferPointer();
			desc.PS.BytecodeLength = a_pBlob->GetBufferSize();
		}
	}

	void MeshPipelineBuilder::AddRenderTarget(DXGI_FORMAT a_format)
	{
		// 最大数を超えないように安全対策
		if (m_renderTargetCount < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
		{
			desc.RTVFormats[m_renderTargetCount] = a_format;
			m_renderTargetCount++;
			desc.NumRenderTargets = m_renderTargetCount;
		}
	}

	void MeshPipelineBuilder::BlendEnable(bool a_isEnable)
	{
		if (m_renderTargetCount == 0) return;
		auto _idx = m_renderTargetCount - 1;
		desc.BlendState.RenderTarget[_idx].BlendEnable = a_isEnable;
		m_isBlendStateSet = true;
	}

	void MeshPipelineBuilder::SrcBlend(D3D12_BLEND a_blend)
	{
		if (m_renderTargetCount == 0) return;
		auto _idx = m_renderTargetCount - 1;
		desc.BlendState.RenderTarget[_idx].SrcBlend = a_blend;
		m_isBlendStateSet = true;
	}

	void MeshPipelineBuilder::DestBlend(D3D12_BLEND a_blend)
	{
		if (m_renderTargetCount == 0) return;
		auto _idx = m_renderTargetCount - 1;
		desc.BlendState.RenderTarget[_idx].DestBlend = a_blend;
		m_isBlendStateSet = true;
	}

	void MeshPipelineBuilder::BlendOp(D3D12_BLEND_OP a_op)
	{
		if (m_renderTargetCount == 0) return;
		auto _idx = m_renderTargetCount - 1;
		desc.BlendState.RenderTarget[_idx].BlendOp = a_op;
		m_isBlendStateSet = true;
	}

	void MeshPipelineBuilder::SrcBlendAlpha(D3D12_BLEND a_blend)
	{
		if (m_renderTargetCount == 0) return;
		auto _idx = m_renderTargetCount - 1;
		desc.BlendState.RenderTarget[_idx].SrcBlendAlpha = a_blend;
		m_isBlendStateSet = true;
	}

	void MeshPipelineBuilder::DestBlendAlpha(D3D12_BLEND a_blend)
	{
		if (m_renderTargetCount == 0) return;
		auto _idx = m_renderTargetCount - 1;
		desc.BlendState.RenderTarget[_idx].DestBlendAlpha = a_blend;
		m_isBlendStateSet = true;
	}

	void MeshPipelineBuilder::BlendOpAlpha(D3D12_BLEND_OP a_op)
	{
		if (m_renderTargetCount == 0) return;
		auto _idx = m_renderTargetCount - 1;
		desc.BlendState.RenderTarget[_idx].BlendOpAlpha = a_op;
		m_isBlendStateSet = true;
	}

	void MeshPipelineBuilder::SetBlendState(const D3D12_BLEND_DESC& a_desc)
	{
		desc.BlendState = a_desc;
		m_isBlendStateSet = true;
	}

	void MeshPipelineBuilder::FillMode(D3D12_FILL_MODE a_mode)
	{
		desc.RasterizerState.FillMode = a_mode;
		m_isRasterizerStateSet = true;
	}

	void MeshPipelineBuilder::CullMode(D3D12_CULL_MODE a_mode)
	{
		desc.RasterizerState.CullMode = a_mode;
		m_isRasterizerStateSet = true;
	}

	void MeshPipelineBuilder::SetRasterizerState(const D3D12_RASTERIZER_DESC& a_desc)
	{
		desc.RasterizerState = a_desc;
		m_isRasterizerStateSet = true;
	}

	void MeshPipelineBuilder::SetDepthStencilFormat(DXGI_FORMAT a_format)
	{
		desc.DSVFormat = a_format;
	}

	void MeshPipelineBuilder::DepthEnable(bool a_isEnable)
	{
		desc.DepthStencilState.DepthEnable = a_isEnable;
		m_isDepthStencilStateSet = true;
	}

	void MeshPipelineBuilder::StencilEnable(bool a_isEnable)
	{
		desc.DepthStencilState.StencilEnable = a_isEnable;
		m_isDepthStencilStateSet = true;
	}

	void MeshPipelineBuilder::DepthWriteMask(bool a_isMask)
	{
		desc.DepthStencilState.DepthWriteMask = a_isMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		m_isDepthStencilStateSet = true;
	}

	void MeshPipelineBuilder::DepthFunc(D3D12_COMPARISON_FUNC a_func)
	{
		desc.DepthStencilState.DepthFunc = a_func;
		m_isDepthStencilStateSet = true;
	}

	void MeshPipelineBuilder::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& a_desc)
	{
		desc.DepthStencilState = a_desc;
		m_isDepthStencilStateSet = true;
	}

	void MeshPipelineBuilder::Commit()
	{
		BlendStateDefault();
		RasterizerStateDefault();
		DepthStencilStateDefault();
	}

	void MeshPipelineBuilder::BlendStateDefault()
	{
		// まだブレンドステートがセットされていないなら、デフォルトのブレンドステートをセットする
		if (!m_isBlendStateSet)
		{
			auto _desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			desc.BlendState = _desc;
			m_isBlendStateSet = true;
		}
	}

	void MeshPipelineBuilder::RasterizerStateDefault()
	{
		// まだラスタライザーステートがセットされていないなら、デフォルトのラスタライザーステートをセットする
		if (!m_isRasterizerStateSet)
		{
			auto _desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			desc.RasterizerState = _desc;
			m_isRasterizerStateSet = true;
		}
	}

	void MeshPipelineBuilder::DepthStencilStateDefault()
	{
		// まだ深度ステンシルステートがセットされていないなら、デフォルトの深度ステンシルステートをセットする
		if (!m_isDepthStencilStateSet)
		{
			auto _desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			desc.DepthStencilState = _desc;
			m_isDepthStencilStateSet = true;
		}
	}
}