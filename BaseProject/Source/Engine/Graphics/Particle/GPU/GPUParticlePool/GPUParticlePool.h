#pragma once
#include "Engine/Graphics/D3D12/GPUBuffer/RWStructuredBuffer/RWStructuredBuffer.h"		// GPU用UAV構造体バッファ
#include "../../../../Resource/Data/Particles/ParticlesAsset.h"
#include "../../Core/EmitterData.h"
#include "../../Core/ParticleData.h"

namespace Engine::Particle
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
		/// <param name="a_resourceManager">アセットの値を引く先</param>
		bool Init(
			D3D12::Device* a_pDevice,
			D3D12::DescriptorHeapManager* a_pHeapManager,
			D3D12::GraphicsCommandList* a_pCmdList,
			Engine::Handle<Resource::ParticlesAsset> a_particleHandle,
			const Resource::ResourceManager& a_resourceManager
		);

		/// <summary>
		/// 持っているバッファをすべて返す(ディスクリプタヒープの席も返す)
		/// </summary>
		/// <remarks>
		/// バッファは壊すだけではディスクリプタを返さないので、捨てる前に必ず呼ぶこと。
		/// GPU がまだ読んでいるかもしれないときは、呼ぶ側が遅延させる
		/// (ParticleBufferManager::DestroyPool が RegisterDeferredResource で呼ぶ)
		/// </remarks>
		void Release();

		// ---- アクセサ ----
		const Handle<D3D12::UAV>& GetParticlePoolUAV() const { return m_particlePool.GetUAV(); }
		const Handle<D3D12::SRV>& GetParticlePoolSRV() const { return m_particlePool.GetSRV(); }
		const Handle<D3D12::UAV>& GetDeadListUAV() const { return m_deadList.GetUAV(); }
		const Handle<D3D12::UAV>& GetCounterUAV() const { return m_counterBuffer.GetUAV(); }
		UINT GetMaxCapacity() const { return m_maxCapacity; }

		// アセットの Capacity から、プールの容量を決める(1 以上)。
		// エディターでは 0 にもできるので、0 個のバッファを作らないよう下限を付ける。
		// プールを作るときと「アセットの値と食い違っているか」を比べるときで、必ず同じ丸め方にすること
		static UINT ToPoolCapacity(int a_assetCapacity) { return static_cast<UINT>((std::max)(a_assetCapacity, 1)); }

		//------------------------------------------------------------------
		// 間接描画の引数
		//
		// シミュレーションが毎フレーム書き(リセット CS → Update)、ParticlePass が ExecuteIndirect で読む。
		// 状態の約束 : フレームの頭は COMMON。シミュレーションで UAV → INDIRECT_ARGUMENT と進め、
		// フレームの終わり(ParticleBufferManager::FinishFrame)で COMMON へ戻す。
		// IsArgsReady はこのフレームの引数を用意したか。描画はこれが立っているプールだけ
		//------------------------------------------------------------------
		D3D12::RWStructuredBuffer<uint32_t>& RefDrawArgs() { return m_drawArgs; }
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
		D3D12::RWStructuredBuffer<uint32_t>& RefAliveList() { return m_aliveList; }
		const Handle<D3D12::UAV>& GetAliveListUAV() const { return m_aliveList.GetUAV(); }
		const Handle<D3D12::SRV>& GetAliveListSRV() const { return m_aliveList.GetSRV(); }

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
		D3D12::RWStructuredBuffer<ParticleData> m_particlePool;		// メインのパーティクルデータプール
		D3D12::RWStructuredBuffer<uint32_t> m_deadList;				// 空き番号管理用 DeadList
		D3D12::RWStructuredBuffer<uint32_t> m_counterBuffer;		// カウンター : デッドリストに残っている空き番号の数

		// 間接描画の引数 : D3D12_DRAW_INDEXED_ARGUMENTS(uint ×5)
		static constexpr UINT DRAW_ARGS_ELEMENT_NUM = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) / sizeof(uint32_t);
		D3D12::RWStructuredBuffer<uint32_t> m_drawArgs;
		bool m_isArgsReady = false;									// このフレームの引数を用意したか

		// 生存リスト : 容量ぶん(生きている粒は容量を超えない)
		D3D12::RWStructuredBuffer<uint32_t> m_aliveList;

		// 最大容量 (アセットから取得したキャパシティ) 
		UINT m_maxCapacity = 10000;
 


	};
}