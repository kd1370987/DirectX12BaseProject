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
			const DXSM::Matrix& a_worldMat,
			const Engine::Resource::Handle<Engine::Resource::Model>& a_modelHandle
		);

		// 初期化
		void Init(uint32_t a_hitGroupNum);

		// レイトレワールド構築
		void Commit();

		// インスタンスのクリア
		void Clear();

		// TLASアドレス取得
		D3D12_GPU_VIRTUAL_ADDRESS GetTLAS();
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVTLAS();

		// インスタンス配列取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetInstanceDataSRV();
		D3D12_CPU_DESCRIPTOR_HANDLE GetInstanceDataSRVCPU();

		D3D12_GPU_DESCRIPTOR_HANDLE GetMaterialSRV();
		D3D12_CPU_DESCRIPTOR_HANDLE GetMaterialSRVCPU();

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
			DXSM::Vector4 baseColor = { 1,1,1,1 };
			float						metallic = 0.0f;						// B : 金属製
			float						roughness = 1.0f;						// G : 粗さ
			DirectX::XMFLOAT3			emissive = { 1.0f,1.0f,1.0f };

			int baseIndex = 0;
		};

		// GPU送信用データ
		Engine::D3D12::StaticStructuredBuffer<InstanceData> m_instanceDataBuffer;
		std::vector<InstanceData> m_instanceDataVec = {};
		Engine::D3D12::StaticStructuredBuffer<Material>     m_materialDataBuffer;
		std::vector<Material> m_materialVec = {};

		std::vector<Instance> m_instanceVec = {};		// レイトレワールドインスタンス
		std::unique_ptr<TLAS> m_upTLAS = nullptr;		// レイトレワールドTLAS

		// 更新
		bool m_isCommit = false;		// コミットされたかどうか
	};
}