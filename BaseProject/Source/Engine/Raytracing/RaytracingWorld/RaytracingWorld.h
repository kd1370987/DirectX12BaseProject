#pragma once

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
			const DirectX::XMFLOAT4X4& a_worldMat,
			const Engine::Resource::Handle<Engine::Resource::Model>& a_modelHandle
		);

		// レイトレワールド構築
		void Commit();

		// 
		void Update();

		// TLASアドレス取得
		D3D12_GPU_VIRTUAL_ADDRESS GetTLAS();
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVTLAS();

		// インスタンス配列取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetInstanceDataSRV();

		D3D12_GPU_DESCRIPTOR_HANDLE GetMaterialSRV();

		// インスタンス取得
		const std::vector<Instance>& GetInstnace() const { return m_instanceVec; }

	private:

		struct InstanceData
		{
			UINT vertexSRVIndex;
			UINT indexSRVIndex;
		};

		struct Material
		{
			DirectX::XMFLOAT4 baseColor = {1,1,1,1};
			int baseTexSRV = 0;
		};

		// GPU送信用データ
		Engine::D3D12::StructuredBuffer<InstanceData> m_instanceDataBuffer;
		Engine::D3D12::StructuredBuffer<Material> m_materialDataBuffer;

		std::vector<Instance> m_instanceVec = {};		// レイトレワールドインスタンス
		std::unique_ptr<TLAS> m_upTLAS = nullptr;		// レイトレワールドTLAS
	};
}