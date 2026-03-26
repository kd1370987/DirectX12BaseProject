#pragma once

namespace Engine::D3D12::Buffer
{
	// バイトデータでバッファを管理するクラス
	class Resource
	{
	public:

	private:
		ComPtr<ID3D12Resource> m_cpResource = nullptr;
	};
}