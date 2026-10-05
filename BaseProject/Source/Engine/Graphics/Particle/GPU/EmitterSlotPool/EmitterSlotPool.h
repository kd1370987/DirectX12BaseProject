#pragma once
namespace Engine::Particle
{
	// 出現位置のワールド行列 : HLSL側(Common/RootParameters/Particle.hlsli)とそろえる
	struct EmitterTransform
	{
		Math::Matrix worldMat;
	};

	//======================================================================================
	// パーティクルを出す際のすべての共通の発生源テーブル
	//
	// ローカル空間で回す粒は、描くときに「どの発生源の座標系か」を席番号で引いて
	// ワールドへ戻す。その席を全アセット共通で1つの表にまとめたもの。
	//
	// ・GPU側は1本の StructuredBuffer。粒は席番号だけ持つので、
	//   ブロックごとにバッファを分けると粒ごとに読むバッファが変わってしまう。
	//   行列は毎フレーム全部送り直すので、足りなくなったら作り直すだけで済む(中身のコピー不要)。
	// ・CPU側は blockSize 単位で伸ばす。
	// ・席 0 は単位行列で予約(ワールド空間の粒がここを指す)。Init で取って二度と返さない。
	// ・返すときは「予約」だけして、まだ生きている粒が消えるまで待ってから空きへ戻す。
	//   すぐ戻すと、残っている粒の席を別の発生源が拾って粒が飛ぶ。
	//
	// ※ メインスレッドからだけ呼ぶこと(ロックは取っていない)
	//======================================================================================
	class EmitterSlotPool
	{
	public:

		//-------------------------------------------------------------------
		// 初期化・解放
		//-------------------------------------------------------------------
		void Init(uint32_t a_blockSize);	// 席 0 を単位行列で予約する
		void Release();						// GPUバッファも返す(DescriptorHeapManager より先に呼ぶ)

		//-------------------------------------------------------------------
		// 座席
		//-------------------------------------------------------------------
		Handle<EmitterTransform> Acquire();		// 座席の取得 : 取れなければ無効なハンドル
		// 自分の席として使えるか。
		// 席 0 は HandlePool の世代が 0 始まりのため id が 0 になり、ゼロ埋めされたハンドルと見分けがつかない。
		// 誰の持ち物でもないので、ここでは無効として扱う(持ち主は自分の席を取り直しに行く)
		bool IsValid(const Handle<EmitterTransform>& a_handle) const
		{
			return !(a_handle == m_identitySlot) && m_handlePool.IsValid(a_handle);
		}

		// 座席の操作
		// 拡縮を落として位置と回転だけを書く。世代が違う(返し終わった)ハンドルは無視
		void SetTransform(const Handle<EmitterTransform>& a_handle, const Math::Matrix& a_world);
		// a_holdSeconds 秒たったら空きへ戻す。それまでは席も行列もそのまま残る
		void ReserveReturn(const Handle<EmitterTransform>& a_handle, float a_holdSeconds);

		//-------------------------------------------------------------------
		// プールの更新
		//-------------------------------------------------------------------
		void BeginFrame(float a_dt);		// 返却待ちの残り時間を減らし 0 以下を HandlePool::Remove する

		// バッファの更新 : GPUに上げる。足りなければ作り直す
		void Upload(
			D3D12::Device* a_pDevice,
			D3D12::DescriptorHeapManager* a_pHeapManager,
			D3D12::GraphicsCommandList* a_pCmdList,
			UINT a_frameIndex
		);

		//-------------------------------------------------------------------
		// アクセサ
		//-------------------------------------------------------------------
		UINT GetSRVIndex() const;		// まだ一度も Upload していなければ無効値
		static uint32_t ToGPUIndex(const Handle<EmitterTransform>& a_handle);		// 無効なら 0

		// デバッグ表示用
		uint32_t GetLiveCount() const { return m_liveCount; }			// 使用中(返却待ちを含む。席 0 も含む)
		uint32_t GetPendingCount() const { return m_pendingCount; }		// 返却待ち
		uint32_t GetUsedCount() const { return m_usedCount; }			// 配ったことのある最大の席番号 + 1(毎フレーム転送する範囲)
		uint32_t GetCPUCapacity() const { return static_cast<uint32_t>(m_transforms.size()); }	// blockSize 単位で伸びる
		uint32_t GetGPUCapacity() const { return m_gpuCapacity; }
		uint32_t GetBlockSize() const { return m_blockSize; }

	private:

		// 拡縮を落として位置と回転だけを残す。
		// 取り付け側のスケール(ブースターは 0.1 倍など)を残したまま戻すと、
		// ローカルで進めた飛距離までそのスケールで縮んでしまう
		static Math::Matrix StripScale(const Math::Matrix& a_world);

	private:

		// 席ごとの返却待ち。handle が無効なら返却待ちではない
		struct PendingReturn
		{
			Handle<EmitterTransform> handle = {};
			float remain = 0.0f;
		};

		// 席番号は uint16 なので、これを超えて配れない
		static constexpr uint32_t SLOT_INDEX_MAX = 0xFFFF;

		Pool::HandlePool<EmitterTransform> m_handlePool;		// 上限なしで伸びる
		std::vector<EmitterTransform> m_transforms;				// 席番号 = 添え字 blockSize 単位で resize
		std::vector<PendingReturn> m_pendingReturns;			// 席番号 = 添え字(m_transforms と同じ長さ)

		Handle<EmitterTransform> m_identitySlot = {};	// 席 0(単位行列)。返さない

		uint32_t m_blockSize = 64;
		uint32_t m_usedCount = 0;		// 配った最大の席番号 + 1(転送する範囲)
		uint32_t m_liveCount = 0;		// 配って、まだ空きへ戻していない数
		uint32_t m_pendingCount = 0;	// 返却待ちの数

		// GPU側は１本 : 足りなくなったら作り直して古いほうは遅延開放
		std::unique_ptr<D3D12::StaticStructuredBuffer<EmitterTransform>> m_upGPUBuffer;
		uint32_t m_gpuCapacity = 0;
	};
}
