#include "RenderPipelineBuilder.h"

namespace Engine::D3D12
{
	RenderPipelineBuilder::RenderPipelineBuilder()
	{
		// 構造体の初期化時に各ステートのデフォルト値をセットしておく
		BlendStateDefault();
		RasterizerStateDefault();
		DepthStencilStateDefault();
	}

	RenderPipelineBuilder::~RenderPipelineBuilder()
	{}

	void RenderPipelineBuilder::SetName(const std::string& a_name)
	{
		m_name = a_name;
	}

	void RenderPipelineBuilder::SetRootSignature(ID3D12RootSignature* a_pSig)
	{
		m_pRootSignature = a_pSig;
	}

	// ==========================================================
	// シェーダー設定
	// ==========================================================
	void RenderPipelineBuilder::SetVS(const D3D12_SHADER_BYTECODE& a_bytecode)
	{
		m_vs = a_bytecode;
	}

	void RenderPipelineBuilder::SetPS(const D3D12_SHADER_BYTECODE& a_bytecode)
	{
		m_ps = a_bytecode;
	}

	void RenderPipelineBuilder::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& a_layout)
	{
		m_inputLayout = a_layout;
	}

	void RenderPipelineBuilder::SetMS(const D3D12_SHADER_BYTECODE& a_bytecode)
	{
		m_ms = a_bytecode;
	}

	void RenderPipelineBuilder::SetAS(const D3D12_SHADER_BYTECODE& a_bytecode)
	{
		m_as = a_bytecode;
	}

	// ==========================================================
	// レンダーターゲット / 深度フォーマット
	// ==========================================================
	void RenderPipelineBuilder::AddRenderTargetFormat(DXGI_FORMAT a_format)
	{
		// レンダーターゲットの最大数(8)を超えないかチェック
		ENGINE_ERRLOG(m_numRenderTargets < 8, "レンダーターゲットの最大数を超過しています。");
		if (m_numRenderTargets < 8)
		{
			m_rtvFormats[m_numRenderTargets] = a_format;
			m_numRenderTargets++;
		}
	}

	void RenderPipelineBuilder::SetDepthStencilFormat(DXGI_FORMAT a_format)
	{
		m_dsvFormat = a_format;
	}

	// ==========================================================
	// ブレンドステート設定 : ０番目のRTVのみ設定
	// ==========================================================
	void RenderPipelineBuilder::BlendEnable(bool a_isEnable)
	{
		m_blendDesc.RenderTarget[0].BlendEnable = a_isEnable ? TRUE : FALSE;
		m_isBlendStateSet = true;
	}

	void RenderPipelineBuilder::SrcBlend(D3D12_BLEND a_blend)
	{
		m_blendDesc.RenderTarget[0].SrcBlend = a_blend;
		m_isBlendStateSet = true;
	}

	void RenderPipelineBuilder::DestBlend(D3D12_BLEND a_blend)
	{
		m_blendDesc.RenderTarget[0].DestBlend = a_blend;
		m_isBlendStateSet = true;
	}

	void RenderPipelineBuilder::BlendOp(D3D12_BLEND_OP a_op)
	{
		m_blendDesc.RenderTarget[0].BlendOp = a_op;
		m_isBlendStateSet = true;
	}

	void RenderPipelineBuilder::SrcBlendAlpha(D3D12_BLEND a_blend)
	{
		m_blendDesc.RenderTarget[0].SrcBlendAlpha = a_blend;
		m_isBlendStateSet = true;
	}

	void RenderPipelineBuilder::DestBlendAlpha(D3D12_BLEND a_blend)
	{
		m_blendDesc.RenderTarget[0].DestBlendAlpha = a_blend;
		m_isBlendStateSet = true;
	}

	void RenderPipelineBuilder::BlendOpAlpha(D3D12_BLEND_OP a_op)
	{
		m_blendDesc.RenderTarget[0].BlendOpAlpha = a_op;
		m_isBlendStateSet = true;
	}

	void RenderPipelineBuilder::SetBlendState(const D3D12_BLEND_DESC& a_desc)
	{
		m_blendDesc = a_desc;
		m_isBlendStateSet = true;
	}

	// ==========================================================
	// ラスタライザーステート設定
	// ==========================================================
	void RenderPipelineBuilder::FillMode(D3D12_FILL_MODE a_mode)
	{
		m_rasterizerDesc.FillMode = a_mode;
		m_isRasterizerStateSet = true;
	}

	void RenderPipelineBuilder::CullMode(D3D12_CULL_MODE a_mode)
	{
		m_rasterizerDesc.CullMode = a_mode;
		m_isRasterizerStateSet = true;
	}

	void RenderPipelineBuilder::SetRasterizerState(const D3D12_RASTERIZER_DESC& a_desc)
	{
		m_rasterizerDesc = a_desc;
		m_isRasterizerStateSet = true;
	}

	// ==========================================================
	// 深度ステンシルステート設定
	// ==========================================================
	void RenderPipelineBuilder::DepthEnable(bool a_isEnable)
	{
		m_depthStencilDesc.DepthEnable = a_isEnable ? TRUE : FALSE;
		m_isDepthStencilStateSet = true;
	}

	void RenderPipelineBuilder::StencilEnable(bool a_isEnable)
	{
		m_depthStencilDesc.StencilEnable = a_isEnable ? TRUE : FALSE;
		m_isDepthStencilStateSet = true;
	}

	void RenderPipelineBuilder::DepthWriteMask(bool a_isWriteEnable)
	{
		m_depthStencilDesc.DepthWriteMask = a_isWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		m_isDepthStencilStateSet = true;
	}

	void RenderPipelineBuilder::DepthFunc(D3D12_COMPARISON_FUNC a_func)
	{
		m_depthStencilDesc.DepthFunc = a_func;
		m_isDepthStencilStateSet = true;
	}

	void RenderPipelineBuilder::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& a_desc)
	{
		m_depthStencilDesc = a_desc;
		m_isDepthStencilStateSet = true;
	}

	// ==========================================================
	// トポロジー設定
	// ==========================================================
	void RenderPipelineBuilder::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE a_type)
	{
		m_topologyType = a_type;
	}

	// ==========================================================
	// プライベート関数（デフォルトステートの初期化）
	// ==========================================================
	void RenderPipelineBuilder::BlendStateDefault()
	{
		m_blendDesc.AlphaToCoverageEnable = FALSE;
		m_blendDesc.IndependentBlendEnable = FALSE;
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			m_blendDesc.RenderTarget[i].BlendEnable = FALSE;
			m_blendDesc.RenderTarget[i].LogicOpEnable = FALSE;
			m_blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
			m_blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
			m_blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
			m_blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
			m_blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
			m_blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			m_blendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
			m_blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
	}

	void RenderPipelineBuilder::RasterizerStateDefault()
	{
		m_rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		m_rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		m_rasterizerDesc.FrontCounterClockwise = FALSE;
		m_rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		m_rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		m_rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		m_rasterizerDesc.DepthClipEnable = TRUE;
		m_rasterizerDesc.MultisampleEnable = FALSE;
		m_rasterizerDesc.AntialiasedLineEnable = FALSE;
		m_rasterizerDesc.ForcedSampleCount = 0;
		m_rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	void RenderPipelineBuilder::DepthStencilStateDefault()
	{
		m_depthStencilDesc.DepthEnable = TRUE;
		m_depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		m_depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		m_depthStencilDesc.StencilEnable = FALSE;
		m_depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		m_depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{
			D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP,
			D3D12_COMPARISON_FUNC_ALWAYS
		};
		m_depthStencilDesc.FrontFace = defaultStencilOp;
		m_depthStencilDesc.BackFace = defaultStencilOp;
	}
}
