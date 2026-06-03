#pragma once

#include "../BaseRenderPass.h"

namespace Engine::Graphics
{
	// コンピュートパスベースクラス
	class ComputePass : public BaseRenderPass
	{
	public:

		ComputePass() = default;
		virtual ~ComputePass() override = default;

		void Init(const PassInitDesc& a_initDesc) override;

	protected:

		// 名前設定
		void SetName(const std::string& a_name);

		// セットシェーダー
		void SetShader(const std::string& a_filePath);

		// ルートシグネチャPSOをバインド
		void SetPSO(RenderContext* a_pCtx);

	protected:

		// パスのコンピュートシェーダーPSO
		D3D12::ComputePipelineDesc m_csPSODesc = {};			// 作成データ
		Resource::Handle<ID3D12PipelineState> m_csPSOHandle = {};	// ハンドル

		// ルートシグネチャ
		ID3D12RootSignature* m_pRootSig = nullptr;
	};
}