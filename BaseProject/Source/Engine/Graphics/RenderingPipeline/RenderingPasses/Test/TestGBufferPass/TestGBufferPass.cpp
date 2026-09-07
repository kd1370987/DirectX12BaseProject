#include "TestGBufferPass.h"

namespace Engine::Graphics::Pipeline
{
	// 入力ピンは「役割」だけ決める。中身のリソースはつないだ相手からもらう。
	// 出力ピンは自分が作るリソースなので、名前とフォーマットまでここで決める
	void TestGBufferPass::SetupSlots()
	{
		// 事前デプス : つながっていなければ自前で作る想定なので任意入力にする。
		// 必須にすると、置いただけのパスが検証で止まってしまう
		DeclareInput("PreDepth", EAccessType::Depth_Write, EPassSlotType::Texture, false);

		DeclareOutput("Albedo", "GBufferAlbedo", DXGI_FORMAT_R8G8B8A8_UNORM);
		DeclareOutput("Normal", "GBufferNormal", DXGI_FORMAT_R10G10B10A2_UNORM);
		DeclareOutput("Depth", "SceneDepth", DXGI_FORMAT_D32_FLOAT, EAccessType::Depth_Write);
	}

	// リソースが揃った後に呼ばれる。
	// ディスクリプタやPSOを引いておくならここ
	void TestGBufferPass::Compile(const PassContext& a_context)
	{
		(void)a_context;
	}

	// バリア・レンダーターゲット切り替え・クリアはグラフ側が済ませてあるので、
	// ここは自分の描画コマンドを積むだけでよい
	void TestGBufferPass::Update(const PassContext& a_context)
	{
		(void)a_context;
	}

	// 選択中に出る詳細側のUI
	// ノードの中に詰めると線が見えなくなるので、細かい設定はこちらへ置く

	// ノードの中に出すUI : ピンと削除ボタンは呼び出し側が描くので、ここは固有分だけ

	// このパスは固有のパラメータをまだ持っていないので何もしない。
	// 定数バッファなどを足したらここへ書く
	void TestGBufferPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
