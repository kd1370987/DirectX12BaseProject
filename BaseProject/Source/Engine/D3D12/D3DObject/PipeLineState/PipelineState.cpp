#include "PipelineState.h"
namespace Engine::D3D12
{
	PipelineState::PipelineState()
	{}

	PipelineState::~PipelineState()
	{}

	
	bool PipelineState::Create(ID3D12Device* a_pDevice, const GraphicsPipelineDesc& a_desc)
	{
		auto _hr = a_pDevice->CreateGraphicsPipelineState(
			&a_desc.desc,
			IID_PPV_ARGS(m_cpPipelineState.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			Editor::MainEditor::Instance().ErrorLog("グラフィックス用パイプラインステートの生成に失敗");
			return false;
		}

		m_cpPipelineState->SetName(StringUtility::ToWideString(a_desc.name).c_str());

		return true;
	}

	bool PipelineState::Create(ID3D12Device* a_pDevice, const ComputePipelineDesc& a_desc)
	{
		auto _hr = a_pDevice->CreateComputePipelineState(
			&a_desc.desc,
			IID_PPV_ARGS(m_cpPipelineState.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			Editor::MainEditor::Instance().ErrorLog("コンピュート用シェーダーの作成に失敗");
			return false;
		}

		m_cpPipelineState->SetName(StringUtility::ToWideString(a_desc.name).c_str());

		return true;
	}

	const ID3D12PipelineState* PipelineState::Get() const
	{
		return m_cpPipelineState.Get();
	}

	ID3D12PipelineState* PipelineState::Ref()
	{
		return m_cpPipelineState.Get();
	}

	GraphicsPipelineDesc::GraphicsPipelineDesc()
	{
		desc.RasterizerState	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		desc.BlendState			= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		desc.DepthStencilState	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		desc.SampleMask = UINT_MAX;				// サンプルマスクは全てのサンプルを有効にする
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// 三角形リスト

		// スタティックサンプラー
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		desc.NumRenderTargets = 0;

		desc.PS = { nullptr,0 };
	}

	void GraphicsPipelineDesc::SetName(const std::string& a_name)
	{
		name = a_name;
	}

	void Engine::D3D12::GraphicsPipelineDesc::SetRootSignature(ID3D12RootSignature* a_pRootSig)
	{
		desc.pRootSignature = a_pRootSig;
	}

	void Engine::D3D12::GraphicsPipelineDesc::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& a_desc)
	{
		desc.InputLayout = a_desc;
	}

	void Engine::D3D12::GraphicsPipelineDesc::SetVS(const D3D12_SHADER_BYTECODE& a_bytecode)
	{
		desc.VS = a_bytecode;
	}

	void Engine::D3D12::GraphicsPipelineDesc::SetPS(const D3D12_SHADER_BYTECODE& a_bytecode)
	{
		desc.PS = a_bytecode;
	}

	void Engine::D3D12::GraphicsPipelineDesc::BlendEnable(bool a_isEnable, UINT a_rtIdx)
	{
		// まだブレンドステートがセットされていないなら、デフォルトのブレンドステートをセットする
		if (!m_isBlendStateSet)
		{
			auto _desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			desc.BlendState = _desc;
			m_isBlendStateSet = true;
		}
		desc.BlendState.RenderTarget[a_rtIdx].BlendEnable = a_isEnable;
	}

	void Engine::D3D12::GraphicsPipelineDesc::SrcBlend(D3D12_BLEND a_blend, UINT a_rtIdx)
	{
		BlendStateDefault();
		desc.BlendState.RenderTarget[a_rtIdx].SrcBlend = a_blend;
	}

	void Engine::D3D12::GraphicsPipelineDesc::DestBlend(D3D12_BLEND a_blend, UINT a_rtIdx)
	{
		BlendStateDefault();
		desc.BlendState.RenderTarget[a_rtIdx].DestBlend = a_blend;
	}

	void Engine::D3D12::GraphicsPipelineDesc::BlendOp(D3D12_BLEND_OP a_op, UINT a_rtIdx)
	{
		BlendStateDefault();
		desc.BlendState.RenderTarget[a_rtIdx].BlendOp = a_op;
	}

	void Engine::D3D12::GraphicsPipelineDesc::SrcBlendAlpha(D3D12_BLEND a_blend, UINT a_rtIdx)
	{
		BlendStateDefault();
		desc.BlendState.RenderTarget[a_rtIdx].SrcBlendAlpha = a_blend;
	}

	void Engine::D3D12::GraphicsPipelineDesc::DestBlendAlpha(D3D12_BLEND a_blend, UINT a_rtIdx)
	{
		BlendStateDefault();
		desc.BlendState.RenderTarget[a_rtIdx].DestBlendAlpha = a_blend;
	}

	void Engine::D3D12::GraphicsPipelineDesc::BlendOpAlpha(D3D12_BLEND_OP a_op, UINT a_rtIdx)
	{
		BlendStateDefault();
		desc.BlendState.RenderTarget[a_rtIdx].BlendOpAlpha = a_op;
	}

	void Engine::D3D12::GraphicsPipelineDesc::SetBlendState(const D3D12_BLEND_DESC& a_desc)
	{
		desc.BlendState = a_desc;
		m_isBlendStateSet = true;
	}

	void Engine::D3D12::GraphicsPipelineDesc::FillMode(D3D12_FILL_MODE a_mode)
	{
		RasterizerStateDefault();
		desc.RasterizerState.FillMode = a_mode;
	}

	void Engine::D3D12::GraphicsPipelineDesc::CullMode(D3D12_CULL_MODE a_mode)
	{

		RasterizerStateDefault();
		desc.RasterizerState.CullMode = a_mode;
	}

	void Engine::D3D12::GraphicsPipelineDesc::SetRasterizerState(const D3D12_RASTERIZER_DESC& a_desc)
	{
		desc.RasterizerState = a_desc;
		m_isRasterizerStateSet = true;
	}

	void Engine::D3D12::GraphicsPipelineDesc::DepthEnable(bool a_isEnable)
	{
		DepthStencilStateDefault();
		desc.DepthStencilState.DepthEnable = a_isEnable;
		if (a_isEnable)
		{
			desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		}
		else
		{
			desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		}
	}

	void GraphicsPipelineDesc::StencilEnable(bool a_isEnable)
	{
		DepthStencilStateDefault();
		desc.DepthStencilState.StencilEnable = a_isEnable;
	}


	void Engine::D3D12::GraphicsPipelineDesc::DepthWriteMask(bool a_isMask)
	{
		DepthStencilStateDefault();
		desc.DepthStencilState.DepthWriteMask = a_isMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	}

	void GraphicsPipelineDesc::DepthFunc(D3D12_COMPARISON_FUNC a_func)
	{
		desc.DepthStencilState.DepthFunc = a_func;
	}


	void Engine::D3D12::GraphicsPipelineDesc::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& a_desc)
	{
		desc.DepthStencilState = a_desc;
		m_isDepthStencilStateSet = true;
		if (desc.DepthStencilState.DepthEnable || desc.DepthStencilState.StencilEnable)
		{
			desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		}
		else
		{
			desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		}
	}

	void Engine::D3D12::GraphicsPipelineDesc::AddRenderTargetFormat(DXGI_FORMAT a_format)
	{
		desc.RTVFormats[m_renderTargetCount] = a_format;
		m_renderTargetCount++;
		desc.NumRenderTargets = m_renderTargetCount;
	}

	void Engine::D3D12::GraphicsPipelineDesc::BlendStateDefault()
	{
		// まだブレンドステートがセットされていないなら、デフォルトのブレンドステートをセットする
		if (!m_isBlendStateSet)
		{
			auto _desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			desc.BlendState = _desc;
			m_isBlendStateSet = true;
		}
	}

	void Engine::D3D12::GraphicsPipelineDesc::RasterizerStateDefault()
	{
		// まだラスタライザーステートがセットされていないなら、デフォルトのラスタライザーステートをセットする
		if (!m_isRasterizerStateSet)
		{
			auto _desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			desc.RasterizerState = _desc;
			m_isRasterizerStateSet = true;
		}
	}

	void Engine::D3D12::GraphicsPipelineDesc::DepthStencilStateDefault()
	{
		// まだ深度ステンシルステートがセットされていないなら、デフォルトの深度ステンシルステートをセットする
		if (!m_isDepthStencilStateSet)
		{
			auto _desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			desc.DepthStencilState = _desc;
			m_isDepthStencilStateSet = true;
		}
	}
	void ComputePipelineDesc::SetName(const std::string& a_name)
	{
		name = a_name;
	}
	void ComputePipelineDesc::SetRootSignature(ID3D12RootSignature * a_pRootSig)
	{
		desc.pRootSignature = a_pRootSig;
	}
	void ComputePipelineDesc::SetCS(const D3D12_SHADER_BYTECODE & a_byteCode,const size_t& a_size)
	{
		desc.CS.pShaderBytecode = &a_byteCode;
		desc.CS.BytecodeLength = a_size;
	}
}