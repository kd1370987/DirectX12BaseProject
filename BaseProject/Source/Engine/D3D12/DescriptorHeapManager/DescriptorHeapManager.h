#pragma once

//class DescriptorHeap;
struct DescriptorHandle;

class CBV_SRV_UAVHeap;
class DSVHeap;
class RTVHeap;

class DescriptorHeapManager
{
public:

	/// <summary>
	/// 各ディスクリプタの生成
	/// </summary>
	void Init();

	//==========================================================================================
	// 
	// CBV_SRV_UAV
	// 
	//==========================================================================================
	/// <summary>
	/// シェーダーリソースビュー作成
	/// </summary>
	/// <param name="a_resource">登録するリソース</param>
	/// <returns>登録した場所を返す</returns>
	Storage::Range RegisterSRV(ID3D12Resource* a_resource);

	/// <summary>
	/// 一括でSRVを確保
	/// </summary>
	/// <param name="a_resource">登録するリソースの配列</param>
	/// <returns>登録した配列の先頭のハンドル</returns>
	Storage::Range AllocateSRVRange(std::vector<SRVViewInit> a_viewInitVec);

	/// <summary>
	/// SRVのCPUハンドルを取得
	/// </summary>
	/// <param name="a_range">レンジ</param>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(Storage::Range a_range);

	/// <summary>
	/// SRVのGPUハンドルを取得
	/// </summary>
	/// <param name="a_range">レンジ</param>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(Storage::Range a_range);

	/// <summary>
	/// ヒープの生ポインタ取得
	/// </summary>
	ID3D12DescriptorHeap* GetCBV_SRV_UAVHeap() const;

	/// <summary>
	/// ImGui用のSRVのCPUハンドルを取得
	/// </summary>
	/// <param name="a_range">レンジ</param>
	D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiCPUHandle();

	/// <summary>
	/// ImGui用のSRVのGPUハンドルを取得
	/// </summary>
	/// <param name="a_range">レンジ</param>
	D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiGPUHandle();

	//==========================================================================================
	// 
	// DSV
	// 
	//==========================================================================================
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

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDSV();

	//==========================================================================================
	// 
	// RTV
	// 
	//==========================================================================================
	/// <summary>
	/// レンダーターゲットビュー登録
	/// </summary>
	/// <param name="a_resource">登録するリソース</param>
	/// <returns>登録したインデックス</returns>
	RTVHandle RegisterRTV(ID3D12Resource* a_resource,D3D12_RENDER_TARGET_VIEW_DESC* a_pRtvDesc);

	/// <summary>
	/// RTVのCPUハンドルを取得
	/// </summary>
	/// <param name="a_index">インデックス</param>
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(RTVHandle a_handle);
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