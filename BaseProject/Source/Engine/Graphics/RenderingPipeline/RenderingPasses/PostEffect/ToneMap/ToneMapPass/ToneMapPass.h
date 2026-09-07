#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// ToneMapPass
	//
	// HDRのまま持ってきた絵を、画面に出せる範囲へ落とし込む。
	// ここから先はLDRなので、ポストプロセスはこれより前に置くこと
	//======================================================================================
	class ToneMapPass : public Pass
	{
	public:
		~ToneMapPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		struct ToneMapCB
		{
			uint32_t type;		// どの曲線で落とすか
			float exposure;		// 曲線を変えずに全体の明るさだけ動かす倍率
			float whitePoint;	// この明るさを白として扱う(一部の曲線のみ使用)
			float pad0;
		};

		// 編集対象の値 : エディターはここだけを触る。
		// 実体は下の m_cb で、シェーダーへはそのまま送られる
		using Params = ToneMapCB;
		Params& RefParams() { return m_cb; }

	private:

		// もとは OptionManager の ToneMapOption を毎フレーム引いていた。
		// パイプラインごとに変えられるよう、このパスの持ち物にしてある。
		// ※ HLSL 側(Asset/Shader/Common/RootParameters/ToneMapOptionData.hlsli)と並びを合わせること
		ToneMapCB m_cb = { 0, 1.0f, 4.0f, 0.0f };
	};
}
