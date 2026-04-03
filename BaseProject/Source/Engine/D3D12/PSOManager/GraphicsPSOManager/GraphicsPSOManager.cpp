#include "GraphicsPSOManager.h"

#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
namespace Engine::D3D12
{
	void GraphicsPSOManager::Init(ID3D12Device* a_pDevice)
	{
		m_pDevice = a_pDevice;
	}

	Resource::Handle<PipelineState> GraphicsPSOManager::Request(const GraphicsPipelineDesc& a_desc)
	{
		// 重なり防止
		auto _it = m_handleMap.find(a_desc.name);
		if (_it != m_handleMap.end())
		{
			return _it->second;
		}

		// ハンドル取得
		Resource::Handle<PipelineState> _handle = m_handleStorage.Allocate();
		
		// データを入れる
		if (m_pipelineData.size() <= _handle.idx)
		{
			m_pipelineData.resize(_handle.idx + 1);
		}
		m_pipelineData[_handle.idx].Create(m_pDevice,a_desc);
		m_handleMap[a_desc.name] = _handle;
		return _handle;
	}

	const ID3D12PipelineState* GraphicsPSOManager::Get(const Resource::Handle<PipelineState>& a_handle) const
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			return m_pipelineData[a_handle.idx].Get();
		}
		return nullptr;
	}

	ID3D12PipelineState* GraphicsPSOManager::Ref(const Resource::Handle<PipelineState>& a_handle)
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			return m_pipelineData[a_handle.idx].Ref();
		}
		return nullptr;
	}

}