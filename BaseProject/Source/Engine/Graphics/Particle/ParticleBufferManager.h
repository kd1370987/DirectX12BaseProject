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
		/// フレームの描画が全部終わった後、コマンドリストを流す前に呼ぶ
		/// </summary>
		/// <remarks>
		/// 間接描画の引数を COMMON へ戻して、「このフレームの引数を用意した」印を下ろす。
		/// バッファは ExecuteCommandLists が終わると COMMON に戻る(decay)が、
		/// GPUResource は CPU 側で状態を覚えているだけなので、明示して戻しておかないと
		/// 次のフレームの遷移で「遷移前の状態」が食い違う。
		/// カメラごとに何度描いても引数は同じなので、戻すのはフレームに1回でよい
		/// </remarks>
		void FinishFrame(D3D12::GraphicsCommandList* a_pCmdList);

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

		// 命令バッファがあふれたことがあるか(デバッグ表示用。警告を出したアセット)
		bool HasOverflowed(const Handle<Resource::ParticlesAsset>& a_handle) const { return m_overflowWarned.contains(a_handle); }

		//----------------------------------------------------------------------------------
		// 起きているか(更新と描画を回す必要があるか)
		//
		// 最後に粒を出してから、そのアセットの最大寿命(+ 少しの余裕)が経ったプールには
		// 生きている粒が1つも残っていない。そういうプールは更新も描画も丸ごと飛ばす。
		// 更新と描画は粒の数ではなく容量ぶん走るので、出していないプールほど無駄が大きい。
		// 判定は CPU 側だけで済む(GPU から生存数を読み戻さない)。
		//
		// 一度も出していないプールは眠っている。アセットが引けないときは安全側(起きている)
		//----------------------------------------------------------------------------------
		bool IsAwake(const Handle<Resource::ParticlesAsset>& a_handle) const;

		// 最後に粒を出してからの秒数(デバッグ表示用)。一度も出していなければ負
		double GetSecondsSinceLastEmit(const Handle<Resource::ParticlesAsset>& a_handle) const;

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

		//------------------------------------------------------------------
		// 眠っているプールを見分けるための時刻(メインスレッドのみ)
		//
		// 時刻は BeginFrame で積む経過時間。更新シェーダーと同じフレーム時間で進むので、
		// 粒の寿命の減り方と食い違わない。
		// 「最後に出した時刻」は命令を実際に GPU へ送ったとき(UploadEmitData)に記録する。
		// 準備中に積まれた命令は数フレーム遅れて出るので、積んだ時刻では早すぎる
		//------------------------------------------------------------------
		double m_elapsedTime = 0.0;
		std::unordered_map<Handle<Resource::ParticlesAsset>, double> m_lastEmitTime;
	};
}