#include "TextureImporter.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "../../../Data/Texture/Texture.h"

void CopyTexRegion(ID3D12Resource* a_pResource, const Engine::Resource::UploadBuffer& a_uploadBuffer)
{
	// コマンドリストを取得
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// コマンドキューリセット
	D3D12Wrapper::Instance().CommandQueueReset();

	for (UINT _i = 0; _i < a_uploadBuffer.subresourceCount; ++_i)
	{
		// 参照元
		D3D12_TEXTURE_COPY_LOCATION _src = {};
		_src.pResource = a_uploadBuffer.pResource;
		_src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		_src.PlacedFootprint = a_uploadBuffer.layoutVec[_i];

		// コピー先
		D3D12_TEXTURE_COPY_LOCATION _dst = {};
		_dst.pResource = a_pResource;
		_dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		_dst.SubresourceIndex = _i;


		// GPUへこぴー
		_pCmdList->CopyTextureRegion(
			&_dst,			// コピー先
			0,					// Xオフセット
			0,					// Yオフセット
			0,					// Zオフセット
			&_src,			// コピー元
			nullptr				// コピー元ボックス（全領域）
		);
	}

	// コピー操作はGPUに対する命令なので、実行するにはコマンドリストをクローズして
	// コマンドキューに積む必要がある
	D3D12_RESOURCE_BARRIER _barrier = {};
	_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	_barrier.Transition.pResource = a_pResource;
	_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	_pCmdList->ResourceBarrier(1, &_barrier);
	_pCmdList->Close();

	// コマンドキューに積む
	ID3D12CommandList* _ppCommandLists[] = { _pCmdList };
	auto* _cmdQueue = D3D12Wrapper::Instance().GetCommandQueue();
	_cmdQueue->ExecuteCommandLists(std::size(_ppCommandLists), _ppCommandLists);

	// 終了待ち
	D3D12Wrapper::Instance().SignalRenderFence();
	D3D12Wrapper::Instance().WaitRender();
}

Engine::Resource::UploadBuffer CreateUploadHeap(const D3D12_RESOURCE_DESC& a_texDesc, const DirectX::TexMetadata& a_meta)
{
	Engine::Resource::UploadBuffer _uploadBuffer = {};

	// サブリソース総数
	_uploadBuffer.subresourceCount = a_meta.mipLevels * a_meta.arraySize;

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> _layoutVec(_uploadBuffer.subresourceCount);
	std::vector<UINT> _numRowVec(_uploadBuffer.subresourceCount);
	std::vector<UINT64> _rowSizeVec(_uploadBuffer.subresourceCount);

	// サイズ計算
	UINT64 _uploadSize = 0;
	D3D12Wrapper::Instance().GetDevice()->GetCopyableFootprints(
		&a_texDesc,
		0,
		_uploadBuffer.subresourceCount,
		0,
		_layoutVec.data(),
		_numRowVec.data(),
		_rowSizeVec.data(),
		&_uploadSize
	);

	_uploadBuffer.layoutVec = _layoutVec;
	_uploadBuffer.numRowVec = _numRowVec;
	_uploadBuffer.rowSizeVec = _rowSizeVec;

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
	HRESULT _hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,					// 特に指定はなし
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,		// CPUから書き込み可能、GPUからは読み取り可能
		nullptr,
		IID_PPV_ARGS(&_uploadBuffer.pResource)
	);
	if (FAILED(_hr))
	{
		assert(0 && "リソース生成に失敗中間バッファ");
		return Engine::Resource::UploadBuffer();
	}

	return _uploadBuffer;
}

bool BuildFromScratchiImage(
	ComPtr<ID3D12Resource>& a_cpRes,
	DirectX::TexMetadata& a_meta,
	DirectX::ScratchImage& a_sImg
)
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
		IID_PPV_ARGS(a_cpRes.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		// テクスチャリソースの生成失敗
		assert(0 && "テクスチャリソースの生成に失敗");
		return false;
	}

	// アップロードヒープ作成
	Engine::Resource::UploadBuffer _uploadBuffer = CreateUploadHeap(_texDesc, a_meta);

	// 中間バッファへデータコピー
	void* _pData = nullptr;
	_hr = _uploadBuffer.pResource->Map(
		0,
		nullptr,
		&_pData
	);

	// サブリソースごとにコピー
	for (UINT _i = 0; _i < _uploadBuffer.subresourceCount; ++_i)
	{
		const DirectX::Image* _img = a_sImg.GetImage(_i % a_meta.mipLevels, _i / a_meta.mipLevels, 0);

		uint8_t* _dst = static_cast<uint8_t*>(_pData) + _uploadBuffer.layoutVec[_i].Offset;
		const uint8_t* _src = _img->pixels;

		for (UINT _row = 0; _row < _uploadBuffer.numRowVec[_i]; ++_row)
		{
			std::memcpy(_dst, _src, _uploadBuffer.rowSizeVec[_i]);
			_dst += _uploadBuffer.layoutVec[_i].Footprint.RowPitch;
			_src += _img->rowPitch;
		}
	}
	// 操作の終了
	_uploadBuffer.pResource->Unmap(0, nullptr);

	// GPUにコピー
	CopyTexRegion(a_cpRes.Get(), _uploadBuffer);

	return true;
}

bool ImportFromPath(DirectX::TexMetadata& a_metaData, DirectX::ScratchImage& a_img, std::wstring& a_path)
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
	return true;
}

ComPtr<ID3D12Resource> Engine::Resource::ImportTexture(
	const std::string& a_filePath, 
	D3D12_RESOURCE_DESC* a_desc
)
{
	//----------------------------------------
	// データ準備
	//----------------------------------------
	ComPtr<ID3D12Resource> _cpRes = nullptr;
	std::wstring _path = StringUtility::ToWideString(a_filePath);
	DirectX::TexMetadata _meta = {};
	DirectX::ScratchImage _sImg = {};

	// テクスチャ読み込み
	if (!ImportFromPath(_meta, _sImg, _path))
	{
		return _cpRes;
	}

	if (a_desc)
	{
		_meta.format = a_desc->Format;
		_meta.width = static_cast<size_t>(a_desc->Width);
		_meta.height = static_cast<size_t>(a_desc->Height);
		_meta.arraySize = static_cast<size_t>(a_desc->DepthOrArraySize);
		_meta.mipLevels = static_cast<size_t>(a_desc->MipLevels);
	}

	// みっぷマップ計算
	DirectX::ScratchImage _mipChain;
	HRESULT _hr = DirectX::GenerateMipMaps(
		_sImg.GetImages(),
		_sImg.GetImageCount(),
		_sImg.GetMetadata(),
		DirectX::TEX_FILTER_DEFAULT,
		0,		// 0 = フルミップ
		_mipChain
	);
	if (FAILED(_hr))
	{
		return _cpRes;
	}

	_sImg = std::move(_mipChain);
	_meta = _sImg.GetMetadata();

	// テクスチャを構築
	BuildFromScratchiImage(_cpRes, _meta, _sImg);

	return _cpRes;
}

ComPtr<ID3D12Resource> Engine::Resource::DefaultTexture(DirectX::XMFLOAT4 a_color)
{
	ComPtr<ID3D12Resource> _cpRes = nullptr;

	DirectX::TexMetadata _meta = {};
	DirectX::ScratchImage _sImg = {};
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
			_row[_x * 4 + 0] = a_color.x;// R
			_row[_x * 4 + 1] = a_color.y;// G
			_row[_x * 4 + 2] = a_color.z;// B
			_row[_x * 4 + 3] = a_color.w;// A
		}
	}
	_meta = _sImg.GetMetadata();
	// テクスチャを構築
	BuildFromScratchiImage(_cpRes, _meta, _sImg);
	return _cpRes;
}

ComPtr<ID3D12Resource> Engine::Resource::WhiteTexture()
{
	return DefaultTexture({
		255,
		255,
		255,
		255
	});
}

ComPtr<ID3D12Resource> Engine::Resource::BlackTexture()
{
	return DefaultTexture({
		0,
		0,
		0,
		255
	});
}

ComPtr<ID3D12Resource> Engine::Resource::NormalWhiteTexture()
{
	return DefaultTexture({
		128,
		128,
		255,
		255
	});
}

ComPtr<ID3D12Resource> Engine::Resource::ORMTexture()
{
	return DefaultTexture({
		0,
		255,
		255,
		255
	});
}
