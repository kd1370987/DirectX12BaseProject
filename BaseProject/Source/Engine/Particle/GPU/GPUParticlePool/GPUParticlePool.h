#pragma once
#include "Engine/D3D12/GPUBuffer/RWStructuredBuffer/RWStructuredBuffer.h"		// GPU用UAV構造体バッファ
#include "../../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
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
		void Init(
			ID3D12Device* a_pDevice,
			ID3D12GraphicsCommandList* a_pCmdList,
			Engine::Handle<Resource::ParticlesAsset> a_particleHandle
		);

		/// <summary>
		/// エミッターデータを更新
		/// </summary>
		/// <param name="a_pCmdList">コマンドリスト</param>
		/// <param name="a_requests">リクエストデータ</param>
		void UploadEmitRequests(
			ID3D12GraphicsCommandList* a_pCmdList,
			std::span<const EmitterData> a_requests
		);
		
		// ---- アクセサ ----
		const Handle<D3D12::UAV>& GetParticlePoolUAV() const { return m_particlePool.GetUAVHandle(); }
		const Handle<D3D12::SRV>& GetParticlePoolSRV() const { return m_particlePool.GetSRVHandle(); }
		const Handle<D3D12::UAV>& GetDeadListUAV() const { return m_deadList.GetUAVHandle(); }
		const Handle<D3D12::UAV>& GetCounterUAV() const { return m_counterBuffer.GetUAVHandle(); }
		UINT GetMaxCapacity() const { return m_maxCapacity; }
		const Handle<D3D12::SRV>& GetEmitterBufferSRV() const { return m_emitterBuffer.GetSRVHandle(); }
		UINT GetEmitterCount() const { return m_emitterCount; }

	private:

		// 参照パーティクル
		Handle<Resource::ParticlesAsset> m_assetHandle;

		// GPU側データ
		// メインのパーティクルデータプール
		D3D12::RWStructuredBuffer<ParticleData> m_particlePool;

		// 空き番号管理用 DeadList
		D3D12::RWStructuredBuffer<uint32_t> m_deadList;

		// カウンター : 間接描画用バッファ(Indirect Argument Buffer)
		D3D12::RWStructuredBuffer<uint32_t> m_counterBuffer;

		// 最大容量 (アセットから取得したキャパシティ) 
		UINT m_maxCapacity = 10000;

		// いまフレームのエミットリクエストバッファ
		D3D12::StaticStructuredBuffer<EmitterData> m_emitterBuffer;
		UINT m_emitterCount = 0;
	};
}