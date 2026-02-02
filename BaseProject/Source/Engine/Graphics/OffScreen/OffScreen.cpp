#include "OffScreen.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/ShaderManager/ShaderManager.h"

bool OffScreen::CreateScreenVertex()
{
	struct ScreenVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	ScreenVertex _sv[4] = {
		{{-1,-1,0.1},{0,1}},
		{{-1, 1,0.1},{0,0}},
		{{ 1,-1,0.1},{1,1}},
		{{ 1, 1,0.1},{1,0}}
	};

	auto _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto _resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(_sv));
	auto _hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&_resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_screenVB.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0);
		return false;
	}

	ScreenVertex* _mapped = nullptr;
	m_screenVB->Map(0,nullptr,(void**)&_mapped);
	std::copy(std::begin(_sv),std::end(_sv),_mapped);
	m_screenVB->Unmap(0,nullptr);

	m_screenVBView.BufferLocation = m_screenVB->GetGPUVirtualAddress();
	m_screenVBView.SizeInBytes = sizeof(_sv);
	m_screenVBView.StrideInBytes = sizeof(ScreenVertex);

	return true;
}