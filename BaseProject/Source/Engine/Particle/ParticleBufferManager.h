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


		std::mutex m_mutex;
		std::unordered_set<Handle<Resource::ParticlesAsset>> m_loadingHandles;
	};
}