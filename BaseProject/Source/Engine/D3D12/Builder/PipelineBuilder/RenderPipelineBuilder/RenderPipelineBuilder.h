#pragma once
namespace Engine::D3D12
{
	/// <summary>
	/// グラフィックス(VS/PS)とメッシュ(MS/AS/PS)の両方に対応する統合パイプラインビルダー
	/// </summary>
	struct RenderPipelineBuilder
	{
		RenderPipelineBuilder();
		~RenderPipelineBuilder();

		// 識別用
		void SetName(const std::string& a_name);

		// ルートシグネチャ
		void SetRootSignature(ID3D12RootSignature* a_pSig);

		// ==========================================================
		// シェーダー設定
		// ==========================================================
		// 従来用 (VS/PS)
		void SetVS(const D3D12_SHADER_BYTECODE& a_bytecode);
		void SetPS(const D3D12_SHADER_BYTECODE& a_bytecode);
		void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& a_layout); // VS用

		// メッシュシェーダー用 (MS/AS)
		void SetMS(const D3D12_SHADER_BYTECODE& a_bytecode);
		void SetAS(const D3D12_SHADER_BYTECODE& a_bytecode);

		// どちらのパイプラインか判定するためのヘルパー
		bool HasMeshShader() const { return m_ms.pShaderBytecode != nullptr; }
		bool HasAmplificationShader() const { return m_as.pShaderBytecode != nullptr; }

		// ==========================================================
		// レンダーターゲット / 深度フォーマット
		// ==========================================================
		void AddRenderTargetFormat(DXGI_FORMAT a_format);
		void SetDepthStencilFormat(DXGI_FORMAT a_format);

		// ==========================================================
		// ブレンドステート設定
		// ==========================================================
		void BlendEnable(bool a_isEnable);
		void SrcBlend(D3D12_BLEND a_blend);
		void DestBlend(D3D12_BLEND a_blend);
		void BlendOp(D3D12_BLEND_OP a_op);
		void SrcBlendAlpha(D3D12_BLEND a_blend);
		void DestBlendAlpha(D3D12_BLEND a_blend);
		void BlendOpAlpha(D3D12_BLEND_OP a_op);
		void SetBlendState(const D3D12_BLEND_DESC& a_desc);

		// ==========================================================
		// ラスタライザーステート設定
		// ==========================================================
		void FillMode(D3D12_FILL_MODE a_mode);
		void CullMode(D3D12_CULL_MODE a_mode);
		void SetRasterizerState(const D3D12_RASTERIZER_DESC& a_desc);

		// ==========================================================
		// 深度ステンシルステート設定
		// ==========================================================
		void DepthEnable(bool a_isEnable);
		void StencilEnable(bool a_isEnable);
		void DepthWriteMask(bool a_isWriteEnable);
		void DepthFunc(D3D12_COMPARISON_FUNC a_func);
		void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& a_desc);

		// ==========================================================
		// トポロジー設定 (基本はTRIANGLE固定が多いですが念のため)
		// ==========================================================
		void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE a_type);

		// ==========================================================
		// ゲッター (ManagerがPSOを構築する際に使用)
		// ==========================================================
		const std::string& GetName() const { return m_name; }
		ID3D12RootSignature* GetRootSignature() const { return m_pRootSignature; }

		const D3D12_SHADER_BYTECODE& GetVS() const { return m_vs; }
		const D3D12_SHADER_BYTECODE& GetPS() const { return m_ps; }
		const D3D12_SHADER_BYTECODE& GetMS() const { return m_ms; }
		const D3D12_SHADER_BYTECODE& GetAS() const { return m_as; }
		const D3D12_INPUT_LAYOUT_DESC& GetInputLayout() const { return m_inputLayout; }

		UINT GetNumRenderTargets() const { return m_numRenderTargets; }
		const DXGI_FORMAT* GetRenderTargetFormats() const { return m_rtvFormats; }
		DXGI_FORMAT GetDepthStencilFormat() const { return m_dsvFormat; }

		const D3D12_BLEND_DESC& GetBlendState() const { return m_blendDesc; }
		const D3D12_RASTERIZER_DESC& GetRasterizerState() const { return m_rasterizerDesc; }
		const D3D12_DEPTH_STENCIL_DESC& GetDepthStencilState() const { return m_depthStencilDesc; }

		D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_topologyType; }
		DXGI_SAMPLE_DESC GetSampleDesc() const { return m_sampleDesc; }

	private:

		// デフォルトステートの初期化
		void BlendStateDefault();
		void RasterizerStateDefault();
		void DepthStencilStateDefault();

	private:

		std::string m_name = "DefaultRenderPSO";

		ID3D12RootSignature* m_pRootSignature = nullptr;

		// シェーダー
		D3D12_SHADER_BYTECODE m_vs = {};
		D3D12_SHADER_BYTECODE m_ps = {};
		D3D12_SHADER_BYTECODE m_ms = {};
		D3D12_SHADER_BYTECODE m_as = {};

		D3D12_INPUT_LAYOUT_DESC m_inputLayout = {};

		// フォーマット
		UINT m_numRenderTargets = 0;
		DXGI_FORMAT m_rtvFormats[8] = {};
		DXGI_FORMAT m_dsvFormat = DXGI_FORMAT_UNKNOWN;

		// ステート
		D3D12_BLEND_DESC m_blendDesc = {};
		D3D12_RASTERIZER_DESC m_rasterizerDesc = {};
		D3D12_DEPTH_STENCIL_DESC m_depthStencilDesc = {};

		D3D12_PRIMITIVE_TOPOLOGY_TYPE m_topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		DXGI_SAMPLE_DESC m_sampleDesc = { 1, 0 }; // デフォルトはMSAAなし

		bool m_isBlendStateSet = false;
		bool m_isRasterizerStateSet = false;
		bool m_isDepthStencilStateSet = false;
	};
}