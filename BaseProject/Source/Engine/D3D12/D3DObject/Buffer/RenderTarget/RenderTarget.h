#pragma once

class RenderTarget
{
public:

	RenderTarget() = default;
	~RenderTarget() = default;

	/// <summary>
	/// 作成
	/// </summary>
	/// <param name="a_pDevice">依存デバイス</param>
	/// <returns>作成成功 = ture</returns>
	bool Create(ID3D12Device* a_pDevice);

	/// <summary>
	/// 作成前に呼ぶことで作成時につかうリソース仕様書を決定することができる
	/// </summary>
	void SetResourceDesc(const D3D12_RESOURCE_DESC& a_desc);

	/// <summary>
	/// バッファリソースのポインタ取得
	/// </summary>
	ID3D12Resource* Ref()
	{
		return m_cpResource.Get();
	}

	/// <summary>
	/// バッファリソースのconstポインタ取得
	/// </summary>
	const ID3D12Resource* Get()
	{
		return m_cpResource.Get();
	}

private:

	// デバイス依存
	ID3D12Device* m_pDevice = nullptr;

	// バッファーリソース
	ComPtr<ID3D12Resource> m_cpResource = nullptr;
	void* m_pMappedPtr = nullptr;

	std::unique_ptr<D3D12_RESOURCE_DESC> m_upResourceDesc = nullptr;

	// コピー禁止
	RenderTarget(const RenderTarget&) = delete;
	void operator = (const RenderTarget&) = delete;

};