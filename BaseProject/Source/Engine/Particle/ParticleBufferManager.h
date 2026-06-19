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
		void Init();

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

		const std::unordered_map<Handle<Resource::ParticlesAsset>, std::unique_ptr<GPUParticlePool>>& GetPoolMap() const;

		const std::vector<EmitterData>& GetRequests(const Engine::GUID& a_assetGuid) const;

	private:
		// アセット(GUID) と 1対1 で紐づくバッファ群のマップ
		std::unordered_map<Engine::GUID, std::unique_ptr<GPUParticlePool>> m_pools;

		// 種類(GUID) ごとの、今フレームの発生リクエスト（毎フレームクリアされる）
		std::unordered_map<Engine::GUID, std::vector<EmitterData>> m_emitRequests;

	};
}