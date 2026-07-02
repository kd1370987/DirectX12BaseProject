#pragma once
namespace Engine::D3D12
{
	struct MeshPipelineBuilder
	{
		MeshPipelineBuilder();
		~MeshPipelineBuilder();

		// 識別用
		void SetName(const std::string& a_name);

		// ルートシグネチャ
		void SetRootSignature(ID3D12RootSignature* a_pSig);

		// シェーダー
		void SetMS(ID3DBlob* a_pBlob);
		void SetPS(ID3DBlob* a_pBlob);

		// レンダーターゲット
		void AddRenderTarget(DXGI_FORMAT a_format);			// レンダーターゲットの追加

		// ブレンドステート設定 : 基本的に直前によばれた AddRenderTarget で生成したレンダーターゲットに対して付与する
		void BlendEnable(bool a_isEnable);										// ブレンドの有効・無効をセット
		void SrcBlend(D3D12_BLEND a_blend);										// ソースのブレンド係数をセット
		void DestBlend(D3D12_BLEND a_blend);									// デストのブレンド係数をセット
		void BlendOp(D3D12_BLEND_OP a_op);										// ブレンドの演算方法をセット
		void SrcBlendAlpha(D3D12_BLEND a_blend);								// ソースのアルファブレンド係数をセット
		void DestBlendAlpha(D3D12_BLEND a_blend);								// デストのアルファブレンド係数をセット
		void BlendOpAlpha(D3D12_BLEND_OP a_op);									// アルファブレンドの演算方法をセット
		void SetBlendState(const D3D12_BLEND_DESC& a_desc);						// 構造体を丸々セット

		// ラスタライザーステートセット
		void FillMode(D3D12_FILL_MODE a_mode);									// 塗りつぶしモードをセット
		void CullMode(D3D12_CULL_MODE a_mode);									// カリングモードをセット
		void SetRasterizerState(const D3D12_RASTERIZER_DESC& a_desc);			// 構造体を丸々セット

		// 深度ステンシルバッファセット
		void SetDepthStencilFormat(DXGI_FORMAT a_format);

		// 深度ステンシルステートセット
		void DepthEnable(bool a_isEnable);										// 深度テストの有効・無効をセット
		void StencilEnable(bool a_isEnable);									// 深度ステンシルの有効・無効をセット
		void DepthWriteMask(bool a_isMask);										// 深度書き込みの有効・無効をセット
		void DepthFunc(D3D12_COMPARISON_FUNC a_func);							// 深度値計算時の設定,正確に計算、大まかかなど
		void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& a_desc);		// 構造体を丸々セット

		// ビルド直前に呼ばれる関数
		void Commit();

		D3DX12_MESH_SHADER_PIPELINE_STATE_DESC desc = {};
		std::string name = "DefaultMeshPSO";
	private:

		// ステートがまだセットされていないときに、デフォルトのステートをセットする
		void BlendStateDefault();
		void RasterizerStateDefault();
		void DepthStencilStateDefault();

		UINT m_renderTargetCount = 0;			// レンダーターゲットの個数
		bool m_isBlendStateSet = false;			// ブレンドステートがセットされているか
		bool m_isRasterizerStateSet = false;	// ラスタライザーステートがセットされているか
		bool m_isDepthStencilStateSet = false;	// 深度ステンシルステートがセットされているか
	};
}