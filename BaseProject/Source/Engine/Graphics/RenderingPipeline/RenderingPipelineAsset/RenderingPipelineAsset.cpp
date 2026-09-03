#include "RenderingPipelineAsset.h"

#include "../Core/Pass/Pass.h"
#include "../Internal/Connection.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderingPipelineMetaRegistry.h"
#include "../StandardPipeline/StandardPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	//
	// RenderingPipelineAsset
	//
	// パス・つなぎ・実行順はすべて RenderGraph の持ち物。
	// ここは「グラフを1つ抱えて、それを編集するUIを出す」役に徹する
	//
	//======================================================================================

	// RenderGraph を unique_ptr で持つので、生成/破棄はここ(完全型が見える場所)に置く
	RenderingPipelineAsset::RenderingPipelineAsset()
		: m_upRenderGraph(std::make_unique<RenderGraph>())
	{}

	RenderingPipelineAsset::~RenderingPipelineAsset()
	{
		DestroyContext();
	}

	// ImNodes のエディターコンテキストは生ポインタで持っているので、
	// 移した側を必ず nullptr にする(両方が同じコンテキストを解放しないように)
	RenderingPipelineAsset::RenderingPipelineAsset(RenderingPipelineAsset&& a_other) noexcept
		: m_name(std::move(a_other.m_name))
		, m_pMetaRegistry(a_other.m_pMetaRegistry)
		, m_upRenderGraph(std::move(a_other.m_upRenderGraph))
		, m_context(a_other.m_context)
		, m_applyPositions(a_other.m_applyPositions)
		, m_pendingDeletePass(a_other.m_pendingDeletePass)
	{
		a_other.m_context = nullptr;
		a_other.m_pMetaRegistry = nullptr;
	}

	RenderingPipelineAsset& RenderingPipelineAsset::operator=(RenderingPipelineAsset&& a_other) noexcept
	{
		if (this == &a_other) return *this;

		DestroyContext();

		m_name = std::move(a_other.m_name);
		m_pMetaRegistry = a_other.m_pMetaRegistry;
		m_upRenderGraph = std::move(a_other.m_upRenderGraph);
		m_context = a_other.m_context;
		m_applyPositions = a_other.m_applyPositions;
		m_pendingDeletePass = a_other.m_pendingDeletePass;

		a_other.m_context = nullptr;
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

		// ロード直後は ImNodes へノード座標を流し込む
		if (a_arch.IsLoading())
		{
			// 古いデータには出口が入っていないので、ここで必ず用意する
			EnsureFinalPass();
			RequestApplyLoadPositions();
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

	void RenderingPipelineAsset::Save(const std::string& a_baseFilePath)
	{
		// 保存の前に、ImNodes 上で動かしたノード座標を書き戻す
		SyncPositions();

		auto _fileDir = Engine::File::GetDirFromPath(a_baseFilePath);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_baseFilePath);

		// 読み込みと同じくJSON固定(理由は RenderingPipelineAssetIO::LoadFromFile を参照)
		Persistence::Archive _arch(Persistence::Archive::Mode::Save, _fileDir, _fileName, kExtension,
			Persistence::Archive::ArchiveFormat::Json);
		Archive(_arch);
	}

	//======================================================================================
	//
	// エディター
	//
	//======================================================================================
	void RenderingPipelineAsset::DrawEditor()
	{
		if (!m_upRenderGraph) return;

		// 出口は常駐。レジストリが後から入った場合もここで揃う
		EnsureFinalPass();

		// インスタンスごとにポップアップ/ウィジェットIDを分離
		ImGui::PushID(this);

		DrawAddPass();
		ImGui::SameLine();

		// 構成が変わっていなければ押しても結果は同じなので、そのときは通さない
		if (ImGui::Button("Compile") && m_isDirty)
		{
			Compile();
		}
		ImGui::SameLine();
		if (m_isDirty)	ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Modified");
		else			ImGui::TextDisabled("Compiled");

		ImGui::SameLine();
		ImGui::TextDisabled("| Pass : %d", static_cast<int>(m_upRenderGraph->GetPasses().size()));

		// 既存の描画と同じ流れを一式組む。
		// 今入っているものは全部捨てるので、押し間違いが痛い分だけ確認を挟む
		ImGui::SameLine();
		if (ImGui::Button("Standard")) ImGui::OpenPopup("StandardPipelinePopup");

		if (ImGui::BeginPopup("StandardPipelinePopup"))
		{
			ImGui::TextDisabled("今のパスと配線をすべて捨てて組み直します");
			if (Engine::Editor::EditorHelper::CreateButton("Build") && m_pMetaRegistry)
			{
				BuildStandardPipeline(*this, *m_pMetaRegistry);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		ImGui::Separator();

		DrawValidation();

		DrawSelectedPassDetail();

		DrawNodeEditor();

		// 線の生成は EndNodeEditor の後でないと拾えない
		HandleCreateLink();

		ImGui::PopID();
	}

	// 繋ぎ方の不備をその場で見せる。
	// ログにしか出ないと、パスが増えたときにどのノードが原因か追えなくなる
	void RenderingPipelineAsset::DrawValidation()
	{
		if (!m_upRenderGraph) return;

		std::vector<ValidationIssue> _issueVec = {};
		const bool _isValid = m_upRenderGraph->Validate(&_issueVec);

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

				Pass* _pPass = m_upRenderGraph->FindPass(_issue.passGUID);
				if (!_pPass) continue;

				ImNodes::EditorContextSet(m_context);
				ImNodes::ClearNodeSelection();
				ImNodes::SelectNode(_pPass->GetNodeID());
			}
			ImGui::TreePop();
		}
		ImGui::Separator();
	}

	void RenderingPipelineAsset::DrawNodeEditor()
	{
		EnsureContext();
		ImNodes::EditorContextSet(m_context);

		// ロード後の初回Drawでノード座標を反映
		// (メインスレッド・コンテキスト有効状態でないと ImNodes を触れない)
		if (m_applyPositions)
		{
			for (auto& _upPass : m_upRenderGraph->GetPasses())
			{
				if (!_upPass) continue;
				const Math::Vector2& _pos = _upPass->GetEditorPos();
				ImNodes::SetNodeEditorSpacePos(_upPass->GetNodeID(), ImVec2(_pos.x, _pos.y));
			}
			m_applyPositions = false;
		}

		ImNodes::BeginNodeEditor();

		const auto& _connectionMap = m_upRenderGraph->GetConnections();
		for (auto& _upPass : m_upRenderGraph->GetPasses())
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

				Pass* _pDst = m_upRenderGraph->FindPass(_connection.dstPassGUID);
				if (!_pDst) continue;

				const Slot* _pDstSlot = _pDst->FindInputSlot(_connection.dstSlotID);
				if (!_pDstSlot) continue;

				_connection.EditConnection(_pSrcSlot->pinID, _pDstSlot->pinID);
			}
		}

		ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
		ImNodes::EndNodeEditor();

		// 選択中のノード/線を Delete キーで削除
		HandleDeleteSelection();

		// ノード内「Delete Pass」ボタンで予約された削除を実行
		// (パス配列を回している最中に消すとイテレータが壊れる)
		if (m_pendingDeletePass.IsValid())
		{
			m_upRenderGraph->RemovePass(m_pendingDeletePass);
			m_pendingDeletePass = {};
			SetDirty();
		}
	}

	void RenderingPipelineAsset::DrawNode(Pass& a_pass)
	{
		ImNodes::BeginNode(a_pass.GetNodeID());

		Editor::EditorHelper::DrawNodeTitleBar(a_pass.GetName());

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

		// パス固有のノード内UI
		a_pass.EditNode();

		// 出口は常駐なので消させない
		if (!IsFinalPass(a_pass))
		{
			// 削除は反復中に消すとイテレータが壊れるので予約だけする
			ImGui::Spacing();
			if (Editor::EditorHelper::DeleteSmallButton("Delete Pass"))
			{
				m_pendingDeletePass = a_pass.GetGUID();
			}
		}

		ImNodes::EndNode();
	}

	void RenderingPipelineAsset::DrawAddPass()
	{
		if (!m_pMetaRegistry)
		{
			ImGui::TextDisabled("No PassMetaRegistry");
			return;
		}

		if (Engine::Editor::EditorHelper::CreateButton("AddPass"))
		{
			ImGui::OpenPopup("AddPassPopup");
		}
		if (ImGui::BeginPopup("AddPassPopup"))
		{
			ImGui::TextDisabled("Select Pass");
			ImGui::Separator();

			const auto& _allMeta = m_pMetaRegistry->GetAllMeta();
			if (_allMeta.empty())
			{
				ImGui::TextDisabled("No registered pass");
			}
			else
			{
				// 検索用
				const std::string& _search = Editor::EditorHelper::DrawSearchBox();

				// クラス名順に並べて表示
				std::vector<ID<Pass>> _ids;
				_ids.reserve(_allMeta.size());
				for (const auto& [_id, _meta] : _allMeta) _ids.push_back(_id);
				std::sort(_ids.begin(), _ids.end(),
					[&_allMeta](ID<Pass> a_lhs, ID<Pass> a_rhs)
					{return _allMeta.at(a_lhs).name < _allMeta.at(a_rhs).name;}
				);

				// 選択欄
				for (ID<Pass> _id : _ids)
				{
					const auto& _meta = _allMeta.at(_id);

					// 出口は常駐なので、手で足せないようにする
					if (_meta.isFinalPass) continue;

					if (!Editor::EditorHelper::IsMatchSearch(_search, _meta.name)) continue;

					std::string _label = _meta.name + "##addobj" + std::to_string(_id.value);
					if (ImGui::Selectable(_label.c_str()))
					{
						AddPassFromEditor(_id);
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::EndPopup();
		}
	}

	// パスの生成そのものは RenderGraph の仕事。
	// ここは、決まったノード座標を ImNodes 側へ反映するところだけを受け持つ
	void RenderingPipelineAsset::AddPassFromEditor(ID<Pass> a_typeID)
	{
		if (!m_pMetaRegistry || !m_upRenderGraph) return;

		Pass* _pPass = m_upRenderGraph->AddPass(*m_pMetaRegistry, a_typeID);
		if (!_pPass) return;

		SetDirty();

		// まだ描いていないノードでも ImNodes 側は FindOrCreate なので座標だけ先に置ける。
		// 逆に ImNodes::SelectNode は描画前だと ObjectPool に無くて assert するので呼ばないこと
		if (m_context)
		{
			ImNodes::EditorContextSet(m_context);
			const Math::Vector2& _pos = _pPass->GetEditorPos();
			ImNodes::SetNodeEditorSpacePos(_pPass->GetNodeID(), ImVec2(_pos.x, _pos.y));
		}
	}

	// 選択中のパスの詳細(パス固有の設定)を出す。
	// ノードの中に全部詰めると線が見えなくなるので、細かい設定はこちら側で編集する
	void RenderingPipelineAsset::DrawSelectedPassDetail()
	{
		EnsureContext();
		ImNodes::EditorContextSet(m_context);

		if (ImNodes::NumSelectedNodes() != 1) return;

		int _nodeID = 0;
		ImNodes::GetSelectedNodes(&_nodeID);

		Pass* _pPass = m_upRenderGraph->FindPassByNodeID(_nodeID);
		if (!_pPass) return;

		if (ImGui::CollapsingHeader("Selected Pass", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID(_nodeID);
			ImGui::Text("%s", _pPass->GetName().c_str());
			ImGui::Separator();

			// パス固有の設定(フォーマットやスケールなど)もリソースの要件を変えるので、
			// 触られたら Dirty にする。
			// 値を確定したところ(ドラッグを離した等)で1回だけ立つ
			// パラメータだけなら組み直さず、カメラ側へ値を写すだけで済ませる
			switch (_pPass->EditUpdate())
			{
			case EPassEditResult::Structure:	SetDirty();			break;
			case EPassEditResult::Param:		++m_paramVersion;	break;
			default: break;
			}

			ImGui::PopID();
		}
		ImGui::Separator();
	}

	void RenderingPipelineAsset::HandleCreateLink()
	{
		int _startAttr = 0;
		int _endAttr = 0;
		if (!ImNodes::IsLinkCreated(&_startAttr, &_endAttr)) return;

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
			Pass* _pPass = m_upRenderGraph->FindPassByPinID(_attr, &_pSlot, &_isInput);
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
		if (m_upRenderGraph->Link(
			_pSrc->GetGUID(), _pSrcSlot->slotID,
			_pDst->GetGUID(), _pDstSlot->slotID))
		{
			SetDirty();
		}
	}

	void RenderingPipelineAsset::HandleDeleteSelection()
	{
		if (!ImGui::IsKeyPressed(ImGuiKey_Delete, false)) return;

		// 選択中の線を削除
		int _numLinks = ImNodes::NumSelectedLinks();
		if (_numLinks > 0)
		{
			std::vector<int> _links(_numLinks);
			ImNodes::GetSelectedLinks(_links.data());
			for (int _linkID : _links)
			{
				m_upRenderGraph->RemoveLink(_linkID);
			}
			ImNodes::ClearLinkSelection();
			m_upRenderGraph->ApplyLinks();
			SetDirty();
		}

		// 選択中のパスを削除(出入りする線も RemovePass 側で巻き添え削除)
		int _numNodes = ImNodes::NumSelectedNodes();
		if (_numNodes > 0)
		{
			std::vector<int> _nodes(_numNodes);
			ImNodes::GetSelectedNodes(_nodes.data());

			// nodeID -> GUID をここで引いておく(消しながら引くと参照が切れる)
			std::vector<Engine::GUID> _targets;
			_targets.reserve(_nodes.size());
			for (int _nodeID : _nodes)
			{
				Pass* _pPass = m_upRenderGraph->FindPassByNodeID(_nodeID);
				if (!_pPass) continue;

				// 出口は常駐なので Delete キーでも消さない
				if (IsFinalPass(*_pPass)) continue;

				_targets.push_back(_pPass->GetGUID());
			}
			for (const Engine::GUID& _guid : _targets)
			{
				m_upRenderGraph->RemovePass(_guid);
			}
			ImNodes::ClearNodeSelection();
			SetDirty();
		}
	}

	void RenderingPipelineAsset::EnsureContext()
	{
		if (!m_context)
		{
			m_context = ImNodes::EditorContextCreate();
		}
	}

	void RenderingPipelineAsset::DestroyContext()
	{
		if (m_context)
		{
			ImNodes::EditorContextFree(m_context);
			m_context = nullptr;
		}
	}

	void RenderingPipelineAsset::SyncPositions()
	{
		if (!m_upRenderGraph) return;

		EnsureContext();
		ImNodes::EditorContextSet(m_context);

		for (auto& _upPass : m_upRenderGraph->GetPasses())
		{
			if (!_upPass) continue;

			ImVec2 _pos = ImNodes::GetNodeEditorSpacePos(_upPass->GetNodeID());
			_upPass->SetEditorPos(Math::Vector2(_pos.x, _pos.y));
		}
	}
}
