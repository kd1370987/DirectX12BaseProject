#pragma once
//==========================================================================================
//
// AliasingReport (Engine::Graphics::Pipeline)
//
// リソースの使い回し(エイリアシング)の結果を、外から読むためだけに1枚へまとめたもの。
//
// RenderGraph は これを持たない。見たいときに BuildAliasingReport() で組む。
// 持たせてしまうとコンパイルのたびに更新する義務が増え、
// 実行に要らないものが実行経路へ紛れ込む。
//
// 中身は「そのときの写し」なので、コンパイルを通すと古くなる。
// 抱え込まず、必要になったところで組み直すこと
//
//==========================================================================================
#include "../../Core/ResourceID.h"

namespace Engine::Graphics::Pipeline
{
	class RenderGraph;

	//======================================================================================
	// 矩形1つぶん = 仮想リソース1本
	//
	// 1リソース = 1つの生存区間なので、1本が1つの矩形になる
	//======================================================================================
	struct AliasingReportEntry
	{
		ResourceID resourceID = {};
		std::string name = "";

		// 席 : スロットに着けなかったものは AllocationInfo::INVALID_SLOT_INDEX
		uint32_t slotIndex = 0;

		// 生存区間 : 閉区間。lastPassIndex のパスも触っている
		uint32_t firstPassIndex = 0;
		uint32_t lastPassIndex = 0;

		// ヒープ上の位置と、実際に使うぶんの大きさ
		uint64_t offset = 0;
		uint64_t size = 0;

		// 同じ席を直前に使っていたリソース : 無効なら、この席の一人目
		ResourceID prevResourceID = {};
		std::string prevName = "";

		// 席に着けなかった理由 : 着けているなら nullptr
		const char* pUnaliasableReason = nullptr;
	};

	// 席1つぶん
	struct AliasingReportSlot
	{
		uint64_t offset = 0;			// ヒープ内での開始位置
		uint64_t reservedSize = 0;		// この席が押さえている大きさ(同居する中で一番大きいもの)
		uint64_t alignment = 0;

		uint64_t usedSizeMax = 0;		// 実際に使われた最大 : reservedSize との差が無駄
	};

	// 実際に積まれたエイリアシングバリア。
	// 並びから推測した継ぎ目と突き合わせると、張り忘れが見つかる
	struct AliasingReportBarrier
	{
		uint32_t passIndex = 0;
		ResourceID before = {};
		ResourceID after = {};
	};

	//======================================================================================
	// 割り当て結果の写し
	//======================================================================================
	struct AliasingReport
	{
		std::vector<std::string> passNames = {};				// 実行順のパス名 : そのまま横軸になる

		std::vector<AliasingReportSlot> slots = {};				// 席 : そのまま縦軸になる
		std::vector<AliasingReportEntry> entries = {};			// 席に着いたもの
		std::vector<AliasingReportEntry> unassignedEntries = {};	// 着けなかったもの

		std::vector<AliasingReportBarrier> barriers = {};

		uint64_t heapSize = 0;			// 席を全部並べたときの総量
		uint64_t totalResourceSize = 0;	// 使い回さずに1本ずつ作った場合の総量

		bool IsEmpty() const { return passNames.empty(); }
	};

	//--------------------------------------------------------------------------------------
	// グラフの今の割り当てから1枚組み立てる。
	// コンパイルを通していなければ空のまま返る
	//--------------------------------------------------------------------------------------
	AliasingReport BuildAliasingReport(const RenderGraph& a_graph);
}
