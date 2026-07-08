#pragma once
namespace Engine::Graphics
{
	/// <summary>
	/// CPU側のフレームリソース管理クラス
	/// </summary>
	class FrameResourceManager
	{
	public:

		/// <summary>
		/// 配列の割り当て
		/// </summary>
		/// <typeparam name="T">型</typeparam>
		/// <param name="a_data">配列データ</param>
		/// <returns>割り当てられた型の領域ハンドル</returns>
		template<typename T>
		RangeHandle<T> AllocateRange(const std::vector<T>& a_data);

		

	private:

		template<typename T>
		Pool::RangePool<T>& RefPool();

	private:
		// フレームで単体のデータ
		CameraData m_cpuCamera = {};			// CPU側のカメラデータ
		CameraData m_gpuCamera = {};			// 最終的にGPUに送る形にされたデータ

		AmbientData m_ambientData = {};

		// 毎フレームクリアされて初めから詰め込まれるデータ
		std::vector<InstanceData>	m_instanceDataVec = {};			// １メッシュ = １インスタンスとしたデータの配列
		std::vector<SubSetData>		m_subsetDataVec = {};			// メッシュが持つサブセット情報

		// レイ用メッシュ頂点情報メガバッファ
		Pool::RangePool<Resource::RTVertex>	m_rtVerticesVec = {};	// 頂点バッファ
		Pool::RangePool<uint32_t>			m_rtIndexVec = {};		// インデックスバッファ

		// 前フレーム計算用キャッシュ
		DXSM::Matrix m_prevViewMat = {};
		DXSM::Matrix m_prevProjMat = {};
		DXSM::Matrix m_prevNonJitteredViewProj = {};
		int m_totlaFrameCount = 0;
	};

	template<typename T>
	inline RangeHandle<T> FrameResourceManager::AllocateRange(const std::vector<T>& a_data)
	{
		return RefPool<T>().AllocateRange(static_cast<uint32_t>(a_data.size()));
	}

}