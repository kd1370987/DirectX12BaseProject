#include "RenderGraphResourceViewPanel.h"

#include "../../../MainEngine.h"

// グラフィックス系
#include "../../../Graphics/GraphicEngine.h"
#include "../../../Graphics/RenderingPipeline/RenderGraph/RenderGraph.h"
#include "../../../Graphics/RenderingPipeline/RenderGraph/Resource/VirtualResource/VirtualResource.h"
#include "../../../Graphics/RenderingPipeline/Core/Pass/Pass.h"

// D3D系
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Editor
{
	namespace
	{
		// よく使うものだけ名前を出す。
		// 一覧の目的は「どのリソースが何で確保されているか」の把握なので、
		// 網羅していなくても数値が出れば追える
		const char* ToFormatName(DXGI_FORMAT a_format)
		{
			switch (a_format)
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM:		return "R8G8B8A8_UNORM";
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:	return "R8G8B8A8_UNORM_SRGB";
			case DXGI_FORMAT_R10G10B10A2_UNORM:		return "R10G10B10A2_UNORM";
			case DXGI_FORMAT_R11G11B10_FLOAT:		return "R11G11B10_FLOAT";
			case DXGI_FORMAT_R16G16B16A16_FLOAT:	return "R16G16B16A16_FLOAT";
			case DXGI_FORMAT_R16G16_FLOAT:			return "R16G16_FLOAT";
			case DXGI_FORMAT_R16_FLOAT:				return "R16_FLOAT";
			case DXGI_FORMAT_R32_FLOAT:				return "R32_FLOAT";
			case DXGI_FORMAT_R32_TYPELESS:			return "R32_TYPELESS";
			case DXGI_FORMAT_D32_FLOAT:				return "D32_FLOAT";
			case DXGI_FORMAT_UNKNOWN:				return "UNKNOWN";
			default:								return "(other)";
			}
		}

		// 用途フラグを短く並べる
		std::string ToUsageText(Resource::TextureUsage a_usage)
		{
			std::string _text = {};
			auto _add = [&_text](const char* a_name)
				{
					if (!_text.empty()) _text += " | ";
					_text += a_name;
				};

			if (HasFlag(a_usage, Resource::TextureUsage::RTV)) _add("RTV");
			if (HasFlag(a_usage, Resource::TextureUsage::DSV)) _add("DSV");
			if (HasFlag(a_usage, Resource::TextureUsage::SRV)) _add("SRV");
			if (HasFlag(a_usage, Resource::TextureUsage::UAV)) _add("UAV");

			if (_text.empty()) _text = "-";
			return _text;
		}
	}

	//======================================================================================
	// レンダリングパイプラインが確保しているリソースの一覧
	//
	// 実行インスタンスはカメラごとにあるので、まずカメラを選んでから中身を見る。
	// 設計図(アセット)側はリソースの実体を持たないので、ここには出てこない
	//======================================================================================
	void RenderGraphResourceViewPanel::OnDrawImGui(EditorContext& a_editContext)
	{
		auto* _pGraphicsEngine = MainEngine::Instance().RefGraphicsEngine();
		if (!_pGraphicsEngine)
		{
			ImGui::TextDisabled("GraphicsEngine がありません");
			return;
		}

		// 今フレーム回っているカメラのグラフ
		const auto _graphVec = _pGraphicsEngine->CollectPipelineGraphs();
		if (_graphVec.empty())
		{
			ImGui::TextDisabled("動いているパイプラインがありません");
			ImGui::TextDisabled("カメラに描画構成(RenderingPipelineAsset)を設定してください");
			return;
		}

		// カメラが増減すると添字がずれるので、範囲に収め直す
		if (m_selectedGraph >= static_cast<int>(_graphVec.size())) m_selectedGraph = 0;

		if (ImGui::BeginCombo("Pipeline", _graphVec[m_selectedGraph].name.c_str()))
		{
			for (int _i = 0; _i < static_cast<int>(_graphVec.size()); ++_i)
			{
				const bool _isSelected = (m_selectedGraph == _i);
				if (ImGui::Selectable(_graphVec[_i].name.c_str(), _isSelected)) m_selectedGraph = _i;
				if (_isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		const auto* _pGraph = _graphVec[m_selectedGraph].pGraph;
		if (!_pGraph) return;

		// 名前でリソースを探す。出しっぱなしの欄なので入力は消さない
		const std::string& _search = EditorHelper::DrawSearchBox("##ResourceSearch", "Search resource...", false);

		ImGui::SameLine();
		ImGui::TextDisabled("| %d 本", static_cast<int>(_pGraph->GetVirtualResources().size()));

		ImGui::Separator();

		if (ImGui::BeginChild("ResourceViewScrollRegion", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
		{
			const auto& _virtualVec = _pGraph->GetVirtualResources();

			for (uint32_t _i = 0; _i < static_cast<uint32_t>(_virtualVec.size()); ++_i)
			{
				const auto& _virtual = _virtualVec[_i];

				if (!EditorHelper::IsMatchSearch(_search, _virtual.GetName())) continue;

				if (!ImGui::TreeNodeEx(_virtual.GetName().c_str(),
					ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					continue;
				}

				ImGui::Text("format : %s", ToFormatName(_virtual.GetFormat()));
				ImGui::Text("usage  : %s", ToUsageText(_virtual.GetUsage()).c_str());

				if (_virtual.IsBuffer())
				{
					// バッファは width にバイト数が入っている
					ImGui::Text("size   : %llu bytes", _virtual.GetWidth());
				}
				else
				{
					ImGui::Text("size   : %llu x %u", _virtual.GetWidth(), _virtual.GetHeight());
				}

				if (_virtual.IsImported()) ImGui::TextDisabled("外部から差し込まれたリソース");
				if (_virtual.IsTemporal()) ImGui::TextDisabled("履歴つき(2枚組)");

				DrawLifetime(*_pGraph, _virtual);

				ImGui::Separator();

				DrawResourceImage(*_pGraph, _i, _virtual.GetPhysicalCount());

				ImGui::TreePop();
			}
		}
		ImGui::EndChild();
	}

	//======================================================================================
	// 生存区間 : このリソースに触る最初のパスと最後のパス
	//
	// 区間が重ならないリソース同士は同じ実体を使い回せる。
	// 使い回せない事情があるものはその理由まで出す
	//======================================================================================
	void RenderGraphResourceViewPanel::DrawLifetime(
		const Graphics::Pipeline::RenderGraph& a_graph, const Graphics::Pipeline::VirtualResource& a_resource)
	{
		if (!a_resource.HasLifetime())
		{
			ImGui::TextDisabled("lifetime : どのパスも触っていません");
			return;
		}

		const auto& _compiledVec = a_graph.GetCompiledPasses();

		auto _passName = [&_compiledVec](uint32_t a_index) -> const char*
			{
				if (a_index >= _compiledVec.size()) return "?";
				if (!_compiledVec[a_index].pPass) return "?";

				return _compiledVec[a_index].pPass->GetName().c_str();
			};

		const uint32_t _first = a_resource.GetFirstPassIndex();
		const uint32_t _last = a_resource.GetLastPassIndex();

		ImGui::Text("lifetime : #%u %s  ->  #%u %s",
			_first, _passName(_first),
			_last, _passName(_last));

		// 何パスぶん抱えているか。長いものほど使い回しの邪魔になる
		ImGui::SameLine();
		ImGui::TextDisabled("(%u パス)", _last - _first + 1);

		if (a_resource.IsAliasable()) return;

		// 使い回せない理由
		if (a_resource.IsImported())		ImGui::TextDisabled("  実体がグラフの外にあるので使い回せません");
		else if (a_resource.IsTemporal())	ImGui::TextDisabled("  区間がフレームをまたぐので使い回せません");
	}

	//======================================================================================
	// リソースの中身を絵として出す
	//
	// 履歴つきは2枚あるので、今フレーム書く側と前フレームぶんを並べる
	//======================================================================================
	void RenderGraphResourceViewPanel::DrawResourceImage(
		const Graphics::Pipeline::RenderGraph& a_graph, uint32_t a_resourceIndex, uint32_t a_sliceCount)
	{
		Graphics::Pipeline::ResourceHandle _handle = {};
		_handle.index = a_resourceIndex;

		for (uint32_t _slice = 0; _slice < a_sliceCount; ++_slice)
		{
			D3D12::GPUResource* _pResource = a_graph.RefGPUResource(_handle, _slice);
			if (!_pResource)
			{
				ImGui::TextDisabled("実体がありません");
				continue;
			}

			// SRV として誰も読んでいないリソースには ImGui 用のビューが無い。
			// コピー先として作っただけのものがこれにあたる
			if (!_pResource->GetImGuiSRV().IsValid())
			{
				ImGui::TextDisabled("SRV を持たないので絵にできません");
				continue;
			}

			if (a_sliceCount > 1)
			{
				ImGui::TextDisabled(_slice == 0 ? "今フレーム" : "前フレーム");
			}

			// アスペクト比の計算
			const float _aspectRatio = m_windowWidth / m_windowHeight;

			// 現在のノード内(インデント込み)の利用可能な横幅を取得
			const float _drawWidth = ImGui::GetContentRegionAvail().x;
			const float _drawHeight = _drawWidth / _aspectRatio;

			auto _gpuHandle =
				D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pResource->GetImGuiSRV());

			EditorHelper::DrawSRVView(_gpuHandle, _drawWidth, _drawHeight);
		}
	}
}
