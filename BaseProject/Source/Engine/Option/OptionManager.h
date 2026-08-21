#pragma once

#include "IOption.h"

// プロジェクトオプション
#include "ProjectOptions/BuildConfig.h"
#include "ProjectOptions/InputOption.h"

// グラフィックスオプション
#include "GraphicsOptions/GIOptions.h"
#include "GraphicsOptions/WindowOption.h"
#include "GraphicsOptions/RenderingOption.h"
#include "GraphicsOptions/LightingOption.h"
#include "GraphicsOptions/BloomOption.h"
#include "GraphicsOptions/ToneMapOption.h"
#include "GraphicsOptions/SkyOption.h"

// デバッグオプション
#include "DebugOptions/DebugDrawOption.h"

namespace Engine::Option
{
	class OptionManager
	{
	public:

		// 初期化
		void Init();

		// シリアライズ・デシリアライズ
		void Serialize();
		void Deserialize();

		// エディター描画
		void DrawEdit();
		//-------------------------------------------------------------------------------------------------
		// アクセサ
		//-------------------------------------------------------------------------------------------------
		// ---- プロジェクト ----
		// 起動設定
		const ProjectOptions::BuildConfig& GetBuildConfig() const{ return m_buildConfig; }

		// 入力設定
		const ProjectOptions::InputOption& GetInputOption() const { return m_inputOption; }
		ProjectOptions::InputOption& RefInputOption() { return m_inputOption; }

		// ---- グラフィックス ---- 
		// GI設定
		const GraphicsOptions::GIOption& GetGIOption() const { return m_giOptions; }
		GraphicsOptions::GIOption& RefGIOption() { return m_giOptions; }

		// ウィンドウ設定
		const GraphicsOptions::WindowOption& GetWindowOption() const { return m_windowOption; }
		GraphicsOptions::WindowOption& RefWindowOption() { return m_windowOption; }

		// レンダリング設定
		const GraphicsOptions::RenderingOption& GetRenderingOption() const { return m_renderingOption; }
		GraphicsOptions::RenderingOption& RefRenderingOption() { return m_renderingOption; }

		// ライティング設定(シェーダーへ送る調整値)
		const GraphicsOptions::LightingOption& GetLightingOption() const { return m_lightingOption; }
		GraphicsOptions::LightingOption& RefLightingOption() { return m_lightingOption; }

		// ブルーム設定(シェーダーへ送る調整値)
		const GraphicsOptions::BloomOption& GetBloomOption() const { return m_bloomOption; }
		GraphicsOptions::BloomOption& RefBloomOption() { return m_bloomOption; }

		// トーンマップ設定(シェーダーへ送る調整値)
		const GraphicsOptions::ToneMapOption& GetToneMapOption() const { return m_toneMapOption; }
		GraphicsOptions::ToneMapOption& RefToneMapOption() { return m_toneMapOption; }

		// スカイ設定(シェーダーへ送る調整値)
		const GraphicsOptions::SkyOption& GetSkyOption() const { return m_skyOption; }
		GraphicsOptions::SkyOption& RefSkyOption() { return m_skyOption; }

		// ※ 被写界深度(DoF)はカメラの持ち物なので、
		//    アクティブカメラの FocusParamComponent が持つ(旧 DoFOption)

		// ---- デバッグ ----
		// デバッグ描画設定(ワイヤー表示のオンオフ)
		const DebugOptions::DebugDrawOption& GetDebugDrawOption() const { return m_debugDrawOption; }
		DebugOptions::DebugDrawOption& RefDebugDrawOption() { return m_debugDrawOption; }

	private:

		void Archive(Persistence::Archive& a_ar);

	private:

		// プロジェクトオプション
		ProjectOptions::BuildConfig m_buildConfig = {};
		ProjectOptions::InputOption m_inputOption = {};

		// グラフィックスオプション
		GraphicsOptions::GIOption m_giOptions = {};
		GraphicsOptions::WindowOption m_windowOption = {};
		GraphicsOptions::RenderingOption m_renderingOption = {};
		GraphicsOptions::LightingOption m_lightingOption = {};
		GraphicsOptions::BloomOption m_bloomOption = {};
		GraphicsOptions::ToneMapOption m_toneMapOption = {};
		GraphicsOptions::SkyOption m_skyOption = {};

		// デバッグオプション
		DebugOptions::DebugDrawOption m_debugDrawOption = {};

		// ループ処理用
		std::vector<IOption*> m_pOptionList;

	// シングルトン
	private:
		OptionManager();
		~OptionManager() = default;
	public:

		static OptionManager& GetInstance()
		{
			static OptionManager _instance;
			return _instance;
		}
	};
}