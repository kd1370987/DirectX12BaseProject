#include "OffScreen.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

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

bool OffScreen::CreateScreenPipeline()
{
	D3D12_DESCRIPTOR_RANGE _range[1] = {};

	_range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	_range[0].BaseShaderRegister = 0;
	_range[0].NumDescriptors = 1;

	D3D12_ROOT_PARAMETER _rp[1] = {};

	_rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	_rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	_rp[0].DescriptorTable.pDescriptorRanges = &_range[0];
	_rp[0].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_ROOT_SIGNATURE_DESC _rsDesc = {};
	_rsDesc.NumParameters = 1;
	_rsDesc.pParameters = _rp;

	D3D12_STATIC_SAMPLER_DESC _sampler = CD3DX12_STATIC_SAMPLER_DESC(0);
	_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	_rsDesc.pStaticSamplers = &_sampler;
	_rsDesc.NumStaticSamplers = 1;
	_rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> _rsBlob;
	ComPtr<ID3DBlob> _errBlob;

	auto _hr = D3D12SerializeRootSignature(
		&_rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		_rsBlob.ReleaseAndGetAddressOf(),
		_errBlob.ReleaseAndGetAddressOf()
	);
	if (FAILED(_hr))
	{
		assert(0 && "OffScreenのルートシグネチャのシリアライズに失敗");
		return false;
	}

	_hr = RenderingEngine::Instance().GetDevice()->CreateRootSignature(
		0,
		_rsBlob->GetBufferPointer(),
		_rsBlob->GetBufferSize(),
		IID_PPV_ARGS(m_screenRootSignature.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "OffScreenのルートしぐねちゃ生成に失敗");
		return false;
	}

	ComPtr<ID3DBlob> _vs;
	ComPtr<ID3DBlob> _ps;

	_hr = D3DCompileFromFile(
		L"Asset/Shader/QuadRenderingShader/QuadRenderingVS.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"vs",
		"vs_5_0",
		0,
		0,
		_vs.ReleaseAndGetAddressOf(),
		_errBlob.ReleaseAndGetAddressOf()
	);
	if (FAILED(_hr))
	{
		assert(0 && "頂点シェーダー読み込み失敗");
		return false;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC _gpsDesc = {};
	_gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_vs.Get());
	_gpsDesc.DepthStencilState.DepthEnable = false;	
	_gpsDesc.DepthStencilState.StencilEnable = false;

	D3D12_INPUT_ELEMENT_DESC _layout[2] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	_gpsDesc.InputLayout.NumElements = std::size(_layout);
	_gpsDesc.InputLayout.pInputElementDescs = _layout;
	_gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	_gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	_gpsDesc.NumRenderTargets = 1;
	_gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	_gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	_gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	_gpsDesc.SampleDesc.Count = 1;
	_gpsDesc.SampleDesc.Quality = 0;
	_gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	_gpsDesc.pRootSignature = m_screenRootSignature.Get();

	_hr = D3DCompileFromFile(
		L"Asset/Shader/QuadRenderingShader/QuadRenderingPS.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"ps",
		"ps_5_0",
		0,
		0,
		_ps.ReleaseAndGetAddressOf(),
		_errBlob.ReleaseAndGetAddressOf()
	);
	if (FAILED(_hr))
	{
		assert(0 && "ピクセルシェーダーの読み込み失敗");
		return false;
	}

	_gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_ps.Get());
	_hr = RenderingEngine::Instance().GetDevice()->CreateGraphicsPipelineState(
		&_gpsDesc,IID_PPV_ARGS(m_screenPipelineDefault.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "オフスクリーンのパイプラインステート生成失敗");
		return false;
	}

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
