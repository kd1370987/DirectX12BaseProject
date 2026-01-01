#include "Texture.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/GPUResource/DescriptorHeap/DescriptorHeap.h"

bool Texture::Load(const std::string& a_path)
{
	//----------------------------------------
	// データ準備
	//----------------------------------------
	std::wstring _path = StringUtility::ToWideString(a_path);
	DirectX::TexMetadata _meta = {};
	DirectX::ScratchImage _sImg = {};
	std::wstring _ext = FileUtility::GetFilePathExtension(_path);

	//----------------------------------------
	// 拡張子によって読み込み方法を変える
	//----------------------------------------
	HRESULT _hr = E_FAIL;
	if (_ext == L"png" || _ext == L"jpg" || _ext == L"jpeg")
	{
		_hr = DirectX::LoadFromWICFile(
			_path.c_str(),
			DirectX::WIC_FLAGS_NONE,
			&_meta,
			_sImg
		);
	}
	else if (_ext == L"tga")
	{
		_hr = DirectX::LoadFromTGAFile(
			_path.c_str(),
			&_meta,
			_sImg
		);
	}
	if (FAILED(_hr))
	{
		// ファイルの読み込み失敗
		assert(0 && "テクスチャファイルの読み込みに失敗 : %ls\n", _path.c_str());
		return false;
	}

	// 仕様書設定
	auto _img = _sImg.GetImage(0, 0, 0);
	D3D12_HEAP_PROPERTIES _heapProp = {};
	_heapProp.Type					= D3D12_HEAP_TYPE_CUSTOM;
	_heapProp.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	_heapProp.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	_heapProp.CreationNodeMask		= 0;
	_heapProp.VisibleNodeMask		= 0;
	D3D12_RESOURCE_DESC _desc = {};
	_desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(_meta.dimension);
	_desc.Format = _meta.format;
	_desc.Width = static_cast<UINT64>(_meta.width);
	_desc.Height = static_cast<UINT>(_meta.height);
	_desc.DepthOrArraySize = static_cast<UINT16>(_meta.arraySize);
	_desc.MipLevels = static_cast<UINT16>(_meta.mipLevels);
	_desc.SampleDesc.Count = 1;
	_desc.SampleDesc.Quality = 0;
	_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//----------------------------------------
	// リソースの生成
	//----------------------------------------
	_hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_textureResource.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		// テクスチャリソースの生成失敗
		assert(0 && "テクスチャリソースの生成に失敗 : %ls\n", _path.c_str());
		return false;
	}

	//----------------------------------------
	// テクスチャデータのコピー
	//----------------------------------------
	_hr = m_textureResource->WriteToSubresource(
		0,
		nullptr,									// 全領域へコピー
		_img->pixels,								// 元データアドレス
		static_cast<UINT>(_img->rowPitch),			// １ラインサイズ
		static_cast<UINT>(_img->slicePitch)			// 全サイズ
	);
	if (FAILED(_hr))
	{
		// テクスチャデータのコピー失敗
		assert(0 && "テクスチャデータのコピーに失敗 : %ls\n", _path.c_str());
		return false;
	}

	//----------------------------------------
	// ディスクリプタヒープに登録
	//----------------------------------------
	auto _handle = DescriptorHeapManager::Instance().RegisterSRV(m_textureResource.Get());
	m_cpuSrvHandle = _handle.handleCPU;
	m_gpuSrvHandle = _handle.handleGPU;

	//----------------------------------------
	// 最終状態
	//----------------------------------------
	// メタデータの保存
	width = _meta.width;
	height = _meta.height;
	mipLevels = _meta.mipLevels;
	format = _meta.format;

	return true;
}

bool Texture::WhiteTexture()
{

	// 白テクスチャ生成
	m_textureResource.Reset();
	auto _pBuff = GetDefaultResource(4, 4);
	if (_pBuff == nullptr)
	{
		return false;
	}
	std::vector<unsigned char> _data(4 * 4 * 4);			// RGBA32ビットデータ
	std::fill(_data.begin(), _data.end(), 0xff);			// 全てのピクセルを白にする

	// テクスチャデータを書き込む
	auto _hr = _pBuff->WriteToSubresource(
		0,
		nullptr,
		_data.data(),
		4 * 4,			// 1ラインのバイトサイズ
		_data.size()			// 全データのバイトサイズ
	);
	if (FAILED(_hr))
	{
		assert(0 && "白テクスチャデータのコピーに失敗\n");
		return false;
	}

	m_textureResource.Attach(_pBuff);

	return true;
}

ID3D12Resource* Texture::GetDefaultResource(size_t a_width, size_t a_height)
{
	auto _resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		a_width,
		a_height,
		1,			// 配列サイズ
		1			// ミップレベル数
	);
	auto _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	// リソースを生成
	ID3D12Resource* _pResource = nullptr;
	HRESULT _hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&_resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&_pResource)
	);
	if (FAILED(_hr))
	{
		// テクスチャリソースの生成失敗
		assert(0 && "デフォルトテクスチャリソースの生成に失敗\n");
		return nullptr;
	}
	return _pResource;
}
