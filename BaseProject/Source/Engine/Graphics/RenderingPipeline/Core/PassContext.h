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

namespace Engine
{
	class MainEngine;
}

namespace Engine::Raytracing
{
	class RayEngine;
}

namespace Engine::Resource
{
	class ResourceManager;
	class AssetDatabase;
}

namespace Engine::Particle
{
	class ParticleBufferManager;
}

namespace Engine::D3D12
{
	class DescriptorHeapManager;
}

namespace Engine::Graphics::Pipeline
{
	class RenderGraph;
	struct Slot;

	//======================================================================================
	// パスへ渡す実行コンテキスト
	//
	// Compile 時は pGraph だけが入る(実行系はまだ無い)。
	// Update 時は pRenderContext / pCmdList も入る。
	//
	// パスからシングルトンを直に引かないための入口でもある。
	// アプリ寿命のものが要るようになったら、パスの中で Instance() を呼ばずに
	// ここへ足して RenderGraph::MakeContext() で配ること。
	// 直に引くと「そのパスが何に依存しているか」が呼び出し側から見えなくなり、
	// プレビュー用の別ワールドのように実体が2つある場面で取り違える
	//======================================================================================
	struct PassContext
	{
		//----------------------------------------------------------------------------------
		// アプリ寿命のもの : Compile / Update のどちらでも入っている
		//----------------------------------------------------------------------------------
		MainEngine*						pMainEngine			= nullptr;
		Resource::ResourceManager*		pResourceManager	= nullptr;
		Resource::AssetDatabase*		pAssetDatabase		= nullptr;
		Raytracing::RayEngine*			pRayEngine			= nullptr;
		Particle::ParticleBufferManager* pParticleManager	= nullptr;

		// ビューの置き場(借り物)。実体は GraphicsEngine が持っている。
		// ハンドルからCPU/GPUハンドルを引きたいパスはここから借りること
		D3D12::DescriptorHeapManager*	pHeapManager		= nullptr;

		//----------------------------------------------------------------------------------
		// 描画系
		//----------------------------------------------------------------------------------
		RenderGraph*					pGraph				= nullptr;	// 仮想/物理リソースを引く
		GraphicsEngine*					pGraphicsEngine		= nullptr;	// 描画アイテム・ライト・カメラを引く
		RenderContext*					pRenderContext		= nullptr;	// 実行時のみ
		D3D12::GraphicsCommandList*		pCmdList			= nullptr;	// 実行時のみ

		// スロットに割り当てられたGPUリソースを引く : 未割り当てなら nullptr
		D3D12::GPUResource* GetResource(const Slot& a_slot) const;
	};
}
