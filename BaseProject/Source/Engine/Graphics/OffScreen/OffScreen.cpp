#include "OffScreen.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/ShaderManager/ShaderManager.h"

bool OffScreen::CreatePostProcessResource(ID3D12Resource& a_backBuffer)
{
	auto* _pDevice = RenderingEngine::Instance().GetDevice();
	auto&  _bBuff = a_backBuffer;
	auto _resDesc = _bBuff.GetDesc();

	// レンダーターゲットの作成
	m_offScreenRT.SetResourceDesc(_resDesc);
	m_offScreenRT.Create(_pDevice);

	// レンダーターゲットビューの作成
	D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
	_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	_rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_rtvHandle = DescriptorHeapManager::Instance().RegisterRTV(m_offScreenRT.Ref(), &_rtvDesc);

	// SRV作成
	D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
	_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	_srvDesc.Format = _rtvDesc.Format;
	_srvDesc.Texture2D.MipLevels = 1;
	_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_srvRange = DescriptorHeapManager::Instance().AllocateSRVRange({ {m_offScreenRT.Ref(),&_srvDesc}});

	
	return true;
}

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

bool OffScreen::CreateScreenPipeline(
	ShaderManager* a_pShaderManager,
	RootSignatureManager* a_pRootSigManager, 
	GraphicsPSOManager* a_pPSOManager
)
{
	// ルートシグネチャ登録
	m_rootSigID = a_pRootSigManager->Register(
		"QuadRendering",
		{
			{RootParameterType::DescriptorTable,{RangeType::SRV}}
		}
	);
	D3D12_INPUT_ELEMENT_DESC _layout[2] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
	D3D12_INPUT_LAYOUT_DESC _desc = {
		.pInputElementDescs = _layout,
		.NumElements = 2
	};
	

	m_vsID = a_pShaderManager->Register(
		{ "x64/Debug/QuadRenderingVS.cso",ShaderStage::Vertex,&_desc }
	);
	m_psID = a_pShaderManager->Register(
		{ "x64/Debug/QuadRenderingPS.cso",ShaderStage::Pixel }
	);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC _gpsDesc = {};
	_gpsDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	_gpsDesc.DepthStencilState.DepthEnable = false;
	_gpsDesc.DepthStencilState.StencilEnable = false;
	_gpsDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	_gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	_gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	_gpsDesc.NumRenderTargets = 1;
	_gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	_gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	_gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	_gpsDesc.SampleDesc.Count = 1;
	_gpsDesc.SampleDesc.Quality = 0;
	_gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	_gpsDesc.pRootSignature = a_pRootSigManager->NGet(m_rootSigID);
	_gpsDesc.VS = a_pShaderManager->NGet(m_vsID)->byteCode;
	_gpsDesc.InputLayout = a_pShaderManager->NGet(m_vsID)->vsInputLayout;
	_gpsDesc.PS = a_pShaderManager->NGet(m_psID)->byteCode;

	m_graphicPSOID = a_pPSOManager->Register("QuadRenderingPipeline",_gpsDesc);

	return true;
}

void OffScreen::SetRenderTarget(
	ID3D12GraphicsCommandList* a_pCmdList, D3D12_CPU_DESCRIPTOR_HANDLE& a_depthHandle
)
{
	auto _rtvHeapPointer = DescriptorHeapManager::Instance().GetRTVCPUHandle(m_rtvHandle);

	a_pCmdList->OMSetRenderTargets(1, &_rtvHeapPointer, false, &a_depthHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE OffScreen::GetRTVHandle()
{
	return DescriptorHeapManager::Instance().GetRTVCPUHandle(m_rtvHandle);
}
