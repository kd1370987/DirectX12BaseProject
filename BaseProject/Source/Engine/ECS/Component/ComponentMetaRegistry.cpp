#include "ComponentMetaRegistry.h"

namespace Engine::ECS
{
	//======================================================================================
	// 見つからなかったときに返す空の情報
	//--------------------------------------------------------------------------------------
	// 取得関数は参照で返すので、ローカル変数や一時オブジェクトを返すと
	// 呼び出し側がぶら下がった参照を読むことになる。寿命のある空を1つずつ置いておく
	//======================================================================================
	namespace
	{
		const ComponentMeta s_emptyMeta = {};
		const ComponentFunc s_emptyFunc = {};
	}

	//======================================================================================
	// 型ごとの置き場所を未登録へ戻す
	//--------------------------------------------------------------------------------------
	// 置き場所は静的なので、レジストリが消えても番号が残る。
	// 残したままだと、次に作ったレジストリが「別のレジストリが登録済み」と判断して止まる
	//======================================================================================
	ComponentMetaRegistry::~ComponentMetaRegistry()
	{
		for (auto _reset : m_resetSlotFuncVec)
		{
			_reset();
		}
	}

	ComponentTypeID ComponentMetaRegistry::GetTypeID(const std::string& a_name)
	{
		auto _it = m_compNameMap.find(a_name);
		if (_it != m_compNameMap.end())
		{
			return _it->second;
		}
		return Limits::INVALID_COMPONENTTYPEID;
	}
	ComponentTypeID ComponentMetaRegistry::GetTypeID(const std::type_index& a_index) const
	{
		auto _it = m_typeIndexMap.find(a_index);
		if (_it != m_typeIndexMap.end())
		{
			return _it->second;
		}
		return Limits::INVALID_COMPONENTTYPEID;
	}

	const ComponentMeta& ComponentMetaRegistry::GetMetaData(const ComponentTypeID& a_id) const
	{
		auto _it = m_compTypeMap.find(a_id);
		if (_it != m_compTypeMap.end())
		{
			return _it->second;
		}

		assert(0 && "登録していないコンポーネントです");
		return s_emptyMeta;
	}

	const ComponentMeta& ComponentMetaRegistry::GetMetaData(const std::type_index& a_index) const
	{
		auto _it = m_typeIndexMap.find(a_index);
		if (_it != m_typeIndexMap.end())
		{
			return GetMetaData(_it->second);
		}

		assert(0 && "登録していないコンポーネントです");
		return s_emptyMeta;
	}

	const std::unordered_map<ComponentTypeID, ComponentMeta>& ComponentMetaRegistry::GetAllMetaData() const
	{
		return m_compTypeMap;
	}

	const ComponentFunc& ComponentMetaRegistry::GetFunc(const ComponentTypeID& a_id) const
	{
		auto _it = m_compFuncMap.find(a_id);
		if (_it != m_compFuncMap.end())
		{
			return _it->second;
		}

		// 未登録の型は、どの関数も空のまま(呼ぶ側は空なら飛ばす)
		return s_emptyFunc;
	}
}