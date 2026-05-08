#include "PipelineStateManager.h"
namespace Engine::D3D12
{
	void PipelineStateManager::Init(ID3D12Device* a_pDevice)
	{
		m_pDevice = a_pDevice;
	}
	ID3D12RootSignature* PipelineStateManager::Request(const D3D12_ROOT_SIGNATURE_DESC& a_desc)
	{
		return nullptr;
	}
	ID3D12PipelineState* PipelineStateManager::Request(const D3D12::GraphicsPipelineDesc& a_desc)
	{
		// ハッシュを求める
		uint64_t _hash = CalcHash(&a_desc.desc,sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		// キャッシュ検索
		if (m_psoMap.contains(_hash))
		{
			return m_psoMap[_hash].Get();
		}

		// キャッシュになければ、ComPtrを作成
		ComPtr<ID3D12PipelineState> _pso;
		m_pDevice->CreateGraphicsPipelineState(&a_desc.desc,IID_PPV_ARGS(&_pso));

		// 名前があれば
		if (!a_desc.name.empty())
		{
			_pso->SetName(StringUtility::ToWideString(a_desc.name).c_str());
		}

		// マップに保存して生ポインタを返す
		m_psoMap[_hash] = _pso;
		return _pso.Get();
	}

	uint64_t PipelineStateManager::CalcHash(const void* a_pData, size_t a_size)
	{
		const uint8_t* _ptr = static_cast<const uint8_t*>(a_pData);
		uint64_t _hash = 14695981039346656037ull;
		for (size_t _i = 0; _i < a_size; ++_i)
		{
			_hash ^= _ptr[_i];
			_hash *= 1099511628211ull;
		}
		return _hash;
	}
}