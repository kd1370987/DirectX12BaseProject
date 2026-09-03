#pragma once
//==========================================================================================
//
// PassContext (Engine::Graphics::Pipeline)
//
// パスへ渡す実行コンテキスト。
// 中身はすべて「借り物のポインタ」なので、実体は前方宣言で足りる
//
//==========================================================================================

// 実行時に渡ってくるもの : このヘッダーでは中身を知らなくてよい
namespace Engine::Graphics
{
	class RenderContext;
	class GraphicsEngine;
}

namespace Engine::Graphics::Pipeline
{
	class RenderGraph;
	struct Slot;

	//======================================================================================
	// パスへ渡す実行コンテキスト
	//
	// パスが RenderGraph の中身を直接知らずに、自分のスロットから
	// GPUリソースへ辿り着けるようにするための入口。
	//
	// Compile 時は pGraph だけが入る(実行系はまだ無い)。
	// Update 時は pRenderContext / pCmdList も入る
	//======================================================================================
	struct PassContext
	{
		RenderGraph* pGraph = nullptr;						// 仮想/物理リソースを引く
		GraphicsEngine* pGraphicsEngine = nullptr;			// 描画アイテム・ライト・カメラを引く
		RenderContext* pRenderContext = nullptr;			// 実行時のみ
		D3D12::GraphicsCommandList* pCmdList = nullptr;		// 実行時のみ

		// スロットに割り当てられたGPUリソースを引く : 未割り当てなら nullptr
		D3D12::GPUResource* GetResource(const Slot& a_slot) const;
	};
}
