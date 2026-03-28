#pragma once

namespace Engine::Raytracing
{
	class RayWorld;
	class RayPSO;

	class ShaderTable
	{
	public:

		// シェーダーテーブル初期化
		void Init(
			const RayWorld& a_rayWorld,
			RayPSO& a_rayPSO
		);

		const D3D12_DISPATCH_RAYS_DESC& GetDispatchDesc()
		{
			return m_dispatchDesc;
		}

	private:
		// 各シェーダー数をカウントする
		void CountUpNumShader();

		// シェーダーテーブルの構成サイズ計算
		void CulcShaderTableSize(UINT a_instanceNum);

	private:


		// シェーダーテーブル
		ComPtr<ID3D12Resource> m_cpShaderTable;

		D3D12_DISPATCH_RAYS_DESC m_dispatchDesc;

		uint32_t m_shaderTableEntrySize = 0;
		int m_numRayGenShader = 0;
		int m_numMissShader = 0;
		int m_numHitShader = 0;

		// シェーダーテーブルオフセット
		uint32_t m_rayGenOffset = 0;	// レイジェネレーションシェーダー開始位置
		uint32_t m_missOffset = 0;		// ミスシェーダー開始位置
		uint32_t m_hitOffset = 0;		// ヒットシェーダー開始位置

		// テーブルサイズ
		uint32_t m_tableSize = 0;	// テーブル全体のサイズ
		uint32_t m_recordSize = 0;		// 1レコードサイズ

	};
}