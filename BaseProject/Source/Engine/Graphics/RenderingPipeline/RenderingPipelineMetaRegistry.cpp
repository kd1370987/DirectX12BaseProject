#include "RenderingPipelineMetaRegistry.h"

// ---- 登録する標準パス ----
#include "RenderingPasses/Test/TestGBufferPass/TestGBufferPass.h"
#include "RenderingPasses/Present/FinalOutputPass/FinalOutputPass.h"
#include "RenderingPasses/Test/TestClearPass/TestClearPass.h"
#include "RenderingPasses/Geometry/GBufferPass/GBufferPass.h"
#include "RenderingPasses/Lighting/DeferredLightingPass/DeferredLightingPass.h"

// ---- ポストプロセス ----
#include "RenderingPasses/PostEffect/Blur/RadialBlurPass/RadialBlurPass.h"
#include "RenderingPasses/PostEffect/Distortion/FishEyePass/FishEyePass.h"
#include "RenderingPasses/PostEffect/DoF/CoCPass/CoCPass.h"
#include "RenderingPasses/PostEffect/DoF/DoFPass/DoFPass.h"
#include "RenderingPasses/PostEffect/AntiAliasing/TAAPass/TAAPass.h"

// ---- ブルーム ----
#include "RenderingPasses/PostEffect/Bloom/BloomExtractPass/BloomExtractPass.h"
#include "RenderingPasses/PostEffect/Blur/GaussianBlurPass/GaussianBlurPass.h"
#include "RenderingPasses/PostEffect/Bloom/KawaseBlurPass/KawaseBlurPass.h"
#include "RenderingPasses/PostEffect/Bloom/BloomCompositePass/BloomCompositePass.h"

// ---- デノイズ ----
#include "RenderingPasses/PostEffect/Denoise/Shadow/ShadowTemporalAccumulationPass/ShadowTemporalAccumulationPass.h"
#include "RenderingPasses/PostEffect/Denoise/Shadow/ShadowSpatialDenoisePass/ShadowSpatialDenoisePass.h"
#include "RenderingPasses/PostEffect/Denoise/GI/GITemporalAccumulationPass/GITemporalAccumulationPass.h"
#include "RenderingPasses/PostEffect/Denoise/GI/GISpatialDenoisePass/GISpatialDenoisePass.h"
#include "RenderingPasses/UpScale/UpScalePass/UpScalePass.h"

// ---- ジオメトリ・提示 ----
#include "RenderingPasses/Geometry/ZPrePass/ZPrePass.h"
#include "RenderingPasses/Sky/SkyPass/SkyPass.h"
#include "RenderingPasses/PostEffect/ToneMap/ToneMapPass/ToneMapPass.h"
#include "RenderingPasses/UI/UIPass/UIPass.h"
#include "RenderingPasses/Geometry/DebugLinePass/DebugLinePass.h"

// ---- リソース操作 ----
#include "RenderingPasses/Utility/CopyPass/CopyPass.h"
#include "RenderingPasses/Utility/BlendPass/BlendPass.h"
#include "RenderingPasses/Utility/MonitorPass/MonitorPass.h"
#include "RenderingPasses/Geometry/ParticlePass/ParticlePass.h"

// ---- レイトレ ----
#include "RenderingPasses/Lighting/Shadow/RaytracingShadowPass/RaytracingShadowPass.h"
#include "RenderingPasses/Lighting/RaytracingGIPass/RaytracingGIPass.h"

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

		// GBufferと影・GIを合成して色を作る(既存パスの移植)
		a_registry.RegisterType<DeferredLightingPass>("DeferredLightingPass");

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

		// リソースブレンド用のパス
		a_registry.RegisterType<BlendPass>("BlendPass");

		// パスの間にはさんで、流れている絵をノードの中に出す確認用
		a_registry.RegisterType<MonitorPass>("MonitorPass");

		// パーティクル描画(発生と更新は GraphicsEngine 側)
		a_registry.RegisterType<ParticlePass>("ParticlePass");

		// ---- レイトレ ----
		a_registry.RegisterType<RaytracingShadowPass>("RaytracingShadowPass");
		a_registry.RegisterType<RaytracingGIPass>("RaytracingGIPass");

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