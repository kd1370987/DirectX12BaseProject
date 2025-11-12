#include "Texture2D.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

// 読み込んだテクスチャを返す
Texture2D* Texture2D::Get(std::string a_path)
{
	auto _wpath = StringUtility::ToWideString(a_path);
	return Get(_wpath);
}
Texture2D* Texture2D::Get(std::wstring a_path)
{
	auto _tex = new Texture2D(a_path);
	if (!_tex->IsValid())
	{
		// 読み込み失敗時には白単色テクスチャを返す
		return GetWhite();
	}
	return _tex;
}
Texture2D* Texture2D::GetWhite()
{
	// 白テクスチャを返す
	ID3D12Resource* _pBuff = GetDefaultResource(4, 4);		// 4x4の白テクスチャを生成

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
		return nullptr;
	}

	return new Texture2D(_pBuff);
}

// テクスチャが正常に読み込まれているかどうか
bool Texture2D::IsValid() const
{
	return m_isValid;
}

// テクスチャリソースを取得
ID3D12Resource* Texture2D::Resource()
{
	return m_pResource.Get();
}

// SRVの設定を返す
D3D12_SHADER_RESOURCE_VIEW_DESC Texture2D::ViewDesc()
{
	// SRV設定の作成
	D3D12_SHADER_RESOURCE_VIEW_DESC _desc = {};
	_desc.Format = m_pResource->GetDesc().Format;									// フォーマット
	_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;		// 標準設定
	_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;							// 2Dテクスチャ
	_desc.Texture2D.MipLevels = 1;													// みっぷマップは使用なし

	return _desc;
}

// コンストラクタ
Texture2D::Texture2D(std::string a_path)
{
	m_isValid = Load(a_path);
}
Texture2D::Texture2D(std::wstring a_path)
{
	m_isValid = Load(a_path);
}
Texture2D::Texture2D(ID3D12Resource* a_buffer)
{
	m_pResource = a_buffer;
	m_isValid = (m_pResource != nullptr);
}

// テクスチャ読み込み
bool Texture2D::Load(std::string& a_path)
{
	auto _wpath = StringUtility::ToWideString(a_path);
	return Load(_wpath);
}
bool Texture2D::Load(std::wstring& a_path)
{
	// WICテクスチャローダーで読み込み
	DirectX::TexMetadata _meta = {};
	DirectX::ScratchImage _scratch = {};
	auto _ext = FileUtility::GetFilePathExtension(a_path);

	HRESULT _hr = S_FALSE;
	if (_ext == L"png")				// pngの時はWICFileを使う
	{
		DirectX::LoadFromWICFile(
			a_path.c_str(),
			DirectX::WIC_FLAGS_NONE,
			&_meta,
			_scratch
		);
	}
	else if (_ext == L"tga")		// tagの時はTGAFileを使う
	{
		_hr = DirectX::LoadFromTGAFile(
			a_path.c_str(),
			&_meta,
			_scratch
		);
	}
	if (FAILED(_hr))
	{
		// テクスチャリソースの生成失敗
		return false;
	}

	auto _img = _scratch.GetImage(0, 0, 0);
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto _desc = CD3DX12_RESOURCE_DESC::Tex2D(
		_meta.format,
		_meta.width,
		_meta.height,
		static_cast<UINT16>(_meta.arraySize),
		static_cast<UINT16>(_meta.mipLevels)
	);

	// リソースを生成
	_hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_pResource.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		// テクスチャリソースの生成失敗
		return false;
	}

	_hr = m_pResource->WriteToSubresource(
		0,
		nullptr,								// 全領域に書き込む
		_img->pixels,							// 元データアドレス
		static_cast<UINT>(_img->rowPitch),		// 1ラインのバイトサイズ
		static_cast<UINT>(_img->slicePitch)		// 全データのバイトサイズ
	);
	if (FAILED(_hr))
	{
		return false;
	}

	return true;
}

// デフォルトリソースの取得
ID3D12Resource* Texture2D::GetDefaultResource(size_t a_width, size_t a_height)
{
	// リソース記述子の作成
	auto _resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, a_width, a_height);
	// ヒーププロパティの作成
	auto _texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	ID3D12Resource* _pBuff = nullptr;
	auto _result = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&_texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&_resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&_pBuff)
	);
	if (FAILED(_result))
	{
		assert(SUCCEEDED(_result));
		return nullptr;
	}
	return _pBuff;

}
