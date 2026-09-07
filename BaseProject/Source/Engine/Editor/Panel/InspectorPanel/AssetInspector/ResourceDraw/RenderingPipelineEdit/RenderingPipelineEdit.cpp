#include "RenderingPipelineEdit.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Editor/Helper/EditorHelper.h"
#include "Engine/Editor/Internal/EditorContext.h"

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"
#include "Engine/Graphics/RenderingPipeline/Internal/Connection.h"
#include "Engine/Graphics/RenderingPipeline/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPipelineMetaRegistry.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPipelineAsset/RenderingPipelineAsset.h"
#include "Engine/Graphics/RenderingPipeline/StandardPipeline/StandardPipeline.h"

namespace Engine::Editor::Inspector
{
	using namespace Engine::Graphics::Pipeline;

	//======================================================================================
	//
	// 描き始めの確認
	//
	//======================================================================================
	bool RenderingPipelineEditor::OnBeginDraw(EditorContext& a_editContext)
	{
		(void)a_editContext;

		m_pAsset = nullptr;

		auto& _manager = Resource::ResourceManager::Instance();

		// 読み込まれていなければ、ここで読み込む口だけ出す
		if (!_manager.Has<RenderingPipelineAsset>(m_assetGUID))
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				_manager.LoadImmediate<RenderingPipelineAsset>(m_assetGUID);
			}
			return false;
		}

		auto _handle = _manager.GetCache<RenderingPipelineAsset>(m_assetGUID);
		m_pAsset = _manager.Ref(_handle);

		if (!m_pAsset)
		{
			ImGui::Text("Not found asset");
			return false;
		}

		// 読み込み直後はレジストリが入っていないことがあるので、ここで必ず通しておく
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		m_pAsset->SetMetaRegistry(_pGE ? _pGE->RefPassMetaRegistry() : nullptr);

		// 出口は常駐。レジストリが後から入った場合もここで揃う
		m_pAsset->EnsureFinalPass();

		return m_pAsset->RefRenderGraph() != nullptr;
	}

	//======================================================================================
	//
	// ヘッダー(グラフの外)
	//
	//======================================================================================
	void RenderingPipelineEditor::OnDrawHeader(EditorContext& a_editContext)
	{
		DrawToolbar(a_editContext);
		ImGui::Separator();

		DrawValidation();
		DrawSelectedPassDetail();
	}

	void RenderingPipelineEditor::DrawToolbar(EditorContext& a_editContext)
	{
		// 保存。
		// ImNodes 上で動かしたノード座標を書き戻してから書き出す
		if (a_editContext.pAssetProp && ImGui::Button("Save"))
		{
			SyncNodePositions();
			m_pAsset->Save(a_editContext.pAssetProp->filePath);
		}

		// 画面が出ているかどうか。
		// 組めていないと画面は真っ黒になるので、ここで分かるようにしておく
		if (auto* _pGE = MainEngine::Instance().RefGraphicsEngine())
		{
			ImGui::SameLine();
			if (_pGE->IsPipelinePresentActive())	ImGui::TextDisabled("| 画面 : 出力中");
			else								ImGui::TextDisabled("| 画面 : 出ていません");
		}

		ImGui::Separator();

		DrawAddPass();
		ImGui::SameLine();

		// 構成が変わっていなければ押しても結果は同じなので、そのときは通さない
		if (ImGui::Button("Compile") && m_pAsset->IsDirty())
		{
			m_pAsset->Compile();
		}
		ImGui::SameLine();
		if (m_pAsset->IsDirty())	ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Modified");
		else						ImGui::TextDisabled("Compiled");

		const RenderGraph* _pGraph = m_pAsset->GetRenderGraph();
		ImGui::SameLine();
		ImGui::TextDisabled("| Pass : %d", static_cast<int>(_pGraph->GetPasses().size()));

		// 既存の描画と同じ流れを一式組む。
		// 今入っているものは全部捨てるので、押し間違いが痛い分だけ確認を挟む
		ImGui::SameLine();
		if (ImGui::Button("Standard")) ImGui::OpenPopup("StandardPipelinePopup");

		if (ImGui::BeginPopup("StandardPipelinePopup"))
		{
			ImGui::TextDisabled("今のパスと配線をすべて捨てて組み直します");

			PassMetaRegistry* _pRegistry = m_pAsset->RefMetaRegistry();
			if (EditorHelper::CreateButton("Build") && _pRegistry)
			{
				BuildStandardPipeline(*m_pAsset, *_pRegistry);

				// パスが総入れ替えになるので、次の Draw で座標を配り直す
				RequestApplyNodePositions();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void RenderingPipelineEditor::DrawAddPass()
	{
		PassMetaRegistry* _pRegistry = m_pAsset->RefMetaRegistry();
		if (!_pRegistry)
		{
			ImGui::TextDisabled("No PassMetaRegistry");
			return;
		}

		if (EditorHelper::CreateButton("AddPass"))
		{
			ImGui::OpenPopup("AddPassPopup");
		}
		if (!ImGui::BeginPopup("AddPassPopup")) return;

		ImGui::TextDisabled("Select Pass");
		ImGui::Separator();

		const auto& _allMeta = _pRegistry->GetAllMeta();
		if (_allMeta.empty())
		{
			ImGui::TextDisabled("No registered pass");
			ImGui::EndPopup();
			return;
		}

		// 検索用
		const std::string& _search = EditorHelper::DrawSearchBox();

		// クラス名順に並べて表示
		std::vector<ID<Pass>> _ids;
		_ids.reserve(_allMeta.size());
		for (const auto& [_id, _meta] : _allMeta) _ids.push_back(_id);
		std::sort(_ids.begin(), _ids.end(),
			[&_allMeta](ID<Pass> a_lhs, ID<Pass> a_rhs)
			{ return _allMeta.at(a_lhs).name < _allMeta.at(a_rhs).name; }
		);

		// 選択欄
		for (ID<Pass> _id : _ids)
		{
			const auto& _meta = _allMeta.at(_id);

			// 出口は常駐なので、手で足せないようにする
			if (_meta.isFinalPass) continue;

			if (!EditorHelper::IsMatchSearch(_search, _meta.name)) continue;

			std::string _label = _meta.name + "##addobj" + std::to_string(_id.value);
			if (ImGui::Selectable(_label.c_str()))
			{
				AddPassFromEditor(_id);
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	// 繋ぎ方の不備をその場で見せる。
	// ログにしか出ないと、パスが増えたときにどのノードが原因か追えなくなる
	void RenderingPipelineEditor::DrawValidation()
	{
		RenderGraph* _pGraph = m_pAsset->RefRenderGraph();

		std::vector<ValidationIssue> _issueVec = {};
		const bool _isValid = _pGraph->Validate(&_issueVec);

		if (_issueVec.empty())
		{
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Validation : OK");
			return;
		}

		// エラーが1つでもあるとコンパイルは通らない
		if (_isValid)	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Validation : %d warning(s)", static_cast<int>(_issueVec.size()));
		else			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Validation : NG");

		if (ImGui::TreeNodeEx("Issues", _isValid ? 0 : ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (const ValidationIssue& _issue : _issueVec)
			{
				const bool _isError = (_issue.level == ValidationIssue::ELevel::Error);
				const ImVec4 _color = _isError
					? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
					: ImVec4(1.0f, 0.8f, 0.3f, 1.0f);

				ImGui::TextColored(_color, "%s : %s", _isError ? "Error" : "Warn", _issue.message.c_str());

				// クリックでそのノードを選ぶ
				if (!ImGui::IsItemClicked()) continue;
				if (!_issue.passGUID.IsValid()) continue;

				Pass* _pPass = _pGraph->FindPass(_issue.passGUID);
				if (!_pPass) continue;

				// コンテキストは Draw が立てているので、ここでは選び直すだけ
				ImNodes::ClearNodeSelection();
				ImNodes::SelectNode(_pPass->GetNodeID());
			}
			ImGui::TreePop();
		}
		ImGui::Separator();
	}

	// 選択中のパスの詳細(パス固有の設定)を出す。
	// ノードの中に全部詰めると線が見えなくなるので、細かい設定はこちら側で編集する
	void RenderingPipelineEditor::DrawSelectedPassDetail()
	{
		const int _nodeID = GetSingleSelectedNodeID();
		if (_nodeID == 0) return;

		Pass* _pPass = m_pAsset->RefRenderGraph()->FindPassByNodeID(_nodeID);
		if (!_pPass) return;

		if (ImGui::CollapsingHeader("Selected Pass", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID(_nodeID);
			ImGui::Text("%s", _pPass->GetName().c_str());
			ImGui::Separator();

			// パス固有の設定(フォーマットやスケールなど)もリソースの要件を変えるので、
			// 触られたら Dirty にする。
			// 値を確定したところ(ドラッグを離した等)で1回だけ立つ。
			// パラメータだけなら組み直さず、カメラ側へ値を写すだけで済ませる
			IPassEditor* _pEditor = m_passEditorRegistry.Find(*_pPass);
			if (!_pEditor)
			{
				// 登録漏れ : 触れないだけで動きはするので、気づけるように出しておく
				ImGui::TextDisabled("編集UIが登録されていません");
			}
			else
			{
				switch (_pEditor->DrawDetail(*_pPass))
				{
				case EPassEditResult::Structure:	m_pAsset->SetDirty();			break;
				case EPassEditResult::Param:		m_pAsset->SetParamDirty();		break;
				default: break;
				}
			}

			ImGui::PopID();
		}
		ImGui::Separator();
	}

	//======================================================================================
	//
	// グラフの中
	//
	//======================================================================================
	void RenderingPipelineEditor::OnDrawNodes(EditorContext& a_editContext)
	{
		(void)a_editContext;

		RenderGraph* _pGraph = m_pAsset->RefRenderGraph();
		const auto& _connectionMap = _pGraph->GetConnections();

		for (auto& _upPass : _pGraph->GetPasses())
		{
			if (!_upPass) continue;

			DrawNode(*_upPass);

			// このパスから伸びる線を描く
			auto _it = _connectionMap.find(_upPass->GetGUID());
			if (_it == _connectionMap.end()) continue;

			for (const Connection& _connection : _it->second)
			{
				const Slot* _pSrcSlot = _upPass->FindOutputSlot(_connection.srcSlotID);
				if (!_pSrcSlot) continue;

				Pass* _pDst = _pGraph->FindPass(_connection.dstPassGUID);
				if (!_pDst) continue;

				const Slot* _pDstSlot = _pDst->FindInputSlot(_connection.dstSlotID);
				if (!_pDstSlot) continue;

				_connection.EditConnection(_pSrcSlot->pinID, _pDstSlot->pinID);
			}
		}
	}

	void RenderingPipelineEditor::DrawNode(Pass& a_pass)
	{
		ImNodes::BeginNode(a_pass.GetNodeID());

		EditorHelper::DrawNodeTitleBar(a_pass.GetName());

		// 入力ピン : つながっていればリソース名まで出す
		for (const Slot& _in : a_pass.GetInputSlots())
		{
			ImNodes::BeginInputAttribute(_in.pinID);
			if (_in.IsConnected())
			{
				ImGui::Text("%s : %s", _in.pinName.c_str(), _in.name.c_str());
			}
			else
			{
				ImGui::TextDisabled("%s", _in.pinName.c_str());
			}
			ImNodes::EndInputAttribute();
		}

		// 出力ピン : 作るリソース名は宣言時に決まっている
		for (const Slot& _out : a_pass.GetOutputSlots())
		{
			ImNodes::BeginOutputAttribute(_out.pinID);
			ImGui::Text("%s : %s", _out.pinName.c_str(), _out.name.c_str());
			ImNodes::EndOutputAttribute();
		}

		// パス固有のノード内UI。
		// パスは ImGui を知らないので、種類ごとの編集UIをレジストリから引く
		if (IPassEditor* _pEditor = m_passEditorRegistry.Find(a_pass))
		{
			_pEditor->DrawNode(a_pass);
		}

		// 出口は常駐なので消させない
		if (!m_pAsset->IsFinalPass(a_pass))
		{
			// 削除は反復中に消すとイテレータが壊れるので予約だけする
			ImGui::Spacing();
			if (EditorHelper::DeleteSmallButton("Delete Pass"))
			{
				m_pendingDeletePass = a_pass.GetGUID();
			}
		}

		ImNodes::EndNode();
	}

	//======================================================================================
	//
	// 操作(EndNodeEditor の後でないと拾えないもの)
	//
	//======================================================================================
	void RenderingPipelineEditor::OnPostDraw(EditorContext& a_editContext)
	{
		(void)a_editContext;

		HandleDeleteSelection();
		HandlePendingDeletePass();
		HandleCreateLink();
	}

	void RenderingPipelineEditor::HandleCreateLink()
	{
		int _startAttr = 0;
		int _endAttr = 0;
		if (!ImNodes::IsLinkCreated(&_startAttr, &_endAttr)) return;

		RenderGraph* _pGraph = m_pAsset->RefRenderGraph();

		// どちらが出力側で引かれたか分からないので、両端をそれぞれ判定する
		Pass* _pSrc = nullptr;
		Slot* _pSrcSlot = nullptr;
		Pass* _pDst = nullptr;
		Slot* _pDstSlot = nullptr;

		const int _attrs[2] = { _startAttr, _endAttr };
		for (int _attr : _attrs)
		{
			Slot* _pSlot = nullptr;
			bool _isInput = false;
			Pass* _pPass = _pGraph->FindPassByPinID(_attr, &_pSlot, &_isInput);
			if (!_pPass || !_pSlot) continue;

			if (_isInput)
			{
				_pDst = _pPass;
				_pDstSlot = _pSlot;
			}
			else
			{
				_pSrc = _pPass;
				_pSrcSlot = _pSlot;
			}
		}

		// 入力どうし・出力どうしをつないだ場合はここで弾かれる
		if (!_pSrc || !_pSrcSlot || !_pDst || !_pDstSlot) return;

		// 自分自身へのつなぎや、入力スロットの張り替えは RenderGraph 側が面倒を見る
		if (_pGraph->Link(
			_pSrc->GetGUID(), _pSrcSlot->slotID,
			_pDst->GetGUID(), _pDstSlot->slotID))
		{
			m_pAsset->SetDirty();
		}
	}

	void RenderingPipelineEditor::HandleDeleteSelection()
	{
		if (!IsDeleteKeyPressed()) return;

		RenderGraph* _pGraph = m_pAsset->RefRenderGraph();

		// 選択中の線を削除
		const std::vector<int> _linkIDs = GetSelectedLinkIDs();
		if (!_linkIDs.empty())
		{
			for (int _linkID : _linkIDs)
			{
				_pGraph->RemoveLink(_linkID);
			}
			ImNodes::ClearLinkSelection();
			_pGraph->ApplyLinks();
			m_pAsset->SetDirty();
		}

		// 選択中のパスを削除(出入りする線も RemovePass 側で巻き添え削除)
		const std::vector<int> _nodeIDs = GetSelectedNodeIDs();
		if (_nodeIDs.empty()) return;

		// nodeID -> GUID をここで引いておく(消しながら引くと参照が切れる)
		std::vector<Engine::GUID> _targets;
		_targets.reserve(_nodeIDs.size());
		for (int _nodeID : _nodeIDs)
		{
			Pass* _pPass = _pGraph->FindPassByNodeID(_nodeID);
			if (!_pPass) continue;

			// 出口は常駐なので Delete キーでも消さない
			if (m_pAsset->IsFinalPass(*_pPass)) continue;

			_targets.push_back(_pPass->GetGUID());
		}
		for (const Engine::GUID& _guid : _targets)
		{
			_pGraph->RemovePass(_guid);
		}
		ImNodes::ClearNodeSelection();
		m_pAsset->SetDirty();
	}

	void RenderingPipelineEditor::HandlePendingDeletePass()
	{
		if (!m_pendingDeletePass.IsValid()) return;

		m_pAsset->RefRenderGraph()->RemovePass(m_pendingDeletePass);
		m_pendingDeletePass = {};
		m_pAsset->SetDirty();
	}

	// パスの生成そのものは RenderGraph の仕事。
	// ここは、決まったノード座標を ImNodes 側へ反映するところだけを受け持つ
	void RenderingPipelineEditor::AddPassFromEditor(ID<Pass> a_typeID)
	{
		PassMetaRegistry* _pRegistry = m_pAsset->RefMetaRegistry();
		if (!_pRegistry) return;

		Pass* _pPass = m_pAsset->RefRenderGraph()->AddPass(*_pRegistry, a_typeID);
		if (!_pPass) return;

		m_pAsset->SetDirty();

		// まだ描いていないノードでも ImNodes 側は FindOrCreate なので座標だけ先に置ける。
		// 逆に ImNodes::SelectNode は描画前だと ObjectPool に無くて assert するので呼ばないこと
		const Math::Vector2& _pos = _pPass->GetEditorPos();
		ImNodes::SetNodeEditorSpacePos(_pPass->GetNodeID(), ImVec2(_pos.x, _pos.y));
	}

	//======================================================================================
	//
	// ノード座標
	//
	//======================================================================================
	void RenderingPipelineEditor::OnApplyNodePositions()
	{
		if (!m_pAsset) return;

		RenderGraph* _pGraph = m_pAsset->RefRenderGraph();
		if (!_pGraph) return;

		for (auto& _upPass : _pGraph->GetPasses())
		{
			if (!_upPass) continue;

			const Math::Vector2& _pos = _upPass->GetEditorPos();
			ImNodes::SetNodeEditorSpacePos(_upPass->GetNodeID(), ImVec2(_pos.x, _pos.y));
		}
	}

	void RenderingPipelineEditor::OnSyncNodePositions()
	{
		if (!m_pAsset) return;

		RenderGraph* _pGraph = m_pAsset->RefRenderGraph();
		if (!_pGraph) return;

		for (auto& _upPass : _pGraph->GetPasses())
		{
			if (!_upPass) continue;

			ImVec2 _pos = ImNodes::GetNodeEditorSpacePos(_upPass->GetNodeID());
			_upPass->SetEditorPos(Math::Vector2(_pos.x, _pos.y));
		}
	}
}
