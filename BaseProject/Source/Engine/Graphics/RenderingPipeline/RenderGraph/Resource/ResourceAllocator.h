#pragma once

#include "../../Core/ResourceID.h"

namespace Engine::Graphics::Pipeline
{
	class VirtualResource;

	/// <summary>
	/// 割り当てられたと仮定した際のリソース
	/// </summary>
	struct AllocationSlot
	{
		uint64_t offset = 0;					// ヒープ内でこのスロットの開始位置
		uint64_t allocationSize = 0;
		uint64_t allocationAlignment = 0;

		// このスロットを最後に使用したVirtualResuorceの終了パス
		uint32_t lastPassIndex = 0;

		ResourceID lastResourceID;
	};

	/// <summary>
	/// エイリアシング計算、実際のリソースが割り当てられる際の制御
	/// </summary>
	class ResourceAllocator
	{
	public:

		/// <summary>
		/// ヒープサイズと各スロットのオフセットを決める
		/// </summary>
		/// <param name="a_virtualResources"></param>
		void CalcAllocation(std::vector<VirtualResource>& a_virtualResources);

		//----------------------------------------------------------------------------------
		// 席の使い回しをするか(デバッグ用の切り替え)
		//
		// 切ると 1リソース = 1席になる。実体はヒープ上に置かれたまま(placed)だが、
		// 生存区間が重ならないもの同士でも席を分け合わないので、絵は使い回し前と同じになる。
		//
		// 絵がおかしいときにここを切って直れば原因は使い回し側、
		// 切っても直らなければ置き場所(placed)側、と切り分けられる。
		// 移行が終わっても残しておく価値がある
		//
		// 全グラフに効く。切り替えたら Compile からやり直すこと
		//----------------------------------------------------------------------------------
		static void SetAliasingEnabled(bool a_isEnabled) { s_isAliasingEnabled = a_isEnabled; }
		static bool IsAliasingEnabled() { return s_isAliasingEnabled; }

		// ヒープの定義取得
		uint32_t GetMaxUageSlot() const { return m_maxUageSlot; }
		uint64_t GetMaxHeapSize() const { return m_maxHeapSize; }

		// 割り当ての結果。使い回しの様子を外から覗くのに使う
		const std::vector<AllocationSlot>& GetSlots() const { return m_slots; }

	private:

		std::vector<AllocationSlot> m_slots;
		uint32_t m_maxUageSlot = 0;
		uint64_t m_maxHeapSize = 0;

		// 使い回しを入れた状態が既定。
		// 絵がおかしいときに切って直れば、原因は置き場所ではなく使い回し側
		inline static bool s_isAliasingEnabled = true;
	};
}