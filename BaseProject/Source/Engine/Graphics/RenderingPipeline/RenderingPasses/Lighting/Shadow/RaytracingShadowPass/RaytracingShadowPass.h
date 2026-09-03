#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

#include "Engine/Raytracing/RayPSO/RayPSO.h"
#include "Engine/Raytracing/ShaderTable/ShaderTable.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// RaytracingShadowPass
	//
	// カメラから見えるピクセルごとに主光源へレイを1本飛ばし、遮られているかを書く。
	//
	// レイトレはPSOとルートシグネチャを自前で管理するので、
	// グラフの自動バインド(ヒープ/ルートシグネチャ/PSO/ディスクリプタテーブル)は使わない。
	// スロットは依存関係とバリアのためだけに宣言し、
	// バインドはバインドレスの添字で自分で行う
	//======================================================================================
	class RaytracingShadowPass : public Pass
	{
	public:
		~RaytracingShadowPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// シェーダーへ渡すGBufferのバインドレス添字
		struct GBufferIndex
		{
			int depth;
			int normal;
			DirectX::XMFLOAT2 pad2;
		};

		Raytracing::RayPSO m_rayPSO = {};
		Raytracing::ShaderTable m_shaderTable = {};

		// Compile が通っているか
		bool m_isReady = false;
	};
}
