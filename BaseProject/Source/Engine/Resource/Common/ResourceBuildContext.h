#pragma once
namespace Engine::Resource
{
	/// <summary>
	/// リソースビルド時に必要なクラスの参照、コマンドリストなどを橋渡しする
	/// </summary>
	struct ResourceBuildContext
	{
		D3D12::GraphicsCommandList* pDirectCmdList = nullptr;
	};
}