#pragma once

namespace Engine::Graphics
{
	class GraphicsEngine;

	//==========================================================================================
	// 計測結果(スナップショット)
	//
	// Capture した時点の描画まわりの使われ方を値で写したもの。
	// GPUリソースへのポインタは持たないので、取った後にリソースが作り直されても読める
	//==========================================================================================

	// 容量のあるもの1つぶんの使われ方。席(ディスクリプタ)でも要素(バッファ)でも同じ形で持つ
	struct GraphicsUsageProfile
	{
		std::string	name = {};
		uint64_t	used = 0;			// 今使っている数
		uint64_t	capacity = 0;		// 全体の数
		uint64_t	peak = 0;			// 一番多く使っていたときの数
		size_t		strideBytes = 0;	// 1要素のバイト数(ディスクリプタの席なら 0)
	};

	// ディスクリプタヒープ
	struct DescriptorHeapProfile
	{
		// CBV / SRV / UAV / RTV / DSV / ImGui SRV / ImGui Backend の順。
		// peak はヒープを作ってからの値(取っていない間の増減も入る)
		std::vector<GraphicsUsageProfile> views = {};

		// 解放を預かっている(GPUが使い終わるのを待っている)席の数。種類はまとめて数える
		size_t pendingFreeCount = 0;
	};

	// メガバッファ1本(メッシュを詰め込む大きなバッファ)。単位は要素数
	struct MegaBufferProfile
	{
		std::string	name = {};
		size_t		strideBytes = 0;
		uint32_t	capacity = 0;			// 全体
		uint32_t	allocated = 0;			// 配っている数(返却待ちを含む)
		uint32_t	pending = 0;			// 返されたが、GPUが使い終わるのを待っている数
		uint32_t	peakAllocated = 0;		// 作ってから一番多く配っていたとき
		uint32_t	freeBlockCount = 0;		// 空き領域の塊の数(多いほど断片化している)
		uint32_t	largestFreeBlock = 0;	// 一番大きい空き領域 : 一度に確保できる上限
	};

	// 定数バッファ(フレームごとのアップロード領域)。単位はバイト
	struct ConstantBufferProfile
	{
		size_t graphicsUsedBytes = 0;
		size_t graphicsCapacityBytes = 0;
		size_t graphicsPeakBytes = 0;		// 計測を始めてからの値
		size_t computeUsedBytes = 0;
		size_t computeCapacityBytes = 0;
		size_t computePeakBytes = 0;		// 計測を始めてからの値
	};

	// 描画要求の数(このフレームに積まれたもの)
	struct DrawCountProfile
	{
		size_t drawItemCount = 0;			// 描画アイテム(サブセット × 受け取るパスの数)
		size_t instanceCount = 0;			// インスタンスデータ
		size_t materialCount = 0;			// サブセットのマテリアル
		size_t skinningCount = 0;			// GPUスキニングの呼び出し
		size_t dynamicRayRequestCount = 0;	// アニメーションするメッシュのレイトレ用インスタンス
		size_t uiCount = 0;
		size_t boneMatrixCount = 0;
		size_t debugLineCount = 0;
		size_t groundImpulseCount = 0;
	};

	// パイプラインステート
	struct PipelineStateProfile
	{
		size_t psoCount = 0;
		size_t rootSignatureCount = 0;
	};

	// ビデオメモリ(DXGI の見積もり)。単位はバイト
	struct VideoMemoryProfile
	{
		bool		isValid = false;		// 取れなかったら false
		uint64_t	localUsage = 0;			// VRAM : このプロセスが使っている量
		uint64_t	localBudget = 0;		// VRAM : OS がこのプロセスに許している量
		uint64_t	nonLocalUsage = 0;		// 共有メモリ(システムRAM側)
		uint64_t	nonLocalBudget = 0;
	};

	// 描画まわり全体
	struct GraphicsSnapshot
	{
		uint64_t							captureCount = 0;		// 何回目の Capture か(0 なら未取得)
		UINT								renderWidth = 0;
		UINT								renderHeight = 0;
		VideoMemoryProfile					videoMemory = {};
		DescriptorHeapProfile				descriptorHeap = {};
		std::vector<MegaBufferProfile>		megaBuffers = {};
		std::vector<GraphicsUsageProfile>	frameBuffers = {};		// 毎フレーム詰め直す構造体バッファ(peak は計測を始めてから)
		ConstantBufferProfile				constantBuffer = {};
		DrawCountProfile					drawCount = {};
		PipelineStateProfile				pipelineState = {};
	};

	//==========================================================================================
	// 描画まわりのプロファイラ
	//
	// メガバッファ・ディスクリプタヒープ・定数バッファ・毎フレームの構造体バッファなど、
	// 「容量が決まっていて、溢れると描画が壊れるもの」の使われ方を取る。
	// 表示はエディター側(ProfilerPanel の Graphics 表示)に任せる。
	//
	// 取るのは GraphicsEngine::Execute の終わり(描画要求が出そろい、EndFrame で消える前)。
	// 毎フレーム取ると無駄なので、見る側が RequestCapture で頼んだフレームだけ取る。
	// 誰も見ていなければ計測の負荷はかからない。
	//
	// 持ち主の GraphicsEngine は const でしか見ないので、ここから描画の状態は変えられない
	//==========================================================================================
	class GraphicsProfiler
	{
	public:

		explicit GraphicsProfiler(const GraphicsEngine* a_pOwner);
		~GraphicsProfiler();

		// コピー禁止(持ち主のグラフィックスエンジンに1つ)
		NON_COPYABLE_NON_MOVABLE(GraphicsProfiler);

		// 次の Execute の終わりに取ってほしいと頼む。見る側が表示しているあいだ毎フレーム呼ぶ
		void RequestCapture() { m_isCaptureRequested = true; }

		// 頼まれていれば取り直す。GraphicsEngine::Execute の終わりから呼ぶ
		void CaptureIfRequested();

		// 直近の Capture の結果
		const GraphicsSnapshot& GetSnapshot() const { return m_snapshot; }

		// 計測を始めてからの最大値(定数バッファ・毎フレームの構造体バッファ)を捨てる。
		// ディスクリプタヒープとメガバッファの最大値は持ち主側が数えているので消えない
		void ResetPeaks();

	private:

		//------------------------------------------------------------------------------------------
		// Capture の中身
		//------------------------------------------------------------------------------------------
		void Capture();
		void CaptureVideoMemory();
		void CaptureDescriptorHeap();
		void CaptureMegaBuffers();
		void CaptureFrameBuffers();
		void CaptureConstantBuffer();
		void CaptureDrawCount();
		void CapturePipelineState();

	private:

		// 計測対象(見るだけ)
		const GraphicsEngine* m_pOwner = nullptr;

		// 直近の Capture の結果
		GraphicsSnapshot m_snapshot = {};

		// 次の Execute で取るか
		bool m_isCaptureRequested = false;

		// 計測を始めてからの最大値。フレームの頭で 0 に戻るものは、ここで覚えておかないと残らない
		size_t m_cbGraphicsPeakBytes = 0;
		size_t m_cbComputePeakBytes = 0;
		std::vector<uint64_t> m_frameBufferPeakVec = {};	// frameBuffers と同じ並び
	};
}
