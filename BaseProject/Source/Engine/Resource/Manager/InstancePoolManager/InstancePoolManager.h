#pragma once

// インスタンスプール
#include "../../Data/StateMachineAsset/StateMachineAsset.h"

namespace Engine::Resource
{
	// ECSなどのポインタが持てない場面や、
	// まとまったメモリ領域で配列を扱いたい場合にハンドルとして管理するクラス
	class InstancePoolManager
	{
	public:

		// 領域を確保
		template<typename T>
		Handle<T> Allocate();

		// 単体を取得
		template<typename T>
		const T* Get(const Handle<T>& a_handle) const;
		template<typename T>
		T* Ref(const Handle<T>& a_handle);

		// プールの取得
		template<typename T>
		const Pool::ItemPool<T>& GetPool() const;
		template<typename T>
		Pool::ItemPool<T>& RefPool();

	private:

		// ステートマシンインスタンス用
		Pool::ItemPool<StateMachinInstance> m_stateMachinInstanceData;
	private:
		InstancePoolManager() {};
		~InstancePoolManager() {};
	public:

		static InstancePoolManager& Instance()
		{
			static InstancePoolManager _instance;
			return _instance;
		}
	};
	template<typename T>
	inline Handle<T> InstancePoolManager::Allocate()
	{
		T _data;
		return RefPool<T>().Add(std::move(_data));
	}
	template<typename T>
	const T* InstancePoolManager::Get(const Handle<T>& a_handle) const
	{
		return GetPool<T>().Get(a_handle);
	}

	template<typename T>
	inline T* InstancePoolManager::Ref(const Handle<T>& a_handle)
	{
		return RefPool<T>().Ref(a_handle);
	}

	// ---- プール取得は特殊化 ----
	// 取得
	template<> inline const Pool::ItemPool<StateMachinInstance>& InstancePoolManager::GetPool<StateMachinInstance>() const {return m_stateMachinInstanceData;}

	// 参照
	template<> inline Pool::ItemPool<StateMachinInstance>& InstancePoolManager::RefPool<StateMachinInstance>() { return m_stateMachinInstanceData; }
}