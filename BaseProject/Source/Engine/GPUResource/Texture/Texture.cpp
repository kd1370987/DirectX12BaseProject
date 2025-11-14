#include "Texture.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

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
	HRESULT _hr = S_FALSE;
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
		printf("テクスチャファイルの読み込みに失敗 : %ls\n", _path.c_str());
		return false;
	}

	//----------------------------------------
	// 変数準備
	//----------------------------------------
	//auto* _device = RenderingEngine::Instance().GetDevice();
	//auto* _cmdList = RenderingEngine::Instance().GetCommandList();

	////----------------------------------------
	//// GPUテクスチャリソース（default heap）
	////----------------------------------------
	//ComPtr<ID3D12Resource> _tex;
	//_hr = DirectX::CreateTexture(
	//	_device,
	//	_meta,
	//	&_tex
	//);
	//if (FAILED(_hr))
	//{
	//	return false;
	//}

	////----------------------------------------
	//// アップロードヒープの作成
	////----------------------------------------
	//std::vector<D3D12_SUBRESOURCE_DATA> _subresources;
	//_hr = DirectX::PrepareUpload(
	//	_device,
	//	_sImg.GetImages(),
	//	_sImg.GetImageCount(),
	//	_meta,
	//	_subresources
	//);
	//if (FAILED(_hr))
	//{
	//	return false;
	//}

	//const UINT64 _uploadSize = GetRequiredIntermediateSize(
	//	_tex.Get(),
	//	0,
	//	static_cast<UINT>(_subresources.size())
	//);

	// 仕様書設定
	auto _img = _sImg.GetImage(0, 0, 0);
	D3D12_HEAP_PROPERTIES _heapProp = {};
	_heapProp.Type					= D3D12_HEAP_TYPE_CUSTOM;
	_heapProp.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	_heapProp.MemoryPoolPreference	= D3D12_MEMORY_POOL_L0;
	D3D12_RESOURCE_DESC _desc = {};
	_desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(_meta.dimension);
	_desc.Format = _meta.format;
	_desc.Width = static_cast<UINT64>(_meta.width);
	_desc.Height = static_cast<UINT>(_meta.height);
	_desc.DepthOrArraySize = static_cast<UINT16>(_meta.arraySize);
	_desc.MipLevels = static_cast<UINT16>(_meta.mipLevels);
	_desc.SampleDesc.Count = 1;

	auto img = _sImg.GetImage(0, 0, 0);
	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
		_meta.format,
		_meta.width,
		_meta.height,
		static_cast<UINT16>(_meta.arraySize),
		static_cast<UINT16>(_meta.mipLevels));

	// auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_uploadSize);

	//----------------------------------------
	// リソースの生成
	//----------------------------------------
	_hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_textureResource.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		// テクスチャリソースの生成失敗
		printf("テクスチャリソースの生成に失敗 : %ls\n", _path.c_str());
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
		printf("テクスチャデータのコピーに失敗 : %ls\n", _path.c_str());
		return false;
	}

	//----------------------------------------
	// 最終状態
	//----------------------------------------
	//auto _resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
	//							_tex.Get(),
	//							D3D12_RESOURCE_STATE_COPY_DEST,
	//							D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	//						);

	//_cmdList->ResourceBarrier(
	//	1,
	//	&_resourceBarrier
	//);

	// メタデータの保存
	//m_textureResource = _tex;
	width = _meta.width;
	height = _meta.height;
	mipLevels = _meta.mipLevels;
	format = _meta.format;
	return true;
}
