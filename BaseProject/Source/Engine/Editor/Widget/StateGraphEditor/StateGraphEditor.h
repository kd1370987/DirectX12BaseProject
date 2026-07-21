#pragma once
//==========================================================================================
//
// StateGraphEditor<TNode>
//
// StateGraph<TNode> を編集する汎用ノードエディタUI(ImNodes)。
// Animator / ゲームプレイFSM / GameFlow で共通の
// 「ノード枠・ピン・遷移線・パラメータ編集・遷移条件ポップアップ」をここに一本化する。
//
// マシンごとに違うのは “ノード内部に何を描くか” だけなので、
// それを Draw() の drawBody コールバックで注入する(継承ではなく委譲)。
//
// Save ボタンや参照モデル/初期シーンなどマシン固有UIは各マシン側が描き、
// グラフ編集部分だけをこのウィジェットに任せる。
//
//==========================================================================================
#include "Engine/Resource/StateGraph/StateGraph.h"
#include "Engine/Editor/Drawers/ComponentEdit/ComponentEdit.h"	// DrawEnumCombo
#include "Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"		// Node::TitleBar

namespace Engine::Editor
{
	template<class TNode>
	class StateGraphEditor
	{
	public:
		using Graph = StateGraph::StateGraph<TNode>;
		// ノード内部(ピンの後)に固有UIを描くコールバック
		using DrawBodyFn = std::function<void(TNode&)>;

		StateGraphEditor() = default;
		~StateGraphEditor() { DestroyContext(); }

		// ImNodesEditorContext* を生ポインタで所有するため move のみ許可(コピー禁止)。
		// 資産(StateMachineAssetなど)が値で move される用途に対応する。
		StateGraphEditor(const StateGraphEditor&) = delete;
		StateGraphEditor& operator=(const StateGraphEditor&) = delete;
		StateGraphEditor(StateGraphEditor&& a_other) noexcept
			: m_context(a_other.m_context)
			, m_editingLinkID(a_other.m_editingLinkID)
			, m_applyPositions(a_other.m_applyPositions)
		{
			a_other.m_context = nullptr;
		}
		StateGraphEditor& operator=(StateGraphEditor&& a_other) noexcept
		{
			if (this != &a_other)
			{
				DestroyContext();
				m_context = a_other.m_context;
				m_editingLinkID = a_other.m_editingLinkID;
				m_applyPositions = a_other.m_applyPositions;
				a_other.m_context = nullptr;
			}
			return *this;
		}

		//----------------------------------------------------------------------------------
		// ImNodesコンテキスト管理(グラフごとに独立して複数開けるように専用で持つ)
		//----------------------------------------------------------------------------------
		void EnsureContext()
		{
			if (!m_context) m_context = ImNodes::EditorContextCreate();
		}
		void DestroyContext()
		{
			if (m_context)
			{
				ImNodes::EditorContextFree(m_context);
				m_context = nullptr;
			}
		}

		// Load直後に呼ぶ。ここでは ImNodes を触らず「次のDrawで座標反映する」フラグだけ立てる。
		// (リソースロードは非同期の可能性があり、ImNodesのグローバル状態を
		//  メインスレッド外から触らないための遅延反映)
		void RequestApplyLoadedPositions()
		{
			m_applyPositions = true;
		}

		// Save直前に呼ぶ: ImNodes 上の現在座標をノードへ書き戻す
		void SyncPositions(Graph& a_graph)
		{
			EnsureContext();
			ImNodes::EditorContextSet(m_context);
			for (auto& [_hash, _node] : a_graph.Nodes())
			{
				ImVec2 _pos = ImNodes::GetNodeEditorSpacePos(_node.nodeID);
				_node.editorPos.x = _pos.x;
				_node.editorPos.y = _pos.y;
			}
		}

		//----------------------------------------------------------------------------------
		// 1フレーム分の編集UI(グラフ部分のみ)
		//----------------------------------------------------------------------------------
		void Draw(Graph& a_graph, const DrawBodyFn& a_drawBody)
		{
			// インスタンスごとにポップアップ/ウィジェットIDを分離
			ImGui::PushID(this);

			DrawResetButton(a_graph);
			ImGui::Separator();

			DrawParameters(a_graph);
			ImGui::Separator();

			DrawAddNode(a_graph);

			DrawNodeEditor(a_graph, a_drawBody);

			HandleCreateArrow(a_graph);	// 内部で条件ポップアップも描く

			ImGui::PopID();
		}

	private:
		//----------------------------------------------------------------------------------
		// リセット
		//----------------------------------------------------------------------------------
		void DrawResetButton(Graph& a_graph)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
			if (ImGui::Button("Reset All"))
			{
				ImGui::OpenPopup("Reset Confirmation Popup");
			}
			ImGui::PopStyleColor(3);

			ImVec2 _center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Reset Confirmation Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Are you sure you want to reset the state machine?\nAll nodes, links, and parameters will be permanently deleted.");
				ImGui::Separator();
				if (ImGui::Button("Yes, Reset", ImVec2(120, 0)))
				{
					a_graph.Clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		//----------------------------------------------------------------------------------
		// パラメータ編集
		//----------------------------------------------------------------------------------
		void DrawParameters(Graph& a_graph)
		{
			auto& _params = a_graph.Parameters();

			ImGui::Text("Parameters");
			ImGui::Indent();

			if (ImGui::Button("Add Parameter"))
			{
				ImGui::OpenPopup("Add Parameter Popup");
			}

			ImVec2 _center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Add Parameter Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				static char _paramName[256] = "";
				static int _paramTypeIdx = 0;
				const char* _typeNames[] = { "Float", "Int", "Bool", "Trigger" };

				ImGui::InputText("New Param Name", _paramName, sizeof(_paramName));
				ImGui::Separator();
				ImGui::Combo("Type", &_paramTypeIdx, _typeNames, IM_ARRAYSIZE(_typeNames));
				ImGui::Separator();

				if (ImGui::Button("Create", ImVec2(120, 0)))
				{
					if (std::strlen(_paramName) > 0)
					{
						UINT _hash = StringUtility::ToHash(_paramName);
						if (_params.find(_hash) == _params.end())
						{
							StateGraph::StateParameter _newParam;
							_newParam.name = _paramName;
							_newParam.hash = _hash;
							_newParam.type = static_cast<StateGraph::EParamType>(_paramTypeIdx);
							_params[_hash] = _newParam;

							std::memset(_paramName, 0, sizeof(_paramName));
							ImGui::CloseCurrentPopup();
						}
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					std::memset(_paramName, 0, sizeof(_paramName));
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();

			if (ImGui::TreeNodeEx("ParametersList"))
			{
				const char* _typeNames[] = { "Float", "Int", "Bool", "Trigger" };

				// 削除は反復中に行うと unordered_map のイテレータが壊れるので予約する
				UINT _deleteParamHash = 0;
				int _uiIndex = 0;
				for (auto& [_hash, _param] : _params)
				{
					ImGui::PushID(_uiIndex++);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.3f, 0.3f, 1.0f));
					if (ImGui::SmallButton("x"))
					{
						_deleteParamHash = _hash;
					}
					ImGui::PopStyleColor(2);
					ImGui::SameLine();
					ImGui::Text("[%s] %s", _typeNames[static_cast<int>(_param.type)], _param.name.c_str());
					ImGui::PopID();
				}

				// 予約された削除を実行(参照している遷移条件もまとめて消える)
				if (_deleteParamHash != 0)
				{
					a_graph.RemoveParameter(_deleteParamHash);
				}

				ImGui::TreePop();
			}

			ImGui::Unindent();
		}

		//----------------------------------------------------------------------------------
		// ノード追加
		//----------------------------------------------------------------------------------
		void DrawAddNode(Graph& a_graph)
		{
			if (ImGui::Button("Add Node"))
			{
				ImGui::OpenPopup("Add Node Popup");
			}

			ImVec2 _center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Add Node Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				static char _name[256] = "";
				ImGui::InputText("Node Name", _name, sizeof(_name));
				ImGui::Separator();

				if (ImGui::Button("Create", ImVec2(120, 0)))
				{
					if (std::strlen(_name) > 0)
					{
						a_graph.AddNode(std::string(_name));
						std::memset(_name, 0, sizeof(_name));
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::SetItemDefaultFocus();
				ImGui::Separator();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					std::memset(_name, 0, sizeof(_name));
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		//----------------------------------------------------------------------------------
		// ノードエディタ描画
		//----------------------------------------------------------------------------------
		void DrawNodeEditor(Graph& a_graph, const DrawBodyFn& a_drawBody)
		{
			EnsureContext();
			ImNodes::EditorContextSet(m_context);

			// Load後の初回Drawでノード座標を反映(メインスレッド・コンテキスト有効状態で行う)
			if (m_applyPositions)
			{
				for (auto& [_hash, _node] : a_graph.Nodes())
				{
					ImNodes::SetNodeEditorSpacePos(_node.nodeID, ImVec2(_node.editorPos.x, _node.editorPos.y));
				}
				m_applyPositions = false;
			}

			ImNodes::BeginNodeEditor();

			for (auto& [_hash, _node] : a_graph.Nodes())
			{
				DrawNode(_node, a_drawBody);

				// この node から伸びる矢印を描く
				auto _it = a_graph.Arrows().find(_hash);
				if (_it != a_graph.Arrows().end())
				{
					for (auto& _arrow : _it->second)
					{
						auto _dstIt = a_graph.Nodes().find(_arrow.dstStartHash);
						if (_dstIt != a_graph.Nodes().end())
						{
							_arrow.EditArrow(_node.outPinID, _dstIt->second.inPinID);
						}
					}
				}
			}

			ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
			ImNodes::EndNodeEditor();

			// 選択中のノード/リンクを Delete キーで削除
			HandleDeleteSelection(a_graph);

			// ノード内「Delete Node」ボタンで予約された削除を実行
			if (m_pendingDeleteNode != 0)
			{
				a_graph.RemoveNode(m_pendingDeleteNode);
				m_pendingDeleteNode = 0;
			}
		}

		//----------------------------------------------------------------------------------
		// 選択中ノード/リンクの Delete キー削除
		//----------------------------------------------------------------------------------
		void HandleDeleteSelection(Graph& a_graph)
		{
			if (!ImGui::IsKeyPressed(ImGuiKey_Delete, false)) return;

			// 選択中リンク(遷移矢印)を削除
			int _numLinks = ImNodes::NumSelectedLinks();
			if (_numLinks > 0)
			{
				std::vector<int> _links(_numLinks);
				ImNodes::GetSelectedLinks(_links.data());
				for (int _lid : _links)
				{
					for (auto& [_src, _arrowVec] : a_graph.Arrows())
					{
						_arrowVec.erase(
							std::remove_if(_arrowVec.begin(), _arrowVec.end(),
								[_lid](const StateGraph::TransitionArrow& a) { return a.linkID == _lid; }),
							_arrowVec.end());
					}
				}
				ImNodes::ClearLinkSelection();
			}

			// 選択中ノードを削除(出入りする矢印も RemoveNode 側で巻き添え削除)
			int _numNodes = ImNodes::NumSelectedNodes();
			if (_numNodes > 0)
			{
				std::vector<int> _nodes(_numNodes);
				ImNodes::GetSelectedNodes(_nodes.data());
				for (int _nid : _nodes)
				{
					// nodeID -> hash を引く
					UINT _hash = 0;
					for (auto& [_h, _node] : a_graph.Nodes())
					{
						if (_node.nodeID == _nid) { _hash = _h; break; }
					}
					if (_hash != 0) a_graph.RemoveNode(_hash);
				}
				ImNodes::ClearNodeSelection();
			}
		}

		void DrawNode(TNode& a_node, const DrawBodyFn& a_drawBody)
		{
			ImNodes::BeginNode(a_node.nodeID);

			Node::TitleBar(a_node.name);

			ImNodes::BeginInputAttribute(a_node.inPinID);
			ImGui::Text("In");
			ImNodes::EndInputAttribute();

			ImNodes::BeginOutputAttribute(a_node.outPinID);
			ImGui::Text("Out");
			ImNodes::EndOutputAttribute();

			// マシン固有のノード内部UI
			if (a_drawBody) a_drawBody(a_node);

			// ノード削除ボタン(反復中に消すとイテレータが壊れるので予約だけする)
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.3f, 0.3f, 1.0f));
			if (ImGui::SmallButton("Delete Node"))
			{
				m_pendingDeleteNode = a_node.hash;
			}
			ImGui::PopStyleColor(2);

			ImNodes::EndNode();
		}

		//----------------------------------------------------------------------------------
		// 矢印の生成・条件ポップアップ
		//----------------------------------------------------------------------------------
		void HandleCreateArrow(Graph& a_graph)
		{
			// リンクをダブルクリックで条件編集ポップアップ
			int _hoveredLinkId = 0;
			if (ImNodes::IsLinkHovered(&_hoveredLinkId) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				m_editingLinkID = _hoveredLinkId;
				ImGui::OpenPopup("EditConditionPopup");
			}

			DrawArrowPopup(a_graph);

			// 新規リンク作成
			int _startAttr = 0, _endAttr = 0;
			if (ImNodes::IsLinkCreated(&_startAttr, &_endAttr))
			{
				UINT _srcHash = 0;
				UINT _dstHash = 0;
				for (const auto& [_hash, _node] : a_graph.Nodes())
				{
					if (_startAttr == _node.outPinID || _endAttr == _node.outPinID) _srcHash = _hash;
					if (_startAttr == _node.inPinID || _endAttr == _node.inPinID)  _dstHash = _hash;
				}

				if (_srcHash != 0 && _dstHash != 0)
				{
					StateGraph::TransitionArrow _newArrow;
					_newArrow.linkID = a_graph.GenerateID();
					_newArrow.dstStartHash = _dstHash;
					a_graph.Arrows()[_srcHash].push_back(_newArrow);
				}
			}
		}

		void DrawArrowPopup(Graph& a_graph)
		{
			if (!ImGui::BeginPopup("EditConditionPopup")) return;

			// 編集対象の矢印を検索
			StateGraph::TransitionArrow* _pArrow = nullptr;
			std::vector<StateGraph::TransitionArrow>* _pArrowVec = nullptr;
			for (auto& [_hash, _arrowVec] : a_graph.Arrows())
			{
				for (auto& _arrow : _arrowVec)
				{
					if (_arrow.linkID == m_editingLinkID)
					{
						_pArrow = &_arrow;
						_pArrowVec = &_arrowVec;
						break;
					}
				}
				if (_pArrow) break;
			}
			if (!_pArrow)
			{
				ImGui::EndPopup();
				return;
			}

			ImGui::Text("Edit Link ID : %d", m_editingLinkID);

			if (ImGui::Button("Delete Arrow", ImVec2(90, 0)))
			{
				auto _it = std::remove_if(_pArrowVec->begin(), _pArrowVec->end(),
					[this](const StateGraph::TransitionArrow& a) { return a.linkID == m_editingLinkID; });
				_pArrowVec->erase(_it, _pArrowVec->end());
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}
			ImGui::Separator();

			// 遷移時のブレンド時間(アニメ用途。未使用マシンでは触らなくてよい)
			ImGui::DragFloat("BlendDuration", &_pArrow->blendDuration, 0.01f, 0.0f);
			ImGui::Separator();

			auto& _params = a_graph.Parameters();

			// 遷移条件編集
			int _uiIndex = 0;
			for (auto _it = _pArrow->conditions.begin(); _it != _pArrow->conditions.end();)
			{
				ImGui::Separator();
				ImGui::PushID(_uiIndex);

				if (ImGui::Button("x"))
				{
					_it = _pArrow->conditions.erase(_it);
					ImGui::PopID();
					continue;
				}

				// パラメータ選択
				const char* _preview = "None";
				if (_params.find(_it->paramHash) != _params.end())
				{
					_preview = _params[_it->paramHash].name.c_str();
				}
				ImGui::SetNextItemWidth(150.0f);
				if (ImGui::BeginCombo("##Param", _preview))
				{
					for (auto& [_hash, _param] : _params)
					{
						bool _isSelected = (_it->paramHash == _hash);
						if (ImGui::Selectable(_param.name.c_str(), _isSelected))
						{
							_it->paramHash = _hash;
						}
						if (_isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::Separator();

				// 比較演算子 + 閾値
				ImGui::SetNextItemWidth(100.0f);
				DrawEnumCombo("Condition", _it->op);
				if (_params.find(_it->paramHash) != _params.end())
				{
					auto& _paramDef = _params[_it->paramHash];
					ImGui::SameLine();
					ImGui::SetNextItemWidth(100.0f);
					if (_paramDef.type == StateGraph::EParamType::Float)
					{
						ImGui::InputFloat("##ThresholdF", &_it->thresholdFloat);
					}
					else if (_paramDef.type == StateGraph::EParamType::Int)
					{
						ImGui::InputInt("##ThresholdI", &_it->thresholdInt);
					}
				}

				ImGui::PopID();
				++_it;
				++_uiIndex;
			}
			ImGui::Separator();

			if (ImGui::Button("Add Condition"))
			{
				_pArrow->conditions.emplace_back();
			}

			ImGui::EndPopup();
		}

	private:
		ImNodesEditorContext* m_context = nullptr;
		int m_editingLinkID = 0;
		bool m_applyPositions = false;	// Load後、次のDrawで座標反映する
		UINT m_pendingDeleteNode = 0;	// このフレーム内で削除予約されたノードのhash
	};
}
