#pragma once
#include "Engine/Graphics/Raytracing/BLAS/BLAS.h"

namespace Engine::Graphics
{
	class FrameManager;
}

namespace Engine::Graphics::Raytracing
{
	//==========================================================================================
	// 静的 BLAS の圧縮
	//
	// 静的 BLAS は ALLOW_COMPACTION を付けてビルドし、ビルドが終わってから次の順で作り直す。
	//   1. GPU に圧縮後の大きさを書かせ、読み戻し用のバッファへ写す(フレーム N)
	//   2. フレーム N が GPU で終わったら大きさを読み、その大きさの実体を作って
	//      圧縮コピーを積み、BLAS の実体を差し替える(古い実体は GPU が使い終わってから手放す)
	// どちらも描画のコマンドリストに積む。TLAS はフレームごとに BLAS のアドレスを
	// 引き直すので、差し替えたフレームから新しい実体を指す。
	//
	// ビルドの完了は、ロードのバッチの完了通知(ワーカースレッド)で届く。
	// 受け口だけを共有で持ち、通知の側は弱参照で触るので、
	// エンジンの終了後に通知が来ても受け口が消えていれば何もしない
	//==========================================================================================
	class BLASCompactor
	{
	public:

		BLASCompactor() = default;
		~BLASCompactor();
		NON_COPYABLE_NON_MOVABLE(BLASCompactor);

		// 大きさの書き込み先と読み戻し先を作る
		bool Init(D3D12::Device* a_pDevice, const FrameManager* a_pFrameManager);

		// 途中の圧縮は諦めて、持っているものを手放す
		void Release();

		// ビルドが終わったら呼ぶ処理を作る。ロードのバッチの完了通知へ積む
		std::function<void()> MakeBuildCompleteNotifier(const std::shared_ptr<BLASCompactionTarget>& a_spTarget) const;

		// 毎フレーム、BLAS を読むもの(TLAS のビルド・レイトレのパス)より前に呼ぶ
		void Execute(D3D12::GraphicsCommandList* a_pCmdList);

		// 計測用 : 圧縮が済んだ数と、それで減った大きさ(バイト)
		uint32_t GetCompactedCount() const { return m_compactedCount; }
		uint64_t GetSavedBytes() const { return m_savedBytes; }

	private:

		// ビルドの完了通知の受け口。ワーカースレッドから積まれる
		struct Inbox
		{
			std::mutex mutex;
			std::vector<std::weak_ptr<BLASCompactionTarget>> targetVec = {};
		};

		// 圧縮 1 件ぶんの進み具合
		struct Job
		{
			std::weak_ptr<BLASCompactionTarget> wpTarget = {};
			uint32_t slot = INVALID_SLOT;			// 大きさの書き込み先の席。取れるまでは INVALID_SLOT
			uint64_t readyFenceValue = 0;			// 大きさが読めるようになるフレームのフェンス値(0 なら未依頼)
		};

		// GPU が使い終わるのを待って手放すもの
		struct Retired
		{
			ComPtr<ID3D12Resource> cpResource = nullptr;
			uint64_t fenceValue = 0;
		};

		// 受け口に届いたものを仕事に移す
		void TakeInbox();

		// 席の取れたものから、圧縮後の大きさを書かせる
		void RecordSizeQueries(D3D12::GraphicsCommandList* a_pCmdList);

		// 大きさが読めるようになったものを圧縮して差し替える
		void RecordCompactions(D3D12::GraphicsCommandList* a_pCmdList);

		// GPU が使い終わった古い実体を手放す
		void ReleaseRetired();

		// 大きさの書き込み先の席
		uint32_t AcquireSlot();
		void ReleaseSlot(uint32_t a_slot);

	private:

		// 同時に大きさを問い合わせられる数。溢れたぶんは次のフレームへ回る
		static constexpr uint32_t SLOT_COUNT = 256;
		static constexpr uint32_t INVALID_SLOT = UINT32_MAX;

		// 1 席の大きさ : 圧縮後の大きさ(UINT64)1 つぶん
		static constexpr UINT64 SLOT_SIZE = sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC);

		// 借り物 : 実体は GraphicsEngine(RenderDevice)が持っている
		D3D12::Device* m_pDevice = nullptr;
		const FrameManager* m_pFrameManager = nullptr;

		std::shared_ptr<Inbox> m_spInbox = nullptr;
		std::vector<Job> m_jobVec = {};
		std::vector<Retired> m_retiredVec = {};

		// 圧縮後の大きさを GPU が書く先(UAV)と、CPU が読む先
		ComPtr<ID3D12Resource> m_cpPostbuildInfo = nullptr;
		ComPtr<ID3D12Resource> m_cpReadback = nullptr;
		const UINT64* m_pReadbackData = nullptr;
		D3D12_RESOURCE_STATES m_postbuildInfoState = D3D12_RESOURCE_STATE_COMMON;

		std::vector<uint32_t> m_freeSlotVec = {};

		// 計測用
		uint32_t m_compactedCount = 0;
		uint64_t m_savedBytes = 0;
	};
}
