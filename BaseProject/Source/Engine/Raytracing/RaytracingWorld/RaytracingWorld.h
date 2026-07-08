#pragma once

#include "../Common/RaytracingInstance.h"

namespace Engine
{
	namespace ECS
	{
		class World;
	}
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
			const DXSM::Matrix& a_worldMat,
			const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
			const DXSM::Vector4& a_colorScale,
			const DXSM::Vector3& a_emissiveScale
		);
		void Register(
			ECS::World& a_world,
			const DXSM::Matrix& a_worldMat,
			const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
			const Handle<DynamicRaytracingData>& a_dynamicData,
			const RangeHandle<Resource::NodePoseMatrix>& a_nodeposeMatVec,
			const DXSM::Vector4& a_colorScale,
			const DXSM::Vector3& a_emissiveScale
		);

		// 初期化
		void Init(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList, 
			uint32_t a_hitGroupNum
		);

		// 解放
		void Release();

		// レイトレワールド構築
		// マイフレーム構築
		void Commit(D3D12::GraphicsCommandList* a_pCmdList);

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
		const std::vector<Instance>& GetInstnace() const { return m_instanceVec; }
	private:

		int GetTexHepaIndex(const Handle<Resource::Texture>& a_handle) const;

	private:

		// GPU送信用データ
		Engine::D3D12::StaticStructuredBuffer<InstanceData> m_instanceDataBuffer;
		std::vector<InstanceData> m_instanceDataVec = {};
		Engine::D3D12::StaticStructuredBuffer<Material>     m_materialDataBuffer;
		std::vector<Material> m_materialVec = {};

		std::vector<Instance> m_instanceVec = {};		// レイトレワールドインスタンス
		std::unique_ptr<TLAS> m_upTLAS = nullptr;		// レイトレワールドTLAS

		// 更新
		bool m_isCommit = false;		// コミットされたかどうか
		bool m_isDrity = false;
	};
}