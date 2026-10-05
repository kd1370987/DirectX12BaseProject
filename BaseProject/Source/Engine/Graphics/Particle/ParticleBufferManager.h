#pragma once

#include "Core/EmitterData.h"
#include "Core/ParticleData.h"

#include "GPU/GPUParticlePool/GPUParticlePool.h"

#include "../../Resource/Data/Particles/ParticlesAsset.h"

namespace Engine::Graphics
{
	class GraphicsEngine;
}

namespace Engine::Particle
{
	class EmitterSlotPool;

	class ParticleBufferManager
	{
	public:

		ParticleBufferManager();
		~ParticleBufferManager();
		NON_COPYABLE_NON_MOVABLE(ParticleBufferManager);

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="a_pGraphicsEngine">
		/// デバイスと非同期転送の依頼先(借り物)。プールは非同期に作られるので、そこまで持ち回る
		/// </param>
		void Init(
			Graphics::GraphicsEngine* a_pGraphicsEngine,
			D3D12::DescriptorHeapManager* a_pHeapManager,
			D3D12::GraphicsCommandList* a_pCmdList
		);

		/// <summary>
		/// 解放。GPUプール・エミットバッファが持つGPUリソースを破棄する。
		/// バッファはディスクリプタヒープにハンドルを持つため、
		/// DescriptorHeapManager の解放より前に呼ぶこと。
		/// </summary>
		void Release();

		/// <summary>
		/// フレームの開始に呼ぶ
		/// リクエストのクリアなど
		/// </summary>
		void BeginFrame(float a_dt);

		/// <summary>
		/// パーティクルを指定して、個数やデータを代入
		/// </summary>
		/// <param name="a_handle">パーティクルハンドル</param>
		/// <param name="a_emitterData">個数やデータ</param>
		void RequestEmit(const Handle<Resource::ParticlesAsset>& a_handle,const EmitterData& a_emitterData);

		//----------------------------------------------------------------------------------
		// 発生源の席(ローカル空間で回すパーティクル用)
		//
		// GPUプールはアセット単位なので、同じ噴射アセットを左右のブースターが使うと
		// 粒が1つのプールに混ざる。「どの発生源にくっついているか」を粒ごとに
		// 持たせないと、描くときに戻す行列を選べない。
		// そこで発生源ごとに席(番号)を配り、粒にはその番号だけを持たせている。
		// 席は全アセット共通の1つの表(EmitterSlotPool)。持ち主はエフェクト側が握る
		//----------------------------------------------------------------------------------
		EmitterSlotPool* RefEmitterSlotPool() { return m_upEmitterSlotPool.get(); }
		const EmitterSlotPool* GetEmitterSlotPool() const { return m_upEmitterSlotPool.get(); }

		/// <summary>
		/// パーティクルのバッファを取得
		/// </summary>
		/// <returns></returns>
		const std::unordered_map<Handle<Resource::ParticlesAsset>, std::unique_ptr<GPUParticlePool>>& GetPoolMap() const;

		/// <summary>
		/// ため込んだエミットデータを構造体バッファにマップする
		/// エミットデータ送信後、パス実行前の間に入れる必要あり
		/// a_frameIndex : 今のCPUフレーム番号(アップロード区画の選択に使う)
		/// </summary>
		void UploadEmitData(D3D12::GraphicsCommandList* a_pCmdList, UINT a_frameIndex);

		/// <summary>
		/// 現在たまっている生成命令をパーティクルを指定して取得
		/// </summary>
		/// <param name="a_assetHandle"></param>
		std::span <const EmitterData> GetRequests(const Handle<Resource::ParticlesAsset>& a_assetHandle) const;

		/// <summary>
		/// エミットバッファー取得
		/// </summary>
		const D3D12::StaticStructuredBuffer<EmitterData>* GetEmitBuffer(const Handle<Resource::ParticlesAsset>& a_handle) const;

		/// <summary>
		/// ランタイム時に非同期で読み込む関数
		/// </summary>
		void CreateParticleDataAsync(const Handle<Resource::ParticlesAsset>& a_handle);

		// 準備完了かどうか : BeginFrameで確定させる、メインスレッドでのみ触る
		bool IsReady(const Handle<Resource::ParticlesAsset>& a_handle) const { return m_readyHandles.contains(a_handle); }

	private:
		// ビューの置き場(借り物)。実体は GraphicsEngine が持っている。
		// プールは非同期に作られるので、Init で受け取ったものを持ち続ける
		D3D12::DescriptorHeapManager* m_pHeapManager = nullptr;

		// デバイスと非同期転送の依頼先(借り物)。持ち主は MainEngine
		Graphics::GraphicsEngine* m_pGraphicsEngine = nullptr;

		// アセットと 1対1 で紐づくバッファ群のマップ
		std::unordered_map<Handle<Resource::ParticlesAsset>, std::unique_ptr<GPUParticlePool>> m_pools;

		// 種類ごとの、今フレームの発生リクエスト（毎フレームクリアされる）
		std::unordered_map<Handle<Resource::ParticlesAsset>, std::vector<EmitterData>> m_emitRequests;


		std::unordered_map<Handle<Resource::ParticlesAsset>, D3D12::StaticStructuredBuffer<EmitterData>> m_emitBuffer;

		std::mutex m_mutex;
		std::unordered_set<Handle<Resource::ParticlesAsset>> m_loadingHandles;

		// メインスレッドだけが触る確定済みの集合
		std::unordered_set<Handle<Resource::ParticlesAsset>> m_readyHandles;	// 準備中パーティクルアセット達

		// 命令バッファのあふれを警告済みのアセット(毎フレーム出すとログが埋まるので1回だけ)
		std::unordered_set<Handle<Resource::ParticlesAsset>> m_overflowWarned;

		// エミット用の座席プール : すべてのパーティクルアセットのワールド座標と生存時間を管理
		std::unique_ptr<EmitterSlotPool> m_upEmitterSlotPool = nullptr;
	};
}