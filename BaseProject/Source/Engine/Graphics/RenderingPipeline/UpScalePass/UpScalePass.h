#pragma once

#include "../RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// UpScalePass
	//
	// ハーフ解像度で作ったGIを、深度と法線を手がかりにフル解像度へ引き伸ばす。
	// 単純な拡大だとエッジがにじむので、エッジを見ながら重みを決める
	//======================================================================================
	class UpScalePass : public Pass
	{
	public:
		~UpScalePass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// シェーダーへ送る調整値
		struct UpScaleCB
		{
			float phiDepth;		// 深度の感度
			float phiNormal;	// 法線の感度
			float pad0;
			float pad1;
		};
		UpScaleCB m_cb = { 1.0f, 32.0f, 0.0f, 0.0f };
	};
}
