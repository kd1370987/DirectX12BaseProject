#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

#include "Engine/Graphics/Raytracing/RayPSO/RayPSO.h"
#include "Engine/Graphics/Raytracing/ShaderTable/ShaderTable.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// RaytracingShadowPass
	//
	// カメラから見えるピクセルごとに主光源へレイを1本飛ばし、遮られているかを書く。
	//
	// ボリュメトリックフォグ用の影(VolumeShadow 出力。任意)も、同じレイトレで作る。
	// 画素ごとにカメラから見えている面までの視線を歩き、各点から主光源へレイを飛ばして
	//   r = 日なたの割合(歩いた点の平均) / g = 歩いた範囲の終わり(カメラからの距離)
	// を書く。歩くのはシーンの影の距離(ShadowDistance)まで。
	// フォグ(SceneVolumetricFogPass)は、その範囲の平行光にこの割合を掛ける。
	//
	// 影の求め方がシャドウマップのフレーム(LightManager::GetShadowMode)はレイを飛ばさず、
	// 出力を「影なし」で埋めるだけにする。影は後ろの ShadowMapMaskPass が描き足す
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


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// PSO が組めているか : 組めていないとこのパスは何もしない
		bool IsReady() const { return m_isReady; }

		// フォグ用の影で視線を歩く歩数(1歩ごとにレイを1本)。0 なら作らない
		uint32_t& RefVolumeShadowSteps() { return m_volumeShadowSteps; }

	private:

		// シェーダーへ渡すGBufferのバインドレス添字
		struct GBufferIndex
		{
			int depth;
			int normal;
			Math::Vector2 pad2;
		};

		// フォグ用の影の設定
		// ※ HLSL 側(RayShadow.hlsli の VolumeShadowParam)と並びを合わせること
		struct VolumeShadowParam
		{
			uint32_t outIndex;		// 出力の UAV の番号。繋がっていなければ VOLUME_SHADOW_NONE(歩かない)
			float distance;			// 歩く範囲(ビュー空間の奥行き。シーンの影の距離)
			uint32_t frame;			// 歩く位置のずらしをフレームごとに変える
			uint32_t stepCount;		// 視線を歩く歩数
		};

		// 出力が無いときに渡す番号(シェーダーの VOLUME_SHADOW_NONE と合わせる)
		static constexpr uint32_t VOLUME_SHADOW_NONE = 0xFFFFFFFF;

		// フォグ用の影の出力を、影なし(日なた 1 / 範囲 0)で埋める
		void ClearVolumeShadow(const PassContext& a_context);

		Raytracing::RayPSO m_rayPSO = {};
		Raytracing::ShaderTable m_shaderTable = {};

		// Compile が通っているか
		bool m_isReady = false;

		// 回したフレームの数(フォグ用の影の、歩く位置のずらしに使う)
		uint32_t m_frameCount = 0;

		// フォグ用の影で視線を歩く歩数。0 なら作らない(フォグを置かないパイプライン用)
		uint32_t m_volumeShadowSteps = 32;
	};
}
