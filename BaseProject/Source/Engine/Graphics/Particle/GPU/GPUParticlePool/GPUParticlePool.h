#pragma once
#include "Engine/Graphics/D3D12/GPUBuffer/RWStructuredBuffer/RWStructuredBuffer.h"		// GPU用UAV構造体バッファ
#include "../../../../Resource/Data/Particles/ParticlesAsset.h"
#include "../../Core/EmitterData.h"
#include "../../Core/ParticleData.h"

namespace Engine::Graphics::Particle
{
	/// <summary>
	/// １種類のアセットに対するGPU上のバッファ群を束ねるクラス
	/// </summary>
	class GPUParticlePool
	{
	public:

		/// <summary>
		/// 初期化 : GPUに命令を出すためCrose前のCmdListを渡すこと
		/// </summary>
		/// <param name="a_pDevice">デバイスポインタ</param>
		/// <param name="a_pCmdList">コマンドリストポインタ</param>
		/// <param name="a_particleHandle">パーティクルアセットのハンドル</param>
		/// <param name="a_capacity">最初の容量(ToInitialCapacity で決めたもの)。足りなくなったら BeginGrow で伸ばす</param>
		bool Init(
			Graphics::D3D12::Device* a_pDevice,
			Graphics::D3D12::DescriptorHeapManager* a_pHeapManager,
			Graphics::D3D12::GraphicsCommandList* a_pCmdList,
			Engine::Handle<Resource::ParticlesAsset> a_particleHandle,
			UINT a_capacity
		);

		//------------------------------------------------------------------
		// 容量を伸ばす(フレームの中、発生の Dispatch より前に呼ぶ)
		//
		//   BeginGrow : 粒バッファ・デッドリスト・生存リストを大きく作り直し、
		//               古い粒とデッドリストを新しい先頭へ写す。古い3本は GPU が使い終わってから返す。
		//               カウンターと間接描画の引数は大きさが変わらないので作り直さない。
		//               終わると粒・デッドリスト・カウンターは UAV の状態
		//   (呼ぶ側)   : 増えた範囲を 0 で埋め、その番号をデッドリストへ積み、カウンターを足す(GrowParticlePool CS)
		//   EndGrow   : 3本を COMMON へ戻す(フレームの残りは今まで通り COMMON から使う)
		//
		// 粒の番号は変わらない(先頭へそのまま写す)ので、生きている粒はそのまま動き続ける
		//------------------------------------------------------------------
		bool BeginGrow(
			Graphics::D3D12::Device* a_pDevice,
			Graphics::D3D12::DescriptorHeapManager* a_pHeapManager,
			Graphics::D3D12::GraphicsCommandList* a_pCmdList,
			UINT a_newCapacity
		);
		void EndGrow(Graphics::D3D12::GraphicsCommandList* a_pCmdList);
		UINT GetGrowFromCapacity() const { return m_growFromCapacity; }		// 直近の BeginGrow の前の容量

		/// <summary>
		/// 持っているバッファをすべて返す(ディスクリプタヒープの席も返す)
		/// </summary>
		/// <remarks>
		/// バッファは壊すだけではディスクリプタを返さないので、捨てる前に必ず呼ぶこと。
		/// GPU がまだ読んでいるかもしれないときは、呼ぶ側が遅延させる
		/// (ParticleBufferManager::DestroyPool が ReserveRelease で呼ぶ)
		/// </remarks>
		void Release();

		// ---- アクセサ ----
		const Handle<Graphics::D3D12::UAV>& GetParticlePoolUAV() const { return m_particlePool.GetUAV(); }
		const Handle<Graphics::D3D12::SRV>& GetParticlePoolSRV() const { return m_particlePool.GetSRV(); }
		const Handle<Graphics::D3D12::UAV>& GetDeadListUAV() const { return m_deadList.GetUAV(); }
		const Handle<Graphics::D3D12::UAV>& GetCounterUAV() const { return m_counterBuffer.GetUAV(); }
		UINT GetMaxCapacity() const { return m_maxCapacity; }			// いまの容量(伸びる)
		UINT GetInitialCapacity() const { return m_initialCapacity; }	// 作ったときの容量(縮めるときの戻り先)

		// アセットの Capacity(最初に用意しておく数の目安)から、最初の容量を決める。
		// ブロック単位に切り上げる。0 でも 1 ブロックは用意する
		static UINT ToInitialCapacity(int a_assetCapacity)
		{
			return RoundUpToPoolBlock(static_cast<uint64_t>((std::max)(a_assetCapacity, 0)));
		}

		//------------------------------------------------------------------
		// 間接描画の引数
		//
		// シミュレーションが毎フレーム書き(リセット CS → Update)、ParticlePass が ExecuteIndirect で読む。
		// 状態の約束 : フレームの頭は COMMON。シミュレーションで UAV → INDIRECT_ARGUMENT と進め、
		// フレームの終わり(ParticleBufferManager::FinishFrame)で COMMON へ戻す。
		// IsArgsReady はこのフレームの引数を用意したか。描画はこれが立っているプールだけ
		//------------------------------------------------------------------
		Graphics::D3D12::RWStructuredBuffer<uint32_t>& RefDrawArgs() { return m_drawArgs; }
		ID3D12Resource* GetDrawArgsResource() const { return m_drawArgs.GetResource(); }
		bool IsArgsReady() const { return m_isArgsReady; }
		void SetArgsReady(bool a_isReady) { m_isArgsReady = a_isReady; }

		//------------------------------------------------------------------
		// 生存リスト : 生きている粒の番号を詰めて並べたもの
		//
		// Update が生き残った粒を積み(数は間接引数のインスタンス数へ足す)、
		// VS が SV_InstanceID で引く。描画は生きている粒の数ぶんだけ走る。
		// 状態の約束は引数と同じ : フレームの頭は COMMON。Update の前に UAV、後に NON_PIXEL_SHADER_RESOURCE、
		// フレームの終わり(FinishFrame)で COMMON へ戻す
		//------------------------------------------------------------------
		Graphics::D3D12::RWStructuredBuffer<uint32_t>& RefAliveList() { return m_aliveList; }
		const Handle<Graphics::D3D12::UAV>& GetAliveListUAV() const { return m_aliveList.GetUAV(); }
		const Handle<Graphics::D3D12::SRV>& GetAliveListSRV() const { return m_aliveList.GetSRV(); }

		// UAVバリア用の生リソース。
		// Emit(取り出し)と Update(返却)は同じデッドリスト/カウンターを触るため、
		// Dispatch の間で同期を取る必要がある。
		ID3D12Resource* GetParticlePoolResource() const { return m_particlePool.GetResource(); }
		ID3D12Resource* GetDeadListResource()     const { return m_deadList.GetResource(); }
		ID3D12Resource* GetCounterResource()      const { return m_counterBuffer.GetResource(); }

	private:

		// 参照パーティクル
		Handle<Resource::ParticlesAsset> m_assetHandle;

		// GPU側データ
		Graphics::D3D12::RWStructuredBuffer<ParticleData> m_particlePool;		// メインのパーティクルデータプール
		Graphics::D3D12::RWStructuredBuffer<uint32_t> m_deadList;				// 空き番号管理用 DeadList
		Graphics::D3D12::RWStructuredBuffer<uint32_t> m_counterBuffer;		// カウンター : デッドリストに残っている空き番号の数

		// 間接描画の引数 : D3D12_DRAW_INDEXED_ARGUMENTS(uint ×5)
		static constexpr UINT DRAW_ARGS_ELEMENT_NUM = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) / sizeof(uint32_t);
		Graphics::D3D12::RWStructuredBuffer<uint32_t> m_drawArgs;
		bool m_isArgsReady = false;									// このフレームの引数を用意したか

		// 生存リスト : 容量ぶん(生きている粒は容量を超えない)
		Graphics::D3D12::RWStructuredBuffer<uint32_t> m_aliveList;

		// いまの容量。足りなくなったら BeginGrow でブロック単位に伸びる
		UINT m_maxCapacity = 0;

		// 作ったときの容量(眠っている間に縮めるときの戻り先)
		UINT m_initialCapacity = 0;

		// 直近の BeginGrow の前の容量(増えた範囲を埋める CS が使う)
		UINT m_growFromCapacity = 0;
	};
}