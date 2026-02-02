#pragma once

//class DescriptorHeap;
struct DescriptorHandle;

class CBV_SRV_UAVHeap;

class DescriptorHeapManager
{
public:

	/// <summary>
	/// 各ディスクリプタの生成
	/// </summary>
	void Init();

	//==========================================================================================
	// 
	// CBV
	// 
	//==========================================================================================

	//==========================================================================================
	// 
	// SRV
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

	//==========================================================================================
	// 
	// UAV
	// 
	//==========================================================================================

	UAVHandle AllocateUAVRange(const std::vector<UAVViewInit>& a_viewInitVec);

	D3D12_CPU_DESCRIPTOR_HANDLE UAVCPUHandle(const UAVHandle& a_handle);

	D3D12_GPU_DESCRIPTOR_HANDLE UAVGPUHandle(const UAVHandle& a_handle);


	/// <summary>
	/// ヒープの生ポインタ取得
	/// </summary>
	ID3D12DescriptorHeap* GetCBV_SRV_UAVHeap() const;

	//==========================================================================================
	// 
	// ImGui
	// 
	//==========================================================================================

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

	DSVHeap& RefDSVHeap();

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
	DSVHeap m_dsvHeap;

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