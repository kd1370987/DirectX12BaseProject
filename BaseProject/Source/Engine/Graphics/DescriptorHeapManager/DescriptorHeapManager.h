#pragma once

//class DescriptorHeap;
struct DescriptorHandle;

class CBVHeap;
class SRVHeap;
class DSVHeap;
class RTVHeap;
#include "../../GPUResource/DescriptorHeap/CBV_SRV_UAVHeap/CBV_SRV_UAVHeap.h"


class DescriptorHeapManager
{
public:

	void Init();

	/// <summary>
	/// CBV_SRV_UAVヒープクラスのポインタを返す
	/// </summary>
	/// <returns>CBV_SRV_UAVクラスポインタ</returns>
	std::shared_ptr<CBV_SRV_UAVHeap> GetDescriptorCBV_SRV_UAV() const { return m_spCBV_SRV_UAVHeap; }

	/// <summary>
	/// 定数バッファを登録
	/// </summary>
	/// <param name="a_resource">リソース</param>
	/// <param name="a_size">サイズ</param>
	/// <returns>登録したハンドル</returns>
	DescriptorHandle RegisterCBV(
		ID3D12Resource* a_resource,
		size_t a_size,
		D3D12_CONSTANT_BUFFER_VIEW_DESC& a_cbvDesc
	);

	/// <summary>
	/// シェーダーリソースビュー作成
	/// </summary>
	/// <param name="a_resource">登録するリソース</param>
	/// <returns>登録した場所を返す</returns>
	DescriptorHandle RegisterSRV(ID3D12Resource* a_resource);

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

	// CBV_SRV_UAVヒープ
	std::shared_ptr<CBV_SRV_UAVHeap> m_spCBV_SRV_UAVHeap = nullptr;

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