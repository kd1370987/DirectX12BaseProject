#pragma once
namespace Engine::ECS
{
	// リソースに割り振るID
	using ResourceTypeID = uint32_t;

	// RTTIに依存しないコンパイル時型IDの生成
	// std::any では内部がブラックボックス & 実行時にアップキャストや　new が挟まるため
	// 静的データでの管理
	class ResourceTypeManager
	{
	public:

		/// <summary>
		/// 型ごとに任意のIDを割り当てる
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <returns></returns>
		template<typename T>
		static ResourceTypeID GetID()
		{
			static  ResourceTypeID _id = s_typeCounter++;
			return _id;
		}

	private:
		inline static ResourceTypeID s_typeCounter = 0;
	};
}