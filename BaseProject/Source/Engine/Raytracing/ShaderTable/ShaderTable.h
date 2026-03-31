#pragma once

namespace Engine::Raytracing
{
	class RayWorld;
	class RayPSO;

	class ShaderTable
	{
	public:

		// シェーダーテーブル初期化
		void Init(const RayWorld& a_rayWorld,RayPSO& a_rayPSO);

		// シェーダーテーブル更新
		void Update(const RayWorld& a_rayWorld,Engine::Raytracing::RayPSO& a_rayPSO);

		// ディスパッチレイ構造体取得
		const D3D12_DISPATCH_RAYS_DESC& GetDispatchDesc();

	private:

		// ディスパッチ設定作成
		D3D12_DISPATCH_RAYS_DESC CreateDispatchDesc(UINT a_instnaceNum);

		// シェーダーテーブルの構成サイズ計算
		void CalucShaderTableSize(UINT a_instanceNum);

		// テクスチャのハンドルを獲得
		D3D12_GPU_DESCRIPTOR_HANDLE GetTextureGPUHandle(const Resource::Handle<Resource::Texture>& a_texHandle);

	private:


		// シェーダーテーブル
		ComPtr<ID3D12Resource> m_cpShaderTable;
		uint8_t* m_pShaderTableData = nullptr;	// マップしておく

		// ディスパッチレイ構造体
		D3D12_DISPATCH_RAYS_DESC m_dispatchDesc;

		// シェーダーテーブルオフセット
		uint32_t m_rayGenOffset = 0;	// レイジェネレーションシェーダー開始位置
		uint32_t m_missOffset = 0;		// ミスシェーダー開始位置
		uint32_t m_hitOffset = 0;		// ヒットシェーダー開始位置

		// テーブルサイズ
		uint32_t m_tableSize = 0;	// テーブル全体のサイズ
		uint32_t m_recordSize = 0;		// 1レコードサイズ

	};
}