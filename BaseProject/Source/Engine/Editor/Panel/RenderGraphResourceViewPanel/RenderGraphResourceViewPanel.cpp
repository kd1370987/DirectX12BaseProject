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

		// バイト数を読める単位で
		std::string ToByteText(uint64_t a_bytes)
		{
			char _buf[64] = {};

			if (a_bytes >= (1ull << 20))		snprintf(_buf, sizeof(_buf), "%.2f MB", static_cast<double>(a_bytes) / (1ull << 20));
			else if (a_bytes >= (1ull << 10))	snprintf(_buf, sizeof(_buf), "%.1f KB", static_cast<double>(a_bytes) / (1ull << 10));
			else								snprintf(_buf, sizeof(_buf), "%llu B", a_bytes);

			return _buf;
		}

		//----------------------------------------------------------------------------------
		// 席の色
		//
		// 色相を席で決めるので、同じ席を使い回しているもの同士が同系色になる。
		// 黄金比で回すと、隣り合った席の色が似ない
		//----------------------------------------------------------------------------------
		ImU32 ToSlotColor(uint32_t a_slotIndex, uint32_t a_orderInSlot, bool a_isDimmed)
		{
			const float _hue = std::fmod(static_cast<float>(a_slotIndex) * 0.6180339887f, 1.0f);

			// 同じ席の中では明るさを振って、何番目に使われたかが分かるようにする
			const float _value = 0.62f + 0.12f * static_cast<float>(a_orderInSlot % 3);

			float _r = 0.f, _g = 0.f, _b = 0.f;
			ImGui::ColorConvertHSVtoRGB(
				_hue,
				a_isDimmed ? 0.18f : 0.62f,
				a_isDimmed ? _value * 0.45f : _value,
				_r, _g, _b);

			return ImGui::ColorConvertFloat4ToU32(ImVec4(_r, _g, _b, 1.0f));
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

		// カメラの選択はタブの外へ置く。
		// タブごとに選ばせると、切り替えたときに別のカメラを見ていることに気づけない
		if (ImGui::BeginTabBar("RenderGraphResourceTabs"))
		{
			if (ImGui::BeginTabItem("Resources"))
			{
				DrawResourceList(*_pGraph);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Aliasing"))
			{
				DrawAliasing(*_pGraph);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	//======================================================================================
	// リソース一覧
	//======================================================================================
	void RenderGraphResourceViewPanel::DrawResourceList(const Graphics::Pipeline::RenderGraph& a_graph)
	{
		// 名前でリソースを探す。出しっぱなしの欄なので入力は消さない
		const std::string& _search = EditorHelper::DrawSearchBox("##ResourceSearch", "Search resource...", false);

		ImGui::SameLine();
		ImGui::TextDisabled("| %d 本", static_cast<int>(a_graph.GetVirtualResources().size()));

		ImGui::Separator();

		if (ImGui::BeginChild("ResourceViewScrollRegion", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
		{
			for (const auto& _virtual : a_graph.GetVirtualResources())
			{
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

				DrawLifetime(a_graph, _virtual);

				ImGui::Separator();

				DrawResourceImage(a_graph, _virtual);

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
		const Graphics::Pipeline::RenderGraph& a_graph, const Graphics::Pipeline::VirtualResource& a_resource)
	{
		const Graphics::Pipeline::ResourceID _resourceID = a_resource.GetResourceID();
		const uint32_t _sliceCount = a_resource.GetPhysicalCount();

		for (uint32_t _slice = 0; _slice < _sliceCount; ++_slice)
		{
			D3D12::GPUResource* _pResource = a_graph.RefGPUResource(_resourceID, _slice);
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

			if (_sliceCount > 1)
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

	//======================================================================================
	//
	// リソースの使い回し(エイリアシング)
	//
	//======================================================================================

	// 割り当ての写しはランタイムが持っていないので、見るたびにここで組む。
	// 50本ほどの配列なので、毎フレーム組み直しても支障はない
	void RenderGraphResourceViewPanel::DrawAliasing(const Graphics::Pipeline::RenderGraph& a_graph)
	{
		const Graphics::Pipeline::AliasingReport _report = Graphics::Pipeline::BuildAliasingReport(a_graph);

		if (_report.IsEmpty())
		{
			ImGui::TextDisabled("まだコンパイルが通っていません");
			return;
		}

		DrawAliasingSummary(_report);

		ImGui::Separator();

		DrawAliasingTimeline(_report);

		DrawAliasingUnassigned(_report);
	}

	//======================================================================================
	// 要約 : 使い回しでどれだけ減ったか
	//======================================================================================
	void RenderGraphResourceViewPanel::DrawAliasingSummary(const Graphics::Pipeline::AliasingReport& a_report)
	{
		// 縦軸の取り方。構造を読むか、無駄を読むかで使い分ける
		ImGui::Checkbox("バイト実寸", &m_isByteScale);

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(
				"off : 席ごとに等間隔。使い回しの構造を読む\n"
				"on  : ヒープ上の実際の高さ。押さえたぶんと使ったぶんの差を読む");
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(160.f);
		ImGui::SliderFloat("##PassWidth", &m_passWidth, 12.f, 90.f, "パス幅 %.0f");

		// 席に着いたぶんだけを足す。
		// 着けなかったものはヒープの外なので、混ぜると削減率が嘘になる
		uint64_t _aliasedTotal = 0;
		for (const auto& _entry : a_report.entries) _aliasedTotal += _entry.size;

		ImGui::Text("席 %d", static_cast<int>(a_report.slots.size()));
		ImGui::SameLine(); ImGui::TextDisabled("|");
		ImGui::SameLine(); ImGui::Text("パス %d", static_cast<int>(a_report.passNames.size()));
		ImGui::SameLine(); ImGui::TextDisabled("|");
		ImGui::SameLine(); ImGui::Text("ヒープ %s", ToByteText(a_report.heapSize).c_str());

		if (_aliasedTotal > 0)
		{
			const float _rate = 100.f * (1.f - static_cast<float>(a_report.heapSize) / static_cast<float>(_aliasedTotal));

			ImGui::SameLine();
			ImGui::TextDisabled("(1本ずつ作れば %s : %.1f%% 削減)", ToByteText(_aliasedTotal).c_str(), _rate);
		}

		// 実体はまだヒープに載っていないので、そのことを隠さない
		ImGui::TextDisabled("※ 実体は個別に作られています。この図は「ヒープに載せたらこう詰まる」という見積もりです");
	}

	//======================================================================================
	// 本体 : 横がパスの実行順、縦が席
	//
	// 矩形1つが仮想リソース1本。同じ行に並んでいるものは同じメモリを順番に使い回している
	//======================================================================================
	void RenderGraphResourceViewPanel::DrawAliasingTimeline(const Graphics::Pipeline::AliasingReport& a_report)
	{
		const uint32_t _passCount = static_cast<uint32_t>(a_report.passNames.size());
		const uint32_t _slotCount = static_cast<uint32_t>(a_report.slots.size());

		if (_slotCount == 0)
		{
			ImGui::TextDisabled("使い回せるリソースがありません");
			return;
		}

		constexpr float _kGutterW = 104.f;		// 左端の席ラベル
		constexpr float _kHeaderH = 20.f;		// 上端のパス番号
		constexpr float _kByteViewH = 340.f;	// バイト実寸のときの高さ

		const float _trackW = _passCount * m_passWidth;
		const float _tracksH = m_isByteScale ? _kByteViewH : (_slotCount * m_slotHeight);

		const ImVec2 _canvasSize(_kGutterW + _trackW, _kHeaderH + _tracksH);

		// 縦横どちらもはみ出るので、専用の枠に入れてスクロールさせる
		const float _viewH = std::min(_canvasSize.y + 20.f, 460.f);

		if (ImGui::BeginChild("AliasingTimeline", ImVec2(0.f, _viewH), true, ImGuiWindowFlags_HorizontalScrollbar))
		{
			ImDrawList* _pDraw = ImGui::GetWindowDrawList();
			const ImVec2 _origin = ImGui::GetCursorScreenPos();

			// 当たり判定は矩形ごとに置かず、盤面ごと1つで取って座標から逆算する
			ImGui::InvisibleButton("AliasingCanvas", _canvasSize);
			const bool _isCanvasHovered = ImGui::IsItemHovered();
			const ImVec2 _mousePos = ImGui::GetIO().MousePos;

			const float _trackX = _origin.x + _kGutterW;
			const float _trackY = _origin.y + _kHeaderH;

			// バイト数を高さへ。実寸表示のときだけ使う
			auto _bytesToY = [&](uint64_t a_bytes) -> float
				{
					if (a_report.heapSize == 0) return _trackY;
					return _trackY + (static_cast<float>(a_bytes) / static_cast<float>(a_report.heapSize)) * _tracksH;
				};

			// 席が押さえている範囲
			auto _slotBand = [&](uint32_t a_slotIndex, float& a_outY0, float& a_outY1)
				{
					if (m_isByteScale)
					{
						const auto& _slot = a_report.slots[a_slotIndex];
						a_outY0 = _bytesToY(_slot.offset);
						a_outY1 = _bytesToY(_slot.offset + _slot.reservedSize);
						return;
					}

					a_outY0 = _trackY + a_slotIndex * m_slotHeight;
					a_outY1 = a_outY0 + m_slotHeight;
				};

			// リソースが実際に使う範囲
			auto _entryBand = [&](const Graphics::Pipeline::AliasingReportEntry& a_entry, float& a_outY0, float& a_outY1)
				{
					if (m_isByteScale)
					{
						a_outY0 = _bytesToY(a_entry.offset);
						a_outY1 = _bytesToY(a_entry.offset + a_entry.size);
						return;
					}

					_slotBand(a_entry.slotIndex, a_outY0, a_outY1);
					a_outY0 += 2.f;
					a_outY1 -= 2.f;
				};

			//------------------------------------------------------------------------------
			// カーソルが乗っている席を先に決める : 同じ席のものをまとめて強調する
			//------------------------------------------------------------------------------
			m_hoveredSlot = static_cast<uint32_t>(-1);
			if (_isCanvasHovered && _mousePos.x >= _trackX)
			{
				for (uint32_t _slot = 0; _slot < _slotCount; ++_slot)
				{
					float _y0 = 0.f, _y1 = 0.f;
					_slotBand(_slot, _y0, _y1);

					if (_mousePos.y >= _y0 && _mousePos.y < _y1) { m_hoveredSlot = _slot; break; }
				}
			}

			//------------------------------------------------------------------------------
			// 席の帯 : 押さえているぶん
			//------------------------------------------------------------------------------
			for (uint32_t _slot = 0; _slot < _slotCount; ++_slot)
			{
				float _y0 = 0.f, _y1 = 0.f;
				_slotBand(_slot, _y0, _y1);

				const bool _isHovered = (_slot == m_hoveredSlot);

				_pDraw->AddRectFilled(
					ImVec2(_trackX, _y0), ImVec2(_trackX + _trackW, _y1),
					_isHovered ? IM_COL32(70, 74, 86, 255) : IM_COL32(46, 48, 56, 255));

				// 席のラベル : 押さえたぶんと、実際に使われた最大
				const auto& _slotInfo = a_report.slots[_slot];

				char _label[96] = {};
				snprintf(_label, sizeof(_label), "Slot %u", _slot);
				_pDraw->AddText(ImVec2(_origin.x + 4.f, _y0 + 2.f), IM_COL32(220, 220, 225, 255), _label);

				// 実寸のときは、押さえたぶんと使ったぶんの差が高さに出る
				if (m_isByteScale && (_y1 - _y0) > 26.f)
				{
					snprintf(_label, sizeof(_label), "%s", ToByteText(_slotInfo.reservedSize).c_str());
					_pDraw->AddText(ImVec2(_origin.x + 4.f, _y0 + 16.f), IM_COL32(150, 150, 158, 255), _label);
				}
			}

			//------------------------------------------------------------------------------
			// パスの区切りと番号
			//------------------------------------------------------------------------------
			// 番号は詰まると読めないので、幅に応じて間引く
			const int _labelStep = (m_passWidth < 26.f) ? 5 : 1;

			for (uint32_t _pass = 0; _pass <= _passCount; ++_pass)
			{
				const float _x = _trackX + _pass * m_passWidth;

				_pDraw->AddLine(
					ImVec2(_x, _trackY), ImVec2(_x, _trackY + _tracksH),
					IM_COL32(255, 255, 255, 18));

				if (_pass >= _passCount) continue;
				if ((_pass % _labelStep) != 0) continue;

				char _num[16] = {};
				snprintf(_num, sizeof(_num), "%u", _pass);
				_pDraw->AddText(ImVec2(_x + 3.f, _origin.y + 3.f), IM_COL32(160, 160, 170, 255), _num);
			}

			//------------------------------------------------------------------------------
			// リソースの矩形
			//------------------------------------------------------------------------------
			const Graphics::Pipeline::AliasingReportEntry* _pHoveredEntry = nullptr;

			uint32_t _prevSlot = static_cast<uint32_t>(-1);
			uint32_t _orderInSlot = 0;

			for (const auto& _entry : a_report.entries)
			{
				// 席ごとに何番目かを数える(entries は席順・実行順に並んでいる)
				if (_entry.slotIndex != _prevSlot) { _prevSlot = _entry.slotIndex; _orderInSlot = 0; }
				else ++_orderInSlot;

				// 生存区間は閉区間なので、最後のパスのマスの右端まで塗る
				const float _x0 = _trackX + _entry.firstPassIndex * m_passWidth;
				const float _x1 = _trackX + (_entry.lastPassIndex + 1) * m_passWidth;

				float _y0 = 0.f, _y1 = 0.f;
				_entryBand(_entry, _y0, _y1);

				// 実寸だと1pxを割ることがあるので、見える高さは残す
				if ((_y1 - _y0) < 3.f) _y1 = _y0 + 3.f;

				const bool _isDimmed = (m_hoveredSlot != static_cast<uint32_t>(-1)) && (_entry.slotIndex != m_hoveredSlot);
				const ImU32 _color = ToSlotColor(_entry.slotIndex, _orderInSlot, _isDimmed);

				_pDraw->AddRectFilled(ImVec2(_x0, _y0), ImVec2(_x1, _y1), _color, 3.f);
				_pDraw->AddRect(ImVec2(_x0, _y0), ImVec2(_x1, _y1), IM_COL32(0, 0, 0, 90), 3.f);

				// 名前 : 枠からはみ出さないように切る
				if ((_x1 - _x0) > 26.f && (_y1 - _y0) > 12.f)
				{
					_pDraw->PushClipRect(ImVec2(_x0 + 2.f, _y0), ImVec2(_x1 - 2.f, _y1), true);
					_pDraw->AddText(ImVec2(_x0 + 4.f, _y0 + 1.f), IM_COL32(16, 16, 20, 255), _entry.name.c_str());
					_pDraw->PopClipRect();
				}

				// カーソルが乗っているものを覚えておく
				if (_isCanvasHovered &&
					_mousePos.x >= _x0 && _mousePos.x < _x1 &&
					_mousePos.y >= _y0 && _mousePos.y < _y1)
				{
					_pHoveredEntry = &_entry;
				}
			}

			//------------------------------------------------------------------------------
			// エイリアシングバリアの印
			//
			// 席の使い手が入れ替わるところに実際に積まれたバリア。
			// 継ぎ目に印が無ければ張り忘れ
			//------------------------------------------------------------------------------
			for (const auto& _barrier : a_report.barriers)
			{
				// バリアが指している後続リソースの席を探す
				const Graphics::Pipeline::AliasingReportEntry* _pAfter = nullptr;
				for (const auto& _entry : a_report.entries)
				{
					if (_entry.resourceID != _barrier.after) continue;
					_pAfter = &_entry;
					break;
				}
				if (!_pAfter) continue;

				float _y0 = 0.f, _y1 = 0.f;
				_slotBand(_pAfter->slotIndex, _y0, _y1);

				const float _x = _trackX + _barrier.passIndex * m_passWidth;

				// 下向きの三角を継ぎ目へ置く
				_pDraw->AddTriangleFilled(
					ImVec2(_x - 5.f, _y0 - 6.f),
					ImVec2(_x + 5.f, _y0 - 6.f),
					ImVec2(_x, _y0 + 1.f),
					IM_COL32(255, 214, 100, 255));
			}

			//------------------------------------------------------------------------------
			// ツールチップ
			//------------------------------------------------------------------------------
			if (_pHoveredEntry)
			{
				ImGui::BeginTooltip();

				ImGui::Text("%s", _pHoveredEntry->name.c_str());
				ImGui::Separator();

				ImGui::Text("Slot %u", _pHoveredEntry->slotIndex);
				ImGui::Text("区間  : #%u -> #%u (%u パス)",
					_pHoveredEntry->firstPassIndex,
					_pHoveredEntry->lastPassIndex,
					_pHoveredEntry->lastPassIndex - _pHoveredEntry->firstPassIndex + 1);

				if (_pHoveredEntry->firstPassIndex < a_report.passNames.size())
				{
					ImGui::TextDisabled("  始 : %s", a_report.passNames[_pHoveredEntry->firstPassIndex].c_str());
				}
				if (_pHoveredEntry->lastPassIndex < a_report.passNames.size())
				{
					ImGui::TextDisabled("  終 : %s", a_report.passNames[_pHoveredEntry->lastPassIndex].c_str());
				}

				ImGui::Text("大きさ : %s", ToByteText(_pHoveredEntry->size).c_str());
				ImGui::Text("位置   : %s", ToByteText(_pHoveredEntry->offset).c_str());

				// 同じ席を直前に使っていた相手 : バリアの before になっているもの
				if (!_pHoveredEntry->prevName.empty())
				{
					ImGui::Separator();
					ImGui::Text("この席を直前に使っていたのは %s", _pHoveredEntry->prevName.c_str());
				}
				else
				{
					ImGui::Separator();
					ImGui::TextDisabled("この席の一人目(バリア不要)");
				}

				ImGui::EndTooltip();
			}
		}
		ImGui::EndChild();
	}

	//======================================================================================
	// 席に着けなかったリソース
	//
	// 使い回せないものこそ、削るときの見どころになる
	//======================================================================================
	void RenderGraphResourceViewPanel::DrawAliasingUnassigned(const Graphics::Pipeline::AliasingReport& a_report)
	{
		if (a_report.unassignedEntries.empty()) return;

		char _header[128] = {};
		snprintf(_header, sizeof(_header), "使い回せていないリソース (%d)###Unassigned",
			static_cast<int>(a_report.unassignedEntries.size()));

		if (!ImGui::CollapsingHeader(_header)) return;

		if (ImGui::BeginTable("UnassignedTable", 3,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("リソース");
			ImGui::TableSetupColumn("大きさ");
			ImGui::TableSetupColumn("理由");
			ImGui::TableHeadersRow();

			for (const auto& _entry : a_report.unassignedEntries)
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", _entry.name.c_str());

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", ToByteText(_entry.size).c_str());

				ImGui::TableSetColumnIndex(2);
				ImGui::TextDisabled("%s", _entry.pUnaliasableReason ? _entry.pUnaliasableReason : "-");
			}

			ImGui::EndTable();
		}
	}
}
