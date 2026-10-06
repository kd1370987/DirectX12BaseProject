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

	ComponentTypeID ComponentMetaRegistry::GetTypeID(const std::string& a_name) const
	{
		auto _it = m_compNameMap.find(a_name);
		if (_it != m_compNameMap.end())
		{
			return _it->second;
		}
		return Limits::INVALID_COMPONENTTYPEID;
	}

	const ComponentMeta& ComponentMetaRegistry::GetMetaData(const ComponentTypeID& a_id) const
	{
		// タイプIDがそのまま添え字
		if (a_id < m_metaVec.size())
		{
			return m_metaVec[a_id];
		}

		ENGINE_ERRLOG(false, "登録していないコンポーネントです");
		return s_emptyMeta;
	}

	void ComponentMetaRegistry::ExpandRequired(Signature& a_sig) const
	{
		// 足したものがさらに要求を持つことがあるので、増えなくなるまで回す。
		// 型の数は有限で、ビットは立つだけなので必ず止まる
		bool _isChanged = true;
		while (_isChanged)
		{
			_isChanged = false;
			for (ComponentTypeID _typeID = 0; _typeID < m_funcVec.size(); ++_typeID)
			{
				if (!a_sig.test(_typeID)) continue;

				const auto& _addRequired = m_funcVec[_typeID].addRequired;
				if (!_addRequired) continue;

				const Signature _before = a_sig;
				_addRequired(a_sig);
				if (a_sig != _before) _isChanged = true;
			}
		}
	}

	const ComponentFunc& ComponentMetaRegistry::GetFunc(const ComponentTypeID& a_id) const
	{
		// タイプIDがそのまま添え字
		if (a_id < m_funcVec.size())
		{
			return m_funcVec[a_id];
		}

		// 未登録の型は、どの関数も空のまま(呼ぶ側は空なら飛ばす)
		return s_emptyFunc;
	}
}
