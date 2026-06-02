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

	protected:

		// 名前設定
		void SetName(const std::string& a_name);

		// セットシェーダー
		void SetShader(const std::string& a_filePath);

	protected:

		// パスのコンピュートシェーダーPSO
		D3D12::ComputePipelineDesc m_csPSODesc = {};			// 作成データ
		Resource::Handle<ID3D12Resource> m_csPSOHandle = {};	// ハンドル

	};
}