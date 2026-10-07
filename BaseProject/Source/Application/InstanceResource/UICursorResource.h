#pragma once

namespace App::InstanceResource
{
	//==========================================================================================
	//
	// UI のカーソルの取り合いを決めるワールドリソース。
	//
	// 画面に重ねて置いた UI のうち、いちばん手前の1つだけがカーソルを受け取れるようにする。
	//
	//   PreUpdate … 各 UI が「カーソルの上に居る」と名乗る(Claim)
	//   Update    … 自分が取れたかを見る(IsOwner)
	//
	// 名乗りが全員ぶん揃ってから決まるので、更新の順番に関係なく手前のものが勝つ。
	// 順番の決め方は描画と同じ : layer が大きいほど手前。
	// 同じ値なら後から名乗ったほう(＝後に描かれて上に乗るほう)が勝つ。
	//
	// ・以前はエンジンの ObjectContext が持っていたが、UI だけの都合なのでこちらへ移した。
	//   エンジン側は「フレームの番号」(ObjectContext::frameIndex)だけを配り、
	//   番号が変わった最初の名乗りで、ここが前のフレームの集計を捨てる。
	// ・書くのも読むのも UI(GameObject)で、ECS のシステムは触らない。
	//   シーンごとに作り直されるワールドに置いておけば、重ねたシーン(ポーズなど)とも混ざらない。
	//
	//==========================================================================================
	struct UICursorResource
	{
		// 集計しているフレーム
		uint64_t frameIndex = UINT64_MAX;

		// 受け取り手。アドレスの比較にしか使わない(実体は触らないので、
		// 名乗った相手がこの後に消えても安全)
		const void* pOwner = nullptr;

		float layer = 0.0f;			// 名乗った時点の重なり順
		bool isCapture = false;		// 押している最中の占有

		/// <summary>
		/// カーソルの上に自分が居ると名乗る(PreUpdate から)
		/// </summary>
		/// <param name="a_frameIndex">今のフレーム(ObjectContext::frameIndex)</param>
		/// <param name="a_pOwner">名乗る本人</param>
		/// <param name="a_layer">重なり順(大きいほど手前)</param>
		/// <param name="a_isCapture">
		/// 押している最中の占有。押し始めたものは、カーソルが外れても
		/// 重なり順に関係なく持ち続ける
		/// (押したまま手を滑らせただけで、下のものが光り始めるのを止めるため)
		/// </param>
		void Claim(uint64_t a_frameIndex, const void* a_pOwner, float a_layer, bool a_isCapture)
		{
			if (a_pOwner == nullptr) return;

			// 新しいフレームの最初の名乗り : 前のフレームの集計を捨てる
			if (frameIndex != a_frameIndex)
			{
				frameIndex = a_frameIndex;
				pOwner = nullptr;
				layer = 0.0f;
				isCapture = false;
			}

			if (pOwner != nullptr)
			{
				// 占有している相手が居るなら、同じ占有にしか譲らない
				if (isCapture && !a_isCapture) return;

				// 強さが同じなら手前が勝つ。同値のときは後から名乗ったほう
				if (isCapture == a_isCapture && a_layer < layer) return;
			}

			pOwner = a_pOwner;
			layer = a_layer;
			isCapture = a_isCapture;
		}

		/// <summary>自分がカーソルを受け取れるか(Update から)</summary>
		/// <remarks>このフレームに誰も名乗っていなければ、誰も受け取れない</remarks>
		bool IsOwner(uint64_t a_frameIndex, const void* a_pOwner) const
		{
			return a_pOwner != nullptr && frameIndex == a_frameIndex && pOwner == a_pOwner;
		}
	};
}
