#pragma once

#include "../Common/RaytracingInstance.h"

namespace Engine
{
	namespace ECS
	{
		class World;
	}
}

namespace Engine::D3D12
{
	class DescriptorHeapManager;
}

namespace Engine::Raytracing
{
	class TLAS;

	class RayWorld
	{
	public:

		RayWorld();
		~RayWorld();

		// モデルとワールド行列を登録して内部でインスタンスに返還
		void Register(
			const Math::Matrix& a_worldMat,
			const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
			const Math::Color& a_colorScale,
			const Math::Vector3& a_emissiveScale,
			const Math::Vector3& a_emissiveAdd = { 0,0,0 }
		);
		void Register(
			ECS::World& a_world,
			const Math::Matrix& a_worldMat,
			const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
			const Handle<DynamicRaytracingData>& a_dynamicData,
			const RangeHandle<Resource::NodePoseMatrix>& a_nodeposeMatVec,
			const Math::Color& a_colorScale,
			const Math::Vector3& a_emissiveScale,
			const Math::Vector3& a_emissiveAdd = { 0,0,0 }
		);

		// 初期化
		void Init(
			D3D12::Device* a_pDevice,
			D3D12::DescriptorHeapManager* a_pHeapManager,
			D3D12::GraphicsCommandList* a_pCmdList,
			Resource::ResourceManager* a_pResourceManager
		);

		// 登録されたモデルのメッシュ・マテリアル・テクスチャを引く先(借り物)
		void SetResourceManager(Resource::ResourceManager* a_pResourceManager) { m_pResourceManager = a_pResourceManager; }

		// 解放
		void Release();

		// レイトレワールド構築
		// 毎フレーム構築。a_frameIndex は今のCPUフレーム番号(TLASのインスタンス区画の選択に使う)
		void Commit(D3D12::GraphicsCommandList* a_pCmdList, UINT a_frameIndex);

		// インスタンスのクリア
		void Clear();

		// TLASアドレス取得
		D3D12_GPU_VIRTUAL_ADDRESS GetTLAS();
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVTLAS();

		Handle<D3D12::SRV> GetInstanceBufferSRV();
		Handle<D3D12::SRV> GetMaterialBufferSRV();

		// インスタンス配列取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetInstanceDataSRV();
		D3D12_CPU_DESCRIPTOR_HANDLE GetInstanceDataSRVCPU();

		D3D12_GPU_DESCRIPTOR_HANDLE GetMaterialSRV();
		D3D12_CPU_DESCRIPTOR_HANDLE GetMaterialSRVCPU();

		// インスタンス取得
		const std::vector<Instance>& GetInstanceVec() const { return m_instanceVec; }
	private:

		int GetTexHeapIndex(const Handle<Resource::Texture>& a_handle) const;

	private:

		// GPU送信用データ
		Engine::D3D12::StaticStructuredBuffer<InstanceData> m_instanceDataBuffer;
		std::vector<InstanceData> m_instanceDataVec = {};
		Engine::D3D12::StaticStructuredBuffer<Material>     m_materialDataBuffer;
		std::vector<Material> m_materialVec = {};

		// ビューの置き場(借り物)。実体は GraphicsEngine が持っている
		D3D12::DescriptorHeapManager* m_pHeapManager = nullptr;

		// リソースの持ち主(借り物)。実体は MainEngine が持っている
		Resource::ResourceManager* m_pResourceManager = nullptr;

		std::vector<Instance> m_instanceVec = {};		// レイトレワールドインスタンス
		std::unique_ptr<TLAS> m_upTLAS = nullptr;		// レイトレワールドTLAS

		// 更新
		bool m_isCommit = false;		// コミットされたかどうか
		bool m_isDirty = false;
	};
}