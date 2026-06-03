#pragma once

namespace Engine::D3D12
{
	// グラフィックスパイプラインステートの設定
	struct GraphicsPipelineDesc
	{
		GraphicsPipelineDesc();

		void SetName(const std::string& a_name);

		// ルートシグネチャセット
		void SetRootSignature(ID3D12RootSignature* a_pRootSig);
		// シェーダー系セット
		void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& a_desc);
		void SetVS(const D3D12_SHADER_BYTECODE& a_bytecode);
		void SetPS(const D3D12_SHADER_BYTECODE& a_bytecode);

		// ブレンドステートセット
		void BlendEnable(bool a_isEnable, UINT a_rtIdx = 0);					// ブレンドの有効・無効をセット
		void SrcBlend(D3D12_BLEND a_blend, UINT a_rtIdx = 0);				// ソースのブレンド係数をセット
		void DestBlend(D3D12_BLEND a_blend, UINT a_rtIdx = 0);				// デストのブレンド係数をセット
		void BlendOp(D3D12_BLEND_OP a_op, UINT a_rtIdx = 0);				// ブレンドの演算方法をセット
		void SrcBlendAlpha(D3D12_BLEND a_blend, UINT a_rtIdx = 0);		// ソースのアルファブレンド係数をセット
		void DestBlendAlpha(D3D12_BLEND a_blend, UINT a_rtIdx = 0);		// デストのアルファブレンド係数をセット
		void BlendOpAlpha(D3D12_BLEND_OP a_op, UINT a_rtIdx = 0);		// アルファブレンドの演算方法をセット
		void SetBlendState(const D3D12_BLEND_DESC& a_desc);
		// ラスタライザーステートセット
		void FillMode(D3D12_FILL_MODE a_mode);			// 塗りつぶしモードをセット
		void CullMode(D3D12_CULL_MODE a_mode);			// カリングモードをセット
		void SetRasterizerState(const D3D12_RASTERIZER_DESC& a_desc);
		// 深度ステンシルステートセット
		void DepthEnable(bool a_isEnable);					// 深度テストの有効・無効をセット
		void StencilEnable(bool a_isEnable);				// 深度テストの有効・無効をセット
		void DepthWriteMask(bool a_isMask);					// 深度書き込みの有効・無効をセット
		void DepthFunc(D3D12_COMPARISON_FUNC a_func);
		void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& a_desc);

		// レンダーターゲットセット
		void AddRenderTargetFormat(DXGI_FORMAT a_format);

		// 本体
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		std::string name = "DefaultGraphicsPSO";
	private:
		// ステートがまだセットされていないときに、デフォルトのステートをセットする
		void BlendStateDefault();
		void RasterizerStateDefault();
		void DepthStencilStateDefault();

		UINT m_renderTargetCount = 0;	// レンダーターゲットの個数
		bool m_isBlendStateSet = false;	// ブレンドステートがセットされているか
		bool m_isRasterizerStateSet = false;	// ラスタライザーステートがセットされているか
		bool m_isDepthStencilStateSet = false;	// 深度ステンシルステートがセットされているか
	};

	// コンピュート用作成構造体
	struct ComputePipelineDesc
	{
		// 名前セット
		void SetName(const std::string& a_name);

		// ルートシグネチャセット
		void SetRootSignature(ID3D12RootSignature* a_pRootSig);

		// シェーダーセット
		void SetCS(const D3D12_SHADER_BYTECODE& a_byteCode, const size_t& a_size);

		// 変数
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		std::string name = "DefaultComputePSO";
	};

	// パイプラインステートオブジェクト
	class PipelineState
	{
	public:

		// コンストラクタ・デストラクタ
		PipelineState();
		~PipelineState();

		// 作成
		// グラフィック
		bool Create(ID3D12Device* a_pDevice, const GraphicsPipelineDesc& a_desc);
		bool Create(ID3D12Device* a_pDevice, const ComputePipelineDesc& a_desc);

		// アクセサ
		const ID3D12PipelineState* Get() const;
		ID3D12PipelineState* Ref();

	private:

		ComPtr<ID3D12PipelineState> m_cpPipelineState = nullptr;	// パイプラインステート

	};
}