#pragma once

#include "../RenderingPipeline.h"

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

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// スロットへ出力の設定を反映する
		void ApplyOutput();

		// 写し先のリソース名 : ノードごとに変える
		std::string m_resourceName = "CopyResult";

		// 写し先のフォーマット。入力と揃っていないとコピーできない
		int m_formatIndex = 0;

		// 選べるフォーマット : よく使うものだけ並べてある
		static DXGI_FORMAT ToFormat(int a_index);
		static const char* ToFormatName(int a_index);
		static constexpr int kFormatCount = 5;
	};
}
