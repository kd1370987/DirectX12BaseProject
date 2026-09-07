#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// CopyPass
	//
	// 入力をそのまま別のリソースへ写すだけのパス。シェーダーもPSOも持たない。
	//
	// 旧版の GBufferHistoryPass / PostHistoryPass の中身はリソースコピーだけだったので、
	// 汎用のこれ1つで置き換える。
	//   GBufferHistoryPass : Depth -> PrevDepth と Normal -> PrevNormal の2本 = ノード2つ
	//   PostHistoryPass    : AfterTAAColor -> HistoryTAAColor = ノード1つ
	//
	// 出力の名前とフォーマットはノードごとに変えられる。
	// コピーなので、入力と出力のフォーマットと大きさは一致している必要がある
	//======================================================================================
	class CopyPass : public Pass
	{
	public:
		~CopyPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// コードから組むとき用 : 出力の名前とフォーマットを決める。
		// エディターで打ち替えるのと同じことをする
		void Configure(const std::string& a_resourceName, int a_formatIndex, bool a_isTemporal = false)
		{
			m_params.resourceName = a_resourceName;
			m_params.formatIndex = a_formatIndex;
			m_params.isTemporal = a_isTemporal;
			ApplyOutput();
		}

		//----------------------------------------------------------------------------------
		// 編集対象の値 : エディターはここだけを触る
		//----------------------------------------------------------------------------------
		struct Params
		{
			// 写し先のリソース名 : ノードごとに変える
			std::string resourceName = "CopyResult";

			// 履歴として使うなら立てる。
			// 2枚持ちになり、「前フレームを読む」と宣言したピンからは1つ前の中身が読める。
			// GBufferの前フレーム(PrevDepth / PrevNormal)を作るのがこれ
			bool isTemporal = false;

			// 写し先のフォーマット。入力と揃っていないとコピーできない
			int formatIndex = 0;
		};
		Params& RefParams() { return m_params; }

		// スロットへ出力の設定を反映する
		void ApplyOutput();

		// 選べるフォーマット : よく使うものだけ並べてある
		static const char* ToFormatName(int a_index);
		static constexpr int kFormatCount = 5;

	private:

		Params m_params = {};

		static DXGI_FORMAT ToFormat(int a_index);
	};
}
