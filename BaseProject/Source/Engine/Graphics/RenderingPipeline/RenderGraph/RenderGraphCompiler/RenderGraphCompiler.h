#pragma once
//==========================================================================================
//
// RenderGraphCompiler (Engine::Graphics::Pipeline)
//
// レンダーグラフの「実行順とリソースの解決」だけを担当する。
// GPUには一切触らないので、物理リソースが無くても最後まで通る。
//
//   実行順を決める → 配線をスロットへ反映 → 仮想リソースを組む → バリアを積む
//
// 結果は CompileResult で返し、RenderGraph のコンパイル済みデータへは書き込まない。
// (どこへ書くかは呼んだ側の都合なので、こちらは知らなくてよい)
//
//==========================================================================================
#include "../Internal/CompiledPass.h"

namespace Engine::Graphics::Pipeline
{
	class Pass;
	class RenderGraph;

	// =====================================================================================
	// コンパイル結果
	//
	// 実行に必要なものはこれで全部。失敗したときは isSuccess が false で中身は空
	// =====================================================================================
	struct CompileResult
	{
		bool isSuccess = false;								// 実行できる形になったか

		std::vector<CompiledPass> compiledPassVec = {};		// 実行順に並んだパス
		std::vector<ResourceBarrier> endBarrierVec = {};	// 全パスの後にフレーム入口へ戻すバリア
	};

	/// <summary>
	/// レンダーグラフのコンパイルを担当する
	/// </summary>
	class RenderGraphCompiler
	{
	public:

		explicit RenderGraphCompiler(RenderGraph* a_pRenderGraph) : m_pRenderGraph(a_pRenderGraph) {}
		~RenderGraphCompiler() = default;

		/// <summary>
		/// レンダーグラフをコンパイルして実行用データを作る
		/// </summary>
		/// <returns>コンパイル結果 : 失敗したら isSuccess が false</returns>
		CompileResult Compile();

		/// <summary>
		/// バリアだけを積み直す
		/// </summary>
		/// <remarks>
		/// Compile の時点ではまだ物理リソースが無く、フレーム入口のステートが仮のままになる。
		/// 実体を作った後にもう一度ここを通して before を本物へ揃える
		/// </remarks>
		/// <param name="a_compiledPassVec">実行順に並んだパス : preBarriers を差し替える</param>
		/// <param name="a_outEndBarrierVec">フレーム終端のバリア : 中身を作り直す</param>
		void BuildBarriers(
			std::vector<CompiledPass>& a_compiledPassVec,
			std::vector<ResourceBarrier>& a_outEndBarrierVec);

	private:

		//----------------------------------------------------------------------------------
		// 実行順フェーズ
		//----------------------------------------------------------------------------------
		// 繋いだ相手だけを辺にして一本へ並べる : 循環していたら false
		bool BuildExecutionOrder(std::vector<Pass*>& a_outSortedVec);

		// 出力ピンへ識別子を覚えさせる : どのパスのどの出力ピンかを確定させる
		void StampResourceIDs();

		// 配線を実行順に一回だけ解決する : 依存順が解決しているパスがいる
		void ResolveLinksInOrder(const std::vector<Pass*>& a_sortedVec);

		// 同じリソースを重複して使っていないか : 外部リソースが名前で判別しているため
		void ValidateResourceWriters();

		//----------------------------------------------------------------------------------
		// 仮想リソースフェーズ
		//----------------------------------------------------------------------------------
		void BuildVirtualResources();	// 出来上がった配線から仮想リソースを組みなおす
		void ResolveStoreOps();			// 出力したリソースが後続で使われるかどうかを StoreOp に落とす

		// リソースの生存区間を出す : 各仮想リソースが持つ
		void BuildResourceLifetimes(const std::vector<CompiledPass>& a_compiledPassVec);

		//----------------------------------------------------------------------------------
		// バリア構築フェーズ
		//----------------------------------------------------------------------------------
		// パス一つ分のバリアを積む
		void BuildPassBarriers(CompiledPass& a_compiledPass);

	private:

		// コンパイル時のみ参照を受け取る : 所有はしない
		RenderGraph* m_pRenderGraph = nullptr;
	};
}
