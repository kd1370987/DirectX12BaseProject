#pragma once

namespace Engine::D3D12
{
	class DescriptorHeapManager;
}

namespace Engine::Raytracing
{

	class TLAS
	{
	public:

		// 作成
		void Create(
			D3D12::Device* a_pDevice,
			D3D12::DescriptorHeapManager* a_pHeapManager,
			D3D12::GraphicsCommandList* a_pCmdList,
			UINT a_maxInstanceNum
		);

		// 解放
		void Release();

		// 更新
		// a_frameIndex : 今のCPUフレーム番号(0 ～ CPU_FRAME_COUNT-1)。
		// インスタンスバッファはフレームごとに区画を分けてあり、この番号の区画へ書く
		void Update(D3D12::GraphicsCommandList* a_pCmdList, const std::vector<Instance>& a_instanceVec, UINT a_frameIndex);

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
		{
			return m_cpResource->GetGPUVirtualAddress();
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle();

	private:

		void CreateBuffer(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			ComPtr<ID3D12Resource>& a_cpRes,
			uint64_t a_size,
			D3D12_RESOURCE_FLAGS a_flags,
			D3D12_RESOURCE_STATES a_initState,
			const D3D12_HEAP_PROPERTIES& a_heapProps
		);

	private:

		// TLAS本体
		ComPtr<ID3D12Resource> m_cpResource = nullptr;
		uint64_t m_resultSize = 0;

		// scratchBuffer
		ComPtr<ID3D12Resource> m_cpScratch = nullptr;
		uint64_t m_scratchSize = 0;

		//--------------------------------------------------------------------------------------------
		// インスタンスバッファ
		//
		// UPLOADヒープをマップしたまま、CPUが毎フレーム書き込む。
		// CPUはGPUより最大 CPU_FRAME_COUNT-1 フレーム先を走るので、1本を使い回すと
		// 前のフレームのビルドがまだ読んでいる中身を上書きしてしまう。
		// フレームの数だけ区画を持ち、今のフレーム番号の区画へ書く
		//--------------------------------------------------------------------------------------------
		ComPtr<ID3D12Resource> m_cpInstanceBuffer = nullptr;
		uint32_t m_maxInstanceCount = 0;									// 1区画に入るインスタンス数(Create で決まる)
		D3D12_RAYTRACING_INSTANCE_DESC* m_pInstanceDesc = nullptr;		// バッファ全体の先頭(マップしておく)
		bool m_isOverflowReported = false;									// 上限超えを知らせたか(毎フレーム言わないため)

		// SRVハンドルと、その置き場(借り物)。
		// 実体は GraphicsEngine が持っているので、Create で受け取ったものを控えて Release で返す
		Engine::Handle<D3D12::SRV> m_srvHandle = {};
		D3D12::DescriptorHeapManager* m_pHeapManager = nullptr;
	};
}