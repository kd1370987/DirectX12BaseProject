#pragma once

#include "../PSOKey.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderGraph;

	

	/// <summary>
	/// マテリアルやエンティティのフラグから、PSOを検索、生成する場所
	/// パスにつき一つ持たせる
	/// </summary>
	class ShadingPipelineBuilder
	{
	public:

		/// <summary>
		/// レンダーグラフのコンパイル時に、このパスのフォーマットを登録しておく
		/// </summary>
		/// <param name="a_rtvFormat">レンダーターゲット配列</param>
		/// <param name="a_dsvFormat">デプスステンシル</param>
		void Init(const std::vector<DXGI_FORMAT>& a_rtvFormat, DXGI_FORMAT a_dsvFormat, UINT a_passNameHash);

		/// <summary>
		/// シェーディングパイプラインが見つかれば返し、なければ作成して返します
		/// 非同期のためPSOがないフレームが出てくるので対処が必須
		/// </summary>
		/// <param name="a_key">PSO検索用キー</param>
		/// <returns>PSOマネージャーから帰ってきたハンドルを返す</returns>
		Handle<ID3D12PipelineState> Request(PSOKey a_key, RenderGraph* a_pRenderGraph, D3D12::PipelineStateManager* a_pPSOManager);

		/// <summary>
		/// フラグに対応したVSを登録する
		/// </summary>
		/// <param name="a_flag">フラグ</param>
		/// <param name="a_vsHandle">VSハンドル</param>
		void RegisterVertexShader(EShaderPermutationFlags a_flag, Handle<Resource::Shader> a_vsHandle);

	private:

		// PSOの作成用キーハッシュとハンドル
		std::unordered_map<PSOKey, Handle<ID3D12PipelineState>> m_psoMap = {};

		// コンパイル中のPSOたち
		std::unordered_set<PSOKey> m_compilingPasses = {};;

		std::vector<DXGI_FORMAT> m_rtvFormats = {};
		DXGI_FORMAT m_dsvFormat;

		Handle<ID3D12PipelineState> m_fallbackPSO;

		UINT m_passNameHash;

		// フラグに対応するVSのマップ
		std::unordered_map<EShaderPermutationFlags, Handle<Resource::Shader>> m_vsMap;
	};
}