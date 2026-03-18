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

		void Create();

		void Commit();

		D3D12_GPU_VIRTUAL_ADDRESS GetTLAS();
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVTLAS();

	private:

		struct InstanceData
		{
			UINT vertexSRVIndex;
			UINT indexSRVIndex;
		};

		// GPU送信用データ
		Engine::D3D12::StructuredBuffer<InstanceData> m_instanceDataBuffer;

		std::vector<Instance> m_instanceVec = {};		// レイトレワールドインスタンス
		std::unique_ptr<TLAS> m_upTLAS = nullptr;		// レイトレワールドTLAS

		// テスト用
		struct Vertex
		{
			float x, y, z;
		};
		Vertex m_testVert[3] = {};
		uint32_t m_testIndex[3] = {};
		VertexBuffer m_testVertBuff;
		IndexBuffer m_testIndexBuff;

		std::unique_ptr<BLAS> m_upTestBLAS = nullptr;
		std::unique_ptr<TLAS> m_upTestTLAS = nullptr;
	};
}