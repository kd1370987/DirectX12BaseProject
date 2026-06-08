#pragma once
namespace Engine::D3D12
{
	class CommandAllocator
	{
	public:

		// コンストラクタ・デストラクタ
		CommandAllocator() = default;
		~CommandAllocator() { Release(); }

		// 単体生成
		bool Create(ID3D12Device* a_pDevice,D3D12_COMMAND_LIST_TYPE a_commandListType);

		// 解放
		void Release();

		// リセット
		void Reset();

		// コマンドアロケーターの取得
		ID3D12CommandAllocator* Get()
		{
			return m_cpCommandAllocator.Get();
		}

	private:
		ComPtr<ID3D12CommandAllocator> m_cpCommandAllocator = nullptr;
	};
}