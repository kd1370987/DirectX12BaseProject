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
	// 最終状態
	//----------------------------------------
	// メタデータの保存
	width = _meta.width;
	height = _meta.height;
	mipLevels = _meta.mipLevels;
	format = _meta.format;

	return true;
}

bool Texture::NormalMapLoad(const std::string& a_path)
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

	// テクスチャのためのヒープ設定
	D3D12_HEAP_PROPERTIES _texHeapProp = {};
	_texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;					// GPU専用
	_texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	_texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	_texHeapProp.CreationNodeMask = 0;
	_texHeapProp.VisibleNodeMask = 0;
	// テクスチャリソース仕様書
	D3D12_RESOURCE_DESC _texDesc = {};
	_texDesc.Format = _meta.format;
	_texDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(_meta.dimension);
	_texDesc.Width = static_cast<UINT64>(_meta.width);
	_texDesc.Height = static_cast<UINT>(_meta.height);
	_texDesc.DepthOrArraySize = static_cast<UINT16>(_meta.arraySize);
	_texDesc.MipLevels = static_cast<UINT16>(_meta.mipLevels);
	_texDesc.SampleDesc.Count = 1;
	_texDesc.SampleDesc.Quality = 0;
	_texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	_texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	// リソース生成（テクスチャ）
	_hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
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
		assert(0 && "テクスチャリソースの生成に失敗 : %ls\n", _path.c_str());
		return false;
	}

	// サイズ計算
	UINT64 _uploadSize = 0;
	RenderingEngine::Instance().GetDevice()->GetCopyableFootprints(
		&_texDesc,
		0,
		1,
		0,
		nullptr,
		nullptr,
		nullptr,
		&_uploadSize
	);

	// 中間バッファの作成（Uploadヒープ）
	auto _img = _sImg.GetImage(0, 0, 0);
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
	_hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
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
		return false;
	}

	// コマンドリストを取得
	auto _cmdList = RenderingEngine::Instance().GetCommandList();

	// コマンドキュー使用前にはリセット必須s
	RenderingEngine::Instance().CommandQueueReset();

	D3D12_TEXTURE_COPY_LOCATION _srcLocation = {};
	_srcLocation.pResource = _uploadBuffer;								// コピー元リソース
	_srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// 配置されたフットプリント
	_srcLocation.PlacedFootprint.Offset = 0;
	RenderingEngine::Instance().GetDevice()->GetCopyableFootprints(
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
		std::memcpy(_dst,_src,_img->rowPitch);
		_src += _img->rowPitch;
		_dst += _srcLocation.PlacedFootprint.Footprint.RowPitch;
	};
	_uploadBuffer->Unmap(0, nullptr);

	// GPUへコピーコマンド発行
	{
		_cmdList->CopyTextureRegion(
			&_dstLocation,		// コピー先
			0,					// Xオフセット
			0,					// Yオフセット
			0,					// Zオフセット
			&_srcLocation,		// コピー元
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
		auto* _cmdQueue = RenderingEngine::Instance().GetCommandQueue();
		_cmdQueue->ExecuteCommandLists(std::size(_ppCommandLists), _ppCommandLists);

		// 終了待ち
		RenderingEngine::Instance().SignalRenderFence();
		RenderingEngine::Instance().WaitRender();
	}

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
