#pragma once

namespace Engine::Raytracing
{
	class RayWorld;
	class RayPSO;

	struct ShaderTableRecord
	{
		void* shaderID;										// シェーダーID
		std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> texHandle;	// テクスチャのGPUハンドル
	};

	struct ShaderTableInit
	{
		const RayPSO*				pRayPSO = nullptr;
		std::vector<RayShaderData>	shaderData = {};
		std::vector<HitGroup>		hitGroup = {};
		UINT						maxInstance = 1000;

		uint32_t maxLocalRootSize = 0;		// ローカルルートシグネチャの最大サイズ
	};

	class ShaderTable
	{
	public:

		// シェーダーテーブル初期化
		void Init(const ShaderTableInit& a_shaderInit);

		// シェーダーテーブル更新
		void Update(const RayWorld& a_rayWorld);

		// ディスパッチレイ構造体取得
		const D3D12_DISPATCH_RAYS_DESC& GetDispatchDesc();

	private:

		// ディスパッチ設定作成
		D3D12_DISPATCH_RAYS_DESC CreateDispatchDesc(UINT a_instnaceNum);

		// シェーダーテーブルの構成サイズ計算
		void CalucShaderTableSize(
			UINT a_instanceNum
		);
		// シェーダー数計算
		void CalucShaderNum(
			const RayPSO* a_rayPSO, 
			const std::vector<RayShaderData>& a_shaderData, 
			const std::vector<HitGroup>& a_hitGroup
		);

		// テクスチャのハンドルを獲得
		D3D12_GPU_DESCRIPTOR_HANDLE GetTextureGPUHandle(const Resource::Handle<Resource::Texture>& a_texHandle);

		
	private:

		// シェーダーテーブル
		ComPtr<ID3D12Resource> m_cpShaderTable;
		uint8_t* m_pShaderTableData = nullptr;	// マップしておく

		// シェーダーID
		const void* m_rayGenID;	// レイジェネレーションシェーダーIDのベクター
		std::vector<const void*> m_missIDVec;		// ミスシェーダーIDのベクター
		std::vector<const void*> m_hitIDVec;		// ヒットシェーダーIDのベクター

		uint32_t m_maxLocalRootSigSize = 0;		// ローカルルートシグネチャの最大サイズ

		// ディスパッチレイ構造体
		D3D12_DISPATCH_RAYS_DESC m_dispatchDesc;

		// シェーダーテーブルオフセット
		uint32_t m_rayGenOffset = 0;	// レイジェネレーションシェーダー開始位置
		uint32_t m_missOffset = 0;		// ミスシェーダー開始位置
		uint32_t m_hitOffset = 0;		// ヒットシェーダー開始位置

		// テーブルサイズ
		uint32_t m_tableSize = 0;		// テーブル全体のサイズ
		uint32_t m_recordSize = 0;		// 1レコードサイズ
	};
}