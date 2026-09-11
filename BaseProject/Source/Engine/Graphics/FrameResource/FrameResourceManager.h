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

		// レイ用メッシュ頂点情報メガバッファ
		Pool::RangePool<Resource::RTVertex>	m_rtVerticesVec = {};	// 頂点バッファ
		Pool::RangePool<uint32_t>			m_rtIndexVec = {};		// インデックスバッファ

		// 前フレーム計算用キャッシュ
		Math::Matrix m_prevViewMat = {};
		Math::Matrix m_prevProjMat = {};
		Math::Matrix m_prevNonJitteredViewProj = {};
		int m_totlaFrameCount = 0;
	};

	template<typename T>
	inline RangeHandle<T> FrameResourceManager::AllocateRange(const std::vector<T>& a_data)
	{
		return RefPool<T>().AllocateRange(static_cast<uint32_t>(a_data.size()));
	}

}