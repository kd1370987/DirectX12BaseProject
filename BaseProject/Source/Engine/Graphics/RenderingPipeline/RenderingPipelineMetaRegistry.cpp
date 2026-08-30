#include "RenderingPipelineMetaRegistry.h"

// ---- 登録する標準パス ----
#include "TestGBufferPass/TestGBufferPass.h"
#include "FinalOutputPass/FinalOutputPass.h"
#include "TestClearPass/TestClearPass.h"
#include "GBufferPass/GBufferPass.h"

// ---- ポストプロセス ----
#include "RadialBlurPass/RadialBlurPass.h"
#include "FishEyePass/FishEyePass.h"
#include "CoCPass/CoCPass.h"
#include "DoFPass/DoFPass.h"
#include "TAAPass/TAAPass.h"

// ---- ブルーム ----
#include "BloomExtractPass/BloomExtractPass.h"
#include "GaussianBlurPass/GaussianBlurPass.h"
#include "KawaseBlurPass/KawaseBlurPass.h"
#include "BloomCompositePass/BloomCompositePass.h"

// ---- デノイズ ----
#include "ShadowTemporalAccumulationPass/ShadowTemporalAccumulationPass.h"
#include "ShadowSpatialDenoisePass/ShadowSpatialDenoisePass.h"
#include "GITemporalAccumulationPass/GITemporalAccumulationPass.h"
#include "GISpatialDenoisePass/GISpatialDenoisePass.h"
#include "UpScalePass/UpScalePass.h"

// ---- ジオメトリ・提示 ----
#include "ZPrePass/ZPrePass.h"
#include "SkyPass/SkyPass.h"
#include "ToneMapPass/ToneMapPass.h"
#include "UIPass/UIPass.h"
#include "DebugLinePass/DebugLinePass.h"

// ---- リソース操作 ----
#include "CopyPass/CopyPass.h"

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

		// 不透明モデルをGBufferへ描く(既存パスの移植)
		a_registry.RegisterType<GBufferPass>("GBufferPass");

		// ---- ポストプロセス ----
		a_registry.RegisterType<RadialBlurPass>("RadialBlurPass");
		a_registry.RegisterType<FishEyePass>("FishEyePass");
		a_registry.RegisterType<CoCPass>("CoCPass");
		a_registry.RegisterType<DoFPass>("DoFPass");
		a_registry.RegisterType<TAAPass>("TAAPass");

		// ---- ブルーム ----
		a_registry.RegisterType<BloomExtractPass>("BloomExtractPass");
		a_registry.RegisterType<GaussianBlurPass>("GaussianBlurPass");
		a_registry.RegisterType<KawaseBlurPass>("KawaseBlurPass");
		a_registry.RegisterType<BloomCompositePass>("BloomCompositePass");

		// ---- デノイズ(1ノード＝1回。反復はノードを並べる) ----
		a_registry.RegisterType<ShadowTemporalAccumulationPass>("ShadowTemporalAccumulationPass");
		a_registry.RegisterType<ShadowSpatialDenoisePass>("ShadowSpatialDenoisePass");
		a_registry.RegisterType<GITemporalAccumulationPass>("GITemporalAccumulationPass");
		a_registry.RegisterType<GISpatialDenoisePass>("GISpatialDenoisePass");
		a_registry.RegisterType<UpScalePass>("UpScalePass");

		// ---- ジオメトリ・提示 ----
		a_registry.RegisterType<ZPrePass>("ZPrePass");
		a_registry.RegisterType<SkyPass>("SkyPass");
		a_registry.RegisterType<ToneMapPass>("ToneMapPass");
		a_registry.RegisterType<UIPass>("UIPass");
		a_registry.RegisterType<DebugLinePass>("DebugLinePass");

		// リソースを写すだけの汎用パス(履歴の作成などに使う)
		a_registry.RegisterType<CopyPass>("CopyPass");

		// 配線が通っているかを画面の色で確かめる用
		a_registry.RegisterType<TestClearPass>("TestClearPass");

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