#pragma once

namespace Engine::D3D12
{
	class GPUBuffer
	{
	public:

		
	private:
		ComPtr<ID3D12Resource> m_cpBuffer = nullptr;			// バッファ本体
	};
}