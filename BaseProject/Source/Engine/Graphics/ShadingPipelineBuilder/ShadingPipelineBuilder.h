#pragma once

#include "../PSOKey.h"

namespace Engine::Graphics
{
	//class RenderGraph;

	///// <summary>
	///// マテリアルやエンティティのフラグから、PSOを検索、生成する場所
	///// </summary>
	//class ShadingPipelineBuilder
	//{
	//public:
	//	/// <summary>
	//	/// シェーディングパイプラインが見つかれば返し、なければ作成して返します
	//	/// 非同期のためPSOがないフレームが出てくるので対処が必須
	//	/// </summary>
	//	/// <param name="a_key">PSO検索用キー</param>
	//	/// <returns>PSOマネージャーから帰ってきたハンドルを返す</returns>
	//	Handle<ID3D12PipelineState> Request(PSOKey a_key, RenderGraph* a_pRenderGraph, D3D12::PipelineStateManager* a_pPSOManager);

	//private:

	//	// PSOの作成用キーハッシュとハンドル
	//	std::unordered_map<PSOKey, Handle<ID3D12PipelineState>> m_psoMap = {};

	//	// コンパイル中のPSOたち
	//	std::unordered_set<PSOKey> m_compilingPasses = {};;

	//};
}