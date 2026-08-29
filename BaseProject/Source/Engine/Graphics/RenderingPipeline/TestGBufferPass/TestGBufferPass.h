#pragma once

#include "../RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	// ノードエディタの動作確認用のGBufferパス
	// 入出力スロットの宣言と、ノード上/詳細側のUIの置き方のサンプルを兼ねる
	class TestGBufferPass : public Pass
	{
	public:
		~TestGBufferPass() override = default;

		// このパスの入出力スロットを宣言する
		void SetupSlots() override;

		// コンパイル : パスの設定されている情報からランタイムデータを構築する
		void Compile(const PassContext& a_context) override;

		// ランタイム中はこの関数のみで処理する
		void Update(const PassContext& a_context) override;

		// エディター用
		bool EditUpdate() override;		// パスの情報を編集する用
		void EditNode() override;		// パスのノード情報を編集する用

		// シリアライズ : 共通部分は Pass::ArchivePass が処理するので、ここは固有データだけ
		void Archive(Engine::Persistence::Archive& a_arch) override;


	private:

		// パス固有の定数バッファ情報
	};
}
