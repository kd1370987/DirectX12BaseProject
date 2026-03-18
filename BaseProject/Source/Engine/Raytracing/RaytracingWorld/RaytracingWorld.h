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
	};
}