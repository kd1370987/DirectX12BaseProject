#pragma once

#include "Engine/Graphics/RenderingPipeline/RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GBufferPass
	//
	// 不透明モデルをGBufferへ描くパス。既存の AddGBufferPass を新しい形へ移したもの。
	//
	// 旧版は ZPre が書いた深度に対して EQUAL で描いていたが、
	// こちらは自分で深度を書く(LESS_EQUAL)。移植の途中でも1本で成立させるため。
	// ZPre を分ける場合は深度を入力で受けて EQUAL へ戻す。
	//
	// どのモデルが流れてくるかはマテリアルの透明モードで決まる。
	// シェーディングモデルは見ない
	//======================================================================================
	class GBufferPass : public Pass
	{
	public:
		~GBufferPass() override = default;

		// シェーディングモデル表はこの名前で引く(表示名とは別)
		const char* GetShadingPassName() const override { return "GBuffer"; }

		void SetupSlots() override;
		void OnLinksResolved() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// ルートシグネチャ : PSOマネージャーから引いたハンドルを持つ
		Handle<ID3D12RootSignature> m_rootSigHandle = {};
	};
}
