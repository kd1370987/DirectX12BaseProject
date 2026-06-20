#pragma once



namespace Engine::D3D12
{
	class CommandPool;

	/// <summary>
	/// D3D12関係のコマンド系をまとめるコンテナクラス
	/// </summary>
	class CommandContext
	{
	public:

		CommandContext();
		~CommandContext();

		/// <summary>
		/// 3つのプールを初期化する
		/// </summary>
		void Init(Device* a_pDevice);

		CommandPool* RefDirectPool();
		CommandPool* RefCopyPool();
		CommandPool* RefComputePool();

	private:
		
		std::unique_ptr<CommandPool> m_upDirectCmdPool = nullptr;
		std::unique_ptr<CommandPool> m_upCopyCmdPool = nullptr;
		std::unique_ptr<CommandPool> m_upComputeCmdPool = nullptr;
	};
}