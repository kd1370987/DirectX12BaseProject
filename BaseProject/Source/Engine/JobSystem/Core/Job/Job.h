#pragma once
namespace Engine::Thread
{
	class JobWorker;

	//==========================================================================================
	// ジョブシステムに任せる際の処理の入れ物
	//
	// 依存関係は「後続リスト(continuations)」で表現する。
	//   waitingCount   : 自分が実行できるようになるまでに終わってほしい先行ジョブの数
	//   continuations  : 自分が終わったときに待ち数を減らしてやる後続ジョブ
	//
	// 「自分が待つ相手」を持つのではなく「自分を待っている相手」を持つ。
	// 先行ジョブが終わった瞬間に後続を辿れないと、誰も後続を起こせないため。
	//
	// ---- 登録と完了のすれ違いについて ----
	// 後続を登録しようとした時点で先行ジョブがもう終わっていると、
	// 登録しても二度と通知されず、後続が永久に待ち続ける。
	// これを防ぐため、完了フラグの書き込みと後続リストの操作は
	// 同じミューテックスの下で行い、
	// 「登録できなかった(=すでに終わっていた)」を呼び出し側へ返す
	//==========================================================================================
	struct Job
	{
		// 1つのジョブが持てる後続の数
		static constexpr uint32_t MAX_CONTINUATION_COUNT = 15;

		Job() = default;

		// ミューテックスとアトミックを持つのでコピーもムーブも不可
		Job(const Job&) = delete;
		Job& operator=(const Job&) = delete;

		/// <summary>
		/// プールから取り出したときの初期化
		/// 使い回すため、前回の内容が残らないよう必ず通すこと
		/// </summary>
		void Reset();

		/// <summary>
		/// 後続を登録する
		/// </summary>
		/// <param name="a_pJob">自分の完了を待っているジョブ</param>
		/// <returns>
		/// 登録できたら true。
		/// false は「すでに完了済み」または「後続リストが溢れた」で、
		/// どちらも通知が飛ばないため、呼び出し側で待ち数を減らすこと
		/// </returns>
		bool AddContinuation(Job* a_pJob);

		/// <summary>
		/// 完了印を付けて、後続リストを取り出す
		/// 以降 AddContinuation は false を返すようになる
		/// </summary>
		/// <param name="a_outContinuations">後続の受け取り先</param>
		/// <returns>取り出した後続の数</returns>
		uint32_t FinishAndTakeContinuations(std::array<Job*, MAX_CONTINUATION_COUNT>& a_outContinuations);

		// 実行する処理
		std::function<void()> task;

		// 自分が実行可能になるまでに終わってほしい先行ジョブの数
		// 0 になった時点でキューへ積まれる
		std::atomic<uint32_t> waitingCount = 0;

	private:

		// 自分の完了で待ち数が減る後続ジョブ
		std::array<Job*, MAX_CONTINUATION_COUNT>	m_continuations = {};
		uint32_t									m_continuationCount = 0;

		// 完了済みか
		// 未使用スロットは true から始める : プールの踏み潰し検出に使う
		std::atomic<bool>							m_isFinished = true;

		// 完了フラグと後続リストをまとめて守る
		std::mutex									m_continuationMutex;

		friend class JobPool;
	};
}
