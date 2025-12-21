#pragma once

//class DescriptorHeap;
struct DescriptorHandle;

class CBVHeap;
class SRVHeap;
class DSVHeap;
class RTVHeap;

class DescriptorHeapManager
{
public:

	void Init();

	/// <summary>
	/// 定数バッファビュー登録
	/// </summary>
	/// <param name="a_resource">登録するリソース</param>
	/// <returns>登録したハンドル</returns>
	DescriptorHandle RegisterCBV(ID3D12Resource* a_resource);

	/// <summary>
	/// CBVヒープクラスのポインタを返す
	/// </summary>
	/// <returns>CBVクラスポインタ</returns>
	std::shared_ptr<CBVHeap> GetDescriptorCBV() const { return m_spCBVHeap; }

	/// <summary>
	/// シェーダーリソースビュー作成
	/// </summary>
	/// <param name="a_resource">登録するリソース</param>
	/// <returns>登録した場所を返す</returns>
	DescriptorHandle RegisterSRV(ID3D12Resource* a_resource);

	/// <summary>
	/// SRVヒープクラスのポインタを返す
	/// </summary>
	/// <returns>SRVヒープクラスポインタ</returns>
	std::shared_ptr<SRVHeap> GetDescriptorSRV() const { return m_spSRVHeap; }

	/// <summary>
	/// デプスステンシルビュー登録
	/// </summary>
	/// <param name="a_resource">登録するリソース</param>
	/// <returns>登録した場所を返す</returns>
	DescriptorHandle RegisterDSV(ID3D12Resource* a_resource);

	/// <summary>
	/// DSVヒープクラスのポインタを返す
	/// </summary>
	/// <returns>DSVヒープクラスポインタ</returns>
	std::shared_ptr<DSVHeap> GetDescriptorDSV() const { return m_spDSVHeap; }

	/// <summary>
	/// レンダーターゲットビュー登録
	/// </summary>
	/// <param name="a_resource">登録するリソース</param>
	/// <returns>登録した場所を返す</returns>
	DescriptorHandle RegisterRTV(ID3D12Resource* a_resource);

	/// <summary>
	/// RTVヒープクラスのポインタを返す
	/// </summary>
	/// <returns>RTVヒープクラスポインタ</returns>
	std::shared_ptr<RTVHeap> GetDescriptorRTV() const { return m_spRTVHeap; }



private:

	// CBVヒープ
	std::shared_ptr<CBVHeap> m_spCBVHeap = nullptr;

	// SRVヒープ
	std::shared_ptr<SRVHeap> m_spSRVHeap = nullptr;

	// DSVヒープ
	std::shared_ptr<DSVHeap> m_spDSVHeap = nullptr;

	// RTVヒープ
	std::shared_ptr<RTVHeap> m_spRTVHeap = nullptr;

// シングルトン
private:
	DescriptorHeapManager() = default;
	~DescriptorHeapManager() = default;

	// コピー禁止
	DescriptorHeapManager(const DescriptorHeapManager&) = delete;
	void operator=(const DescriptorHeapManager&) = delete;

public:

	static DescriptorHeapManager& Instance()
	{
		static DescriptorHeapManager instance;
		return instance;
	}
};