#include "RenderingPipelineAsset.h"

#include "../Core/Pass/Pass.h"
#include "../Internal/Connection.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderingPipelineMetaRegistry.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	//
	// RenderingPipelineAsset
	//
	// パス・つなぎ・実行順はすべて RenderGraph の持ち物。
	// ここは「グラフを1つ抱える」役に徹する(編集UIは Editor 側)
	//
	//======================================================================================

	// RenderGraph を unique_ptr で持つので、生成/破棄はここ(完全型が見える場所)に置く
	RenderingPipelineAsset::RenderingPipelineAsset()
		: m_upRenderGraph(std::make_unique<RenderGraph>())
	{}

	RenderingPipelineAsset::~RenderingPipelineAsset() = default;

	// 抱えているのはグラフだけになったので、そのまま運べばよい。
	// 借りているレジストリは移した側から外しておく
	RenderingPipelineAsset::RenderingPipelineAsset(RenderingPipelineAsset&& a_other) noexcept
		: m_name(std::move(a_other.m_name))
		, m_pMetaRegistry(a_other.m_pMetaRegistry)
		, m_upRenderGraph(std::move(a_other.m_upRenderGraph))
		, m_isDirty(a_other.m_isDirty)
		, m_structureVersion(a_other.m_structureVersion)
		, m_paramVersion(a_other.m_paramVersion)
	{
		a_other.m_pMetaRegistry = nullptr;
	}

	RenderingPipelineAsset& RenderingPipelineAsset::operator=(RenderingPipelineAsset&& a_other) noexcept
	{
		if (this == &a_other) return *this;

		m_name = std::move(a_other.m_name);
		m_pMetaRegistry = a_other.m_pMetaRegistry;
		m_upRenderGraph = std::move(a_other.m_upRenderGraph);
		m_isDirty = a_other.m_isDirty;
		m_structureVersion = a_other.m_structureVersion;
		m_paramVersion = a_other.m_paramVersion;

		a_other.m_pMetaRegistry = nullptr;
		return *this;
	}

	// 保存・読込はグラフ側が持っている(パスと配線はあちらの持ち物)
	void RenderingPipelineAsset::Archive(Persistence::Archive& a_arch)
	{
		if (!m_upRenderGraph) return;
		if (!m_pMetaRegistry)
		{
			ENGINE_WARNING("[RenderingPipelineAsset] PassMetaRegistry が未設定のため読み書きできません");
			return;
		}

		a_arch.StringField("pipelineName", m_name);
		m_upRenderGraph->Archive(a_arch, *m_pMetaRegistry);

		// 古いデータには出口が入っていないので、ここで必ず用意する。
		// 読み込んだ座標を ImNodes へ流し込むのはエディター側の仕事
		if (a_arch.IsLoading())
		{
			EnsureFinalPass();
		}
	}

	// グラフの出口が無ければ足す。
	// 常駐させることで、パス側は「自分が画面に出るかどうか」を気にしなくてよくなる
	void RenderingPipelineAsset::EnsureFinalPass()
	{
		if (!m_pMetaRegistry || !m_upRenderGraph) return;

		const ID<Pass> _finalTypeID = m_pMetaRegistry->GetFinalPassTypeID();
		if (!_finalTypeID.IsValid()) return;

		// すでに居れば何もしない
		for (const auto& _upPass : m_upRenderGraph->GetPasses())
		{
			if (_upPass && _upPass->GetTypeID() == _finalTypeID) return;
		}

		Pass* _pPass = m_upRenderGraph->AddPass(*m_pMetaRegistry, _finalTypeID);
		if (!_pPass) return;

		// 出口なので既定では右のほうへ置いておく
		_pPass->SetEditorPos(Math::Vector2(520.0f, 40.0f));
		SetDirty();
	}

	bool RenderingPipelineAsset::IsFinalPass(const Pass& a_pass) const
	{
		if (!m_pMetaRegistry) return false;
		return m_pMetaRegistry->IsFinalPassType(a_pass.GetTypeID());
	}

	void RenderingPipelineAsset::Compile()
	{
		if (!m_upRenderGraph) return;

		// 失敗しても Dirty は下ろす。
		// 直さないまま押し続けても同じ結果にしかならないので、
		// 「押した = 一度は試した」で区切る
		m_upRenderGraph->Compile();
		m_isDirty = false;
	}

	// ノード座標は Pass が持っているので、ここは書き出すだけ。
	// ImNodes 上で動かした位置を書き戻すのは呼ぶ側(エディター)の役
	void RenderingPipelineAsset::Save(const std::string& a_baseFilePath)
	{
		auto _fileDir = Engine::File::GetDirFromPath(a_baseFilePath);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_baseFilePath);

		// 読み込みと同じくJSON固定(理由は RenderingPipelineAssetIO::LoadFromFile を参照)
		Persistence::Archive _arch(Persistence::Archive::Mode::Save, _fileDir, _fileName, kExtension,
			Persistence::Archive::ArchiveFormat::Json);
		Archive(_arch);
	}
}
