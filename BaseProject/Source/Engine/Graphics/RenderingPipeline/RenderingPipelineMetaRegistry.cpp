#include "RenderingPipelineMetaRegistry.h"

// ---- 登録する標準パス ----
#include "TestGBufferPass/TestGBufferPass.h"
#include "FinalOutputPass/FinalOutputPass.h"

namespace Engine::Graphics::Pipeline
{
	ID<Pass> PassMetaRegistry::GetTypeID(const std::string& a_name) const
	{
		auto _it = m_nameMap.find(a_name);
		if (_it != m_nameMap.end())
		{
			return _it->second;
		}

		// 見つからなければ無効値を返す
		return ID<Pass>();
	}
	ID<Pass> PassMetaRegistry::GetTypeID(const std::type_index& a_index) const
	{
		auto _it = m_typeIndexMap.find(a_index);
		if (_it != m_typeIndexMap.end())
		{
			return _it->second;
		}

		// 見つからなければ無効値を返す
		return ID<Pass>();
	}
	const PassMeta* PassMetaRegistry::GetMeta(ID<Pass> a_id) const
	{
		if (!a_id.IsValid()) return nullptr;

		auto _it = m_metaMap.find(a_id);
		if (_it != m_metaMap.end())
		{
			return &_it->second;
		}

		return nullptr;
	}
	std::unique_ptr<Pass> PassMetaRegistry::Create(ID<Pass> a_id) const
	{
		auto _it = m_funcMap.find(a_id);
		if (_it != m_funcMap.end() && _it->second.factory)
		{
			return _it->second.factory();
		}
		return nullptr;
	}

	// 登録名は保存データのキーになる(ハッシュを取ってタイプIDにしている)ので、
	// 一度出したら変えないこと。変えると既存のパイプラインからパスが消える
	void RegisterBuiltinPasses(PassMetaRegistry& a_registry)
	{
		a_registry.RegisterType<TestGBufferPass>("TestGBufferPass");

		// グラフの出口。どのパイプラインにも自動で1つ置かれる
		a_registry.RegisterType<FinalOutputPass>("FinalOutputPass", true);
	}

	ID<Pass> PassMetaRegistry::GetFinalPassTypeID() const
	{
		for (const auto& [_id, _meta] : m_metaMap)
		{
			if (_meta.isFinalPass) return _id;
		}
		return ID<Pass>();
	}

	bool PassMetaRegistry::IsFinalPassType(ID<Pass> a_id) const
	{
		const PassMeta* _pMeta = GetMeta(a_id);
		return _pMeta && _pMeta->isFinalPass;
	}
}