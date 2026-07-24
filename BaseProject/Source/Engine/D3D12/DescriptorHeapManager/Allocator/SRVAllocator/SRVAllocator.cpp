#include "SRVAllocator.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
namespace Engine::D3D12
{
	bool Engine::D3D12::SRVAllocator::Create(
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>* a_pHeap,
		UINT a_srvStart,
		UINT a_srvCount
	)
	{
		// ヒープ参照
		m_pHeap = a_pHeap;

		// インデックス行列作成
		m_genVec.resize(a_srvCount);
		for (UINT _idx = 0; _idx < a_srvCount; ++_idx)
		{
			m_indexQueue.push(_idx);
			m_genVec[_idx] = 0;
		}

		// SRV開始位置
		m_srvStartIndex = a_srvStart;
		m_srvRangeList.Init(a_srvCount);

		return true;
	}

	std::vector<Engine::Resource::Handle<SRV>> Engine::D3D12::SRVAllocator::Allocate(
		std::vector<SRVViewInit> a_resourceVec
	)
	{
		// インデックス上限チェック
		if (m_indexQueue.empty())
		{
			assert(0 && "SRVの使用上限に達しました");
		}

		// リザルト用意
		std::vector<Engine::Resource::Handle<SRV>> _result = {};
		_result.resize(a_resourceVec.size());

		// インデックス取得
		auto _range = m_srvRangeList.Allocate(a_resourceVec.size());
		//ImGuiContex::Instance().AddLog("Range : %d , %d\n", _range.startIndex,_range.rangeSize);
		for (UINT _i = 0; _i < _result.size(); ++_i)
		{
			// リソースごとのハンドル取得
			auto _handle = m_pHeap->GetCPU(m_srvStartIndex + _range.startIndex + _i);

			// ビューの仕様書作成
			D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
			if (a_resourceVec[_i].pDesc)
			{
				_srvDesc = *a_resourceVec[_i].pDesc;
			}
			else
			{
				_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				_srvDesc.Texture2D.MipLevels = a_resourceVec[_i].pResource->GetDesc().MipLevels;
			}

			// SRV生成
			D3D12Wrapper::Instance().GetDevice()->CreateShaderResourceView(
				a_resourceVec[_i].pResource,
				&_srvDesc,
				_handle
			);

			// リザルト作成
			_result[_i].idx = _range.startIndex + _i;
			_result[_i].gen = m_genVec[_result[_i].idx];

			//ImGuiContex::Instance().AddLog("Allocate : %d\n" ,_result[_i].idx);
		}

		return _result;
	}

	

	void Engine::D3D12::SRVAllocator::Remove(Engine::Resource::Handle<SRV> a_handle)
	{
		if (Check(a_handle))
		{
			m_indexQueue.push(a_handle.idx);
			m_genVec[a_handle.idx]++;
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Engine::D3D12::SRVAllocator::GetCPU(
		const Engine::Resource::Handle<SRV>& a_handle
	) const
	{
		if (Check(a_handle))
		{
			return m_pHeap->GetCPU(m_srvStartIndex + static_cast<UINT>(a_handle.idx));
		}
		assert(0 && "SRVの世代が違います");
		return D3D12_CPU_DESCRIPTOR_HANDLE();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Engine::D3D12::SRVAllocator::GetGPU(const Engine::Resource::Handle<SRV>& a_handle) const
	{
		if (Check(a_handle))
		{
			return m_pHeap->GetGPU(m_srvStartIndex + static_cast<UINT>(a_handle.idx));
		}
		assert(0 && "SRVの世代が違います");
		return D3D12_GPU_DESCRIPTOR_HANDLE();
	}

	bool Engine::D3D12::SRVAllocator::Check(const Engine::Resource::Handle<SRV>& a_handle) const
	{

		if (m_genVec[a_handle.idx] == a_handle.gen)
		{
			return true;
		}
		return false;
	}
}