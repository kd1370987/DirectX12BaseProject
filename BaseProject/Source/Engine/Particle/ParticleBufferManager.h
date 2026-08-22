#pragma once

#include "Core/EmitterData.h"
#include "Core/ParticleData.h"

#include "GPU/GPUParticlePool/GPUParticlePool.h"

#include "../Resource/Data/Particles/ParticlesAsset.h"

namespace Engine::Particle
{
	class ParticleBufferManager
	{
	public:

		/// <summary>
		/// 初期化
		/// </summary>
		void Init(
			D3D12::Device* a_pDevice,
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
		void BeginFrame();

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
		//
		// 席 0 は単位行列で予約。ワールド空間で回す粒はここを指すので、
		// 描画側は分岐なしで同じ掛け算を通せる。
		//----------------------------------------------------------------------------------

		/// <summary>
		/// 発生源の席を確保して番号を返す。すでに配ってあれば行列だけ更新する
		/// </summary>
		/// <param name="a_ownerKey">発生源を見分ける鍵(エンティティIDなど)</param>
		/// <param name="a_ownerWorld">発生源のワールド行列</param>
		/// <returns>席の番号。席が尽きたら 0(=ワールド空間として出る)</returns>
		/// <remarks>
		/// 渡された行列からは拡縮を落として、位置と回転だけを覚える。
		/// 落とさないと、取り付け側のスケール(ブースターは 0.1 倍など)が
		/// そのまま粒の飛距離に掛かってしまう。
		/// </remarks>
		uint32_t AcquireEmitterSlot(
			const Handle<Resource::ParticlesAsset>& a_handle,
			uint64_t a_ownerKey,
			const Math::Matrix& a_ownerWorld);

		/// <summary>
		/// 席の行列一覧。描画時に定数バッファへ積む
		/// </summary>
		std::span<const Math::Matrix> GetEmitterMatrices(const Handle<Resource::ParticlesAsset>& a_handle) const;

		/// <summary>
		/// パーティクルのバッファを取得
		/// </summary>
		/// <returns></returns>
		const std::unordered_map<Handle<Resource::ParticlesAsset>, std::unique_ptr<GPUParticlePool>>& GetPoolMap() const;

		/// <summary>
		/// ため込んだエミットデータを構造体バッファにマップする
		/// エミットデータ送信後、パス実行前の間に入れる必要あり
		/// </summary>
		void UploadEmitData(D3D12::GraphicsCommandList* a_pCmdList);

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

		/// <summary>
		/// 指定したアセットが現在ロード中かどうか
		/// </summary>
		bool IsLoading(const Handle<Resource::ParticlesAsset>& a_handle);

		/// <summary>
		/// 指定したパーティクルのロード処理が終わっているかどうか
		/// </summary>
		bool IsLoaded(const Handle<Resource::ParticlesAsset>& a_handle);

	private:
		// アセットと 1対1 で紐づくバッファ群のマップ
		std::unordered_map<Handle<Resource::ParticlesAsset>, std::unique_ptr<GPUParticlePool>> m_pools;

		// 種類ごとの、今フレームの発生リクエスト（毎フレームクリアされる）
		std::unordered_map<Handle<Resource::ParticlesAsset>, std::vector<EmitterData>> m_emitRequests;


		std::unordered_map<Handle<Resource::ParticlesAsset>, D3D12::StaticStructuredBuffer<EmitterData>> m_emitBuffer;

		//------------------------------------------------------------------
		// アセットごとの発生源の席
		//
		// フレームを跨いで保つ(粒より先に席が消えると、まだ生きている粒の行列が引けない)。
		//
		// ただし持ちっぱなしにはしない。席は1アセットあたり
		// PARTICLE_EMITTER_MAX 個しか無く、鍵はエンティティなので、
		// 出しては消えるもの(シーンを読み直すたびに作り直されるブースター等)が
		// 席を取ったまま居なくなると、そのうち席が尽きて
		// 後から来たものが全部ワールド空間で出てしまう。
		//
		// そこで最後に使われたフレームを覚えておき、席が尽きたときは
		// 「しばらく使われていない席」を回して使う。
		// 十分に間を空けてから回すので、生きている粒の行列を奪うことにはならない。
		//------------------------------------------------------------------
		struct EmitterSlotTable
		{
			std::unordered_map<uint64_t, uint32_t> slotMap = {};	// 鍵 → 席番号
			std::vector<Math::Matrix> matrices = {};				// 席番号 → 行列([0] は単位行列)
			std::vector<uint64_t> slotOwners = {};				// 席番号 → 鍵([0] は未使用)
			std::vector<uint64_t> slotUsedFrame = {};			// 席番号 → 最後に使われたフレーム
		};
		std::unordered_map<Handle<Resource::ParticlesAsset>, EmitterSlotTable> m_emitterSlots;

		// 席の使用状況を測るためのフレーム番号(BeginFrame で進める)
		uint64_t m_frameCount = 0;


		std::mutex m_mutex;
		std::unordered_set<Handle<Resource::ParticlesAsset>> m_loadingHandles;
	};
}