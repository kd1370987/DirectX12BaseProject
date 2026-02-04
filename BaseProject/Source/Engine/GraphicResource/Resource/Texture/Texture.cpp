#include "Texture.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"

bool Texture::Load(const std::string& a_path)
{
	//----------------------------------------
	// データ準備
	//----------------------------------------
	std::wstring _path = StringUtility::ToWideString(a_path);
	DirectX::TexMetadata _meta = {};
	DirectX::ScratchImage _sImg = {};

	// テクスチャ読み込み
	if (!LoadFromPath(_meta, _sImg, _path))
	{
		_sImg = {};
		_sImg.Initialize2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			4, 4,
			1, 1
		);

		auto* _image = _sImg.GetImage(0, 0, 0);

		for (size_t _h = 0; _h < _image->height; ++_h)
		{
			uint8_t* _row = _image->pixels + _h * _image->rowPitch;
			for (size_t _x = 0; _x < _image->width; ++_x)
			{
				_row[_x * 4 + 0] = 128;// R
				_row[_x * 4 + 1] = 128;// G
				_row[_x * 4 + 2] = 255;// B
				_row[_x * 4 + 3] = 255;// A
			}
		}

		_meta = _sImg.GetMetadata();
		return BuildFromScratchiImage(_meta, _sImg);
	}

	return BuildFromScratchiImage(_meta, _sImg);
}

bool Texture::NormalMapLoad(const std::string& a_path)
{
	//----------------------------------------
	// データ準備
	//----------------------------------------
	std::wstring _path = StringUtility::ToWideString(a_path);
	DirectX::TexMetadata _meta = {};
	DirectX::ScratchImage _sImg = {};

	// テクスチャ読み込み
	if (!LoadFromPath(_meta, _sImg, _path))
	{
		_sImg = {};
		_sImg.Initialize2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			4,4,
			1,1
		);

		auto* _image = _sImg.GetImage(0,0,0);

		for (size_t _h = 0; _h < _image->height; ++_h)
		{
			uint8_t* _row = _image->pixels + _h * _image->rowPitch;
			for (size_t _x = 0; _x < _image->width; ++_x)
			{
				_row[_x * 4 + 0] = 128;// R
				_row[_x * 4 + 1] = 128;// G
				_row[_x * 4 + 2] = 255;// B
				_row[_x * 4 + 3] = 255;// A
			}
		}

		_meta = _sImg.GetMetadata();
		return BuildFromScratchiImage(_meta,_sImg);
	}
	
	return BuildFromScratchiImage(_meta,_sImg);
}

bool Texture::WhiteTexture()
{
		DirectX::ScratchImage _sImg = {};
		_sImg.Initialize2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			4, 4,
			1, 1
		);

		auto* _image = _sImg.GetImage(0, 0, 0);

		for (size_t _h = 0; _h < _image->height; ++_h)
		{
			uint8_t* _row = _image->pixels + _h * _image->rowPitch;
			for (size_t _x = 0; _x < _image->width; ++_x)
			{
				_row[_x * 4 + 0] = 128;// R
				_row[_x * 4 + 1] = 128;// G
				_row[_x * 4 + 2] = 255;// B
				_row[_x * 4 + 3] = 255;// A
			}
		}

		DirectX::TexMetadata _meta = _sImg.GetMetadata();
		return BuildFromScratchiImage(_meta, _sImg);
}
bool Texture::LoadFromPath(DirectX::TexMetadata& a_metaData, DirectX::ScratchImage& a_img, std::wstring& a_path)
{
	//----------------------------------------
	// 拡張子によって読み込み方法を変える
	//----------------------------------------
	HRESULT _hr = E_FAIL;
	std::wstring _ext = FileUtility::GetFilePathExtension(a_path);
	if (_ext == L"png" || _ext == L"jpg" || _ext == L"jpeg")
	{
		_hr = DirectX::LoadFromWICFile(
			a_path.c_str(),
			DirectX::WIC_FLAGS_NONE,
			&a_metaData,
			a_img
		);
	}
	else if (_ext == L"tga")
	{
		_hr = DirectX::LoadFromTGAFile(
			a_path.c_str(),
			&a_metaData,
			a_img
		);
	}
	if (FAILED(_hr))
	{
		return false;
	}

	/*DXGI_FORMAT desiredFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	DirectX::ScratchImage converted;
	HRESULT hr = DirectX::Convert(
		a_img.GetImages(),
		a_img.GetImageCount(),
		a_metaData,
		desiredFormat,
		DirectX::TEX_FILTER_DEFAULT,
		DirectX::TEX_THRESHOLD_DEFAULT,
		converted
	);

	if (FAILED(hr))
		return false;

	a_img = std::move(converted);
	a_metaData.format = desiredFormat;*/
	return true;
}

bool Texture::BuildFromScratchiImage(DirectX::TexMetadata& a_meta, DirectX::ScratchImage& a_sImg)
{
	HRESULT _hr = E_FAIL;
	// テクスチャのためのヒープ設定
	D3D12_HEAP_PROPERTIES _texHeapProp = {};
	_texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;					// GPU専用
	_texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	_texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	_texHeapProp.CreationNodeMask = 0;
	_texHeapProp.VisibleNodeMask = 0;
	// テクスチャリソース仕様書
	D3D12_RESOURCE_DESC _texDesc = {};
	_texDesc.Format = a_meta.format;
	_texDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(a_meta.dimension);
	_texDesc.Width = static_cast<UINT64>(a_meta.width);
	_texDesc.Height = static_cast<UINT>(a_meta.height);
	_texDesc.DepthOrArraySize = static_cast<UINT16>(a_meta.arraySize);
	_texDesc.MipLevels = static_cast<UINT16>(a_meta.mipLevels);
	_texDesc.SampleDesc.Count = 1;
	_texDesc.SampleDesc.Quality = 0;
	_texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	_texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	// リソース生成（テクスチャ）
	_hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&_texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_textureResource.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		// テクスチャリソースの生成失敗
		assert(0 && "テクスチャリソースの生成に失敗");
		return false;
	}

	// アップロードヒープ作成
	ID3D12Resource* _uploadBuffer = CreateUploadHeap(_texDesc);;

	auto _img = a_sImg.GetImage(0, 0, 0);


	D3D12_TEXTURE_COPY_LOCATION _srcLocation = {};
	_srcLocation.pResource = _uploadBuffer;								// コピー元リソース
	_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// 配置されたフットプリント
	_srcLocation.PlacedFootprint.Offset = 0;
	D3D12Wrapper::Instance().GetDevice()->GetCopyableFootprints(
		&_texDesc,
		0,
		1,
		0,
		&_srcLocation.PlacedFootprint,
		nullptr,
		nullptr,
		nullptr
	);

	D3D12_TEXTURE_COPY_LOCATION _dstLocation = {};
	_dstLocation.pResource = m_textureResource.Get();					// コピー先リソース
	_dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		// サブリソースインデックス	
	_dstLocation.SubresourceIndex = 0;

	// 中間バッファへデータコピー
	void* _pData = nullptr;
	_hr = _uploadBuffer->Map(
		0,
		nullptr,
		&_pData
	);
	uint8_t* _dst = static_cast<uint8_t*>(_pData);
	const uint8_t* _src = _img->pixels;

	for (UINT _y = 0; _y < _srcLocation.PlacedFootprint.Footprint.Height; ++_y)
	{
		std::memcpy(_dst, _src, _img->rowPitch);
		_src += _img->rowPitch;
		_dst += _srcLocation.PlacedFootprint.Footprint.RowPitch;
	};
	_uploadBuffer->Unmap(0, nullptr);

	// GPUにコピー
	CopyTexRegion(_dstLocation, _srcLocation);

	//----------------------------------------
	// 最終状態
	//----------------------------------------
	// メタデータの保存
	width = a_meta.width;
	height = a_meta.height;
	mipLevels = a_meta.mipLevels;
	format = a_meta.format;

	return true;
}

ID3D12Resource* Texture::CreateUploadHeap(const D3D12_RESOURCE_DESC& a_texDesc)
{
	// サイズ計算
	UINT64 _uploadSize = 0;
	D3D12Wrapper::Instance().GetDevice()->GetCopyableFootprints(
		&a_texDesc,
		0,
		1,
		0,
		nullptr,
		nullptr,
		nullptr,
		&_uploadSize
	);

	// 中間バッファの作成（Uploadヒープ）
	D3D12_HEAP_PROPERTIES _heapProp = {};
	_heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;							// Map可能にするためアップロードヒープ
	_heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;		// アップロード前提
	_heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	_heapProp.CreationNodeMask = 0;										// 単一アダプタのため0
	_heapProp.VisibleNodeMask = 0;
	// リソース仕様書
	D3D12_RESOURCE_DESC _desc = {};
	_desc.Format = DXGI_FORMAT_UNKNOWN;								// 単なるデータの塊として扱う
	_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;				// バッファリソースとして指定
	_desc.Width = _uploadSize;									// バッファ全体のサイズを指定
	_desc.Height = 1;											// バッファなので高さは1
	_desc.DepthOrArraySize = 1;									// バッファなので深度は1
	_desc.MipLevels = 1;										// バッファなのでミップレベルは1
	_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;				// バッファなので行優先
	_desc.Flags = D3D12_RESOURCE_FLAG_NONE;						// 特にフラグはなし
	_desc.SampleDesc.Count = 1;									// 通常テクスチャのためマルチサンプリングはなし
	_desc.SampleDesc.Quality = 0;

	// リソース生成（中間バッファ）
	ID3D12Resource* _uploadBuffer = nullptr;
	HRESULT _hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,					// 特に指定はなし
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,		// CPUから書き込み可能、GPUからは読み取り可能
		nullptr,
		IID_PPV_ARGS(&_uploadBuffer)
	);
	if (FAILED(_hr))
	{
		assert(0 && "リソース生成に失敗中間バッファ");
		return nullptr;
	}

	return _uploadBuffer;
}

void Texture::CopyTexRegion(D3D12_TEXTURE_COPY_LOCATION& a_dstLoca, D3D12_TEXTURE_COPY_LOCATION& a_srcLoca)
{
	// コマンドリストを取得
	auto _cmdList = D3D12Wrapper::Instance().GetCommandList();

	// コマンドキュー使用前にはリセット必須s
	D3D12Wrapper::Instance().CommandQueueReset();

	_cmdList->CopyTextureRegion(
		&a_dstLoca,			// コピー先
		0,					// Xオフセット
		0,					// Yオフセット
		0,					// Zオフセット
		&a_srcLoca,			// コピー元
		nullptr				// コピー元ボックス（全領域）
	);

	// コピー操作はGPUに対する命令なので、実行するにはコマンドリストをクローズして
	// コマンドキューに積む必要がある
	D3D12_RESOURCE_BARRIER _barrier = {};
	_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	_barrier.Transition.pResource = m_textureResource.Get();
	_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	_cmdList->ResourceBarrier(1, &_barrier);
	_cmdList->Close();

	// コマンドキューに積む
	ID3D12CommandList* _ppCommandLists[] = { _cmdList };
	auto* _cmdQueue = D3D12Wrapper::Instance().GetCommandQueue();
	_cmdQueue->ExecuteCommandLists(std::size(_ppCommandLists), _ppCommandLists);

	// 終了待ち
	D3D12Wrapper::Instance().SignalRenderFence();
	D3D12Wrapper::Instance().WaitRender();
}
