#include "ECSProfilerView.h"

#include "../../../Scene/SceneManager/SceneManager.h"
#include "../../../ECS/World/World.h"
#include "../../../ECS/World/Profile/ECSWorldProfiler.h"

namespace Engine::Editor
{
	namespace
	{
		// バイト数を読みやすい単位にする
		std::string FormatBytes(size_t a_bytes)
		{
			char _buf[64] = {};
			if (a_bytes >= 1024 * 1024)
			{
				snprintf(_buf, sizeof(_buf), "%.2f MB", static_cast<double>(a_bytes) / (1024.0 * 1024.0));
			}
			else if (a_bytes >= 1024)
			{
				snprintf(_buf, sizeof(_buf), "%.1f KB", static_cast<double>(a_bytes) / 1024.0);
			}
			else
			{
				snprintf(_buf, sizeof(_buf), "%zu B", a_bytes);
			}
			return _buf;
		}

		// 名前を区切って1行にする
		std::string JoinNames(const std::vector<std::string>& a_names, const char* a_separator = ", ")
		{
			std::string _out = {};
			for (size_t _i = 0; _i < a_names.size(); ++_i)
			{
				if (_i > 0) _out += a_separator;
				_out += a_names[_i];
			}
			return _out;
		}

		// アーキタイプのコンポーネント名を1行にする
		std::string ArchetypeLabel(const ECS::ECSArchetypeProfile& a_archetype)
		{
			if (a_archetype.components.empty()) return "(no component)";

			std::string _out = {};
			for (size_t _i = 0; _i < a_archetype.components.size(); ++_i)
			{
				if (_i > 0) _out += ", ";
				_out += a_archetype.components[_i].name;
			}
			return _out;
		}

		// 割合(0 除算を避ける)
		float Ratio(double a_value, double a_max)
		{
			return (a_max > 0.0) ? static_cast<float>(a_value / a_max) : 0.0f;
		}

		// 表のフラグ
		constexpr ImGuiTableFlags TABLE_FLAGS =
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
	}

	//======================================================================================
	// 表示
	//======================================================================================
	void ECSProfilerView::Draw()
	{
		ECS::World* _pWorld = Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit())
		{
			Engine::Editor::HelpText("World is not available.");
			return;
		}

		// 見るときにだけ付ける。
		// シーンが変わるとワールドごと作り直されるので、そのたびにここで付け直される
		_pWorld->EnableProfiler();
		ECS::ECSWorldProfiler* _pProfiler = _pWorld->RefProfiler();
		if (!_pProfiler) return;

		// 操作
		Engine::Editor::Field("Pause", m_isPaused);
		if (ImGui::Button("Reset Timings"))
		{
			_pProfiler->ResetTaskTimings();
		}

		// 止めていても、付けたばかりで何も取れていなければ1回は取る
		if (!m_isPaused || _pProfiler->GetSnapshot().captureCount == 0)
		{
			_pProfiler->Capture();
		}
		const ECS::ECSWorldSnapshot& _snapshot = _pProfiler->GetSnapshot();

		Engine::Editor::Line();

		if (ImGui::BeginTabBar("ECSProfilerTab"))
		{
			if (ImGui::BeginTabItem("Overview"))
			{
				DrawOverview(_snapshot);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Archetypes"))
			{
				DrawArchetypes(_snapshot);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Components"))
			{
				DrawComponents(_snapshot);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Systems"))
			{
				DrawSystems(_snapshot);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Resources"))
			{
				DrawResources(_snapshot);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}

	//======================================================================================
	// 概要
	//======================================================================================
	void ECSProfilerView::DrawOverview(const ECS::ECSWorldSnapshot& a_snapshot)
	{
		//------------------------------------------------------------------
		// エンティティ
		//------------------------------------------------------------------
		Engine::Editor::Header("Entity");
		Engine::Editor::Value("Alive", "%u", a_snapshot.entity.aliveCount);
		Engine::Editor::Value("Slots", "%zu", a_snapshot.entity.slotCount);
		Engine::Editor::Value("Recyclable", "%zu", a_snapshot.entity.recycleCount);

		//------------------------------------------------------------------
		// アーキタイプ
		//------------------------------------------------------------------
		size_t _emptyArchetypeCount = 0;
		size_t _chunkCount = 0;
		size_t _freeChunkCount = 0;
		for (const auto& _archetype : a_snapshot.archetypes)
		{
			if (_archetype.entityCount == 0) _emptyArchetypeCount++;
			_chunkCount += _archetype.chunks.size();
			for (const auto& _chunk : _archetype.chunks)
			{
				if (_chunk.isFreeChunk) _freeChunkCount++;
			}
		}

		Engine::Editor::Header("Archetype");
		Engine::Editor::Value("Archetypes", "%zu (empty %zu)", a_snapshot.archetypes.size(), _emptyArchetypeCount);
		Engine::Editor::Value("Chunks", "%zu (kept empty %zu)", _chunkCount, _freeChunkCount);
		Engine::Editor::Value("Generation", "%llu", static_cast<unsigned long long>(a_snapshot.archetypeGeneration));

		//------------------------------------------------------------------
		// チャンクのメモリ
		//------------------------------------------------------------------
		const ECS::ECSChunkMemoryProfile& _memory = a_snapshot.memory;

		Engine::Editor::Header("Chunk Memory");
		Engine::Editor::Value("Blocks", "%zu (x %zu chunks, %s / chunk)", _memory.blockCount, _memory.blockChunkNum, FormatBytes(_memory.chunkBytes).c_str());
		Engine::Editor::Value("Reserved", "%s", FormatBytes(_memory.reservedBytes).c_str());

		// 貸し出し中のチャンク / 確保済みのチャンク
		{
			char _overlay[64] = {};
			snprintf(_overlay, sizeof(_overlay), "%zu / %zu chunks", _memory.usedChunkCount, _memory.totalChunkCount);
			Engine::Editor::ProgressBar("Chunk Use", Ratio(static_cast<double>(_memory.usedChunkCount), static_cast<double>(_memory.totalChunkCount)), _overlay);
		}

		// 貸し出し中のチャンクのうち、生きているエンティティが使っている割合
		{
			const size_t _usedBytes = _memory.usedChunkCount * _memory.chunkBytes;
			char _overlay[64] = {};
			snprintf(_overlay, sizeof(_overlay), "%s / %s", FormatBytes(_memory.liveBytes).c_str(), FormatBytes(_usedBytes).c_str());
			Engine::Editor::ProgressBar("Fill", Ratio(static_cast<double>(_memory.liveBytes), static_cast<double>(_usedBytes)), _overlay);
		}

		//------------------------------------------------------------------
		// 構造変更
		//------------------------------------------------------------------
		const ECS::ECSStructuralChangeProfile& _structural = a_snapshot.structural;

		Engine::Editor::Header("Structural Change");
		Engine::Editor::HelpText("Applied since last capture / Pending now");
		Engine::Editor::Value("Create", "%zu / %zu", _structural.created, _structural.pendingCreate);
		Engine::Editor::Value("Change", "%zu / %zu", _structural.changed, _structural.pendingChange);
		Engine::Editor::Value("Remove", "%zu / %zu", _structural.removed, _structural.pendingRemove);
		Engine::Editor::Value("Refresh", "- / %zu", _structural.pendingRefresh);

		//------------------------------------------------------------------
		// システム・リソース
		//------------------------------------------------------------------
		double _totalMs = 0.0;
		for (const auto& _task : a_snapshot.systemTasks)
		{
			_totalMs += _task.lastMs;
		}

		Engine::Editor::Header("System / Resource");
		Engine::Editor::Value("Tasks", "%zu (last total %.3f ms)", a_snapshot.systemTasks.size(), _totalMs);
		Engine::Editor::Value("Resources", "%zu", a_snapshot.resources.size());
	}

	//======================================================================================
	// アーキタイプ
	//======================================================================================
	void ECSProfilerView::DrawArchetypes(const ECS::ECSWorldSnapshot& a_snapshot)
	{
		m_archetypeFilter.Draw("Filter (component)", 200.0f);
		Engine::Editor::SameLine();
		Engine::Editor::Field("Hide Empty", m_isHideEmptyArchetype);
		Engine::Editor::Field("Sort by Entities", m_isSortArchetypeByEntity);
		Engine::Editor::Line();

		// 表示順
		std::vector<const ECS::ECSArchetypeProfile*> _archetypeVec = {};
		_archetypeVec.reserve(a_snapshot.archetypes.size());
		for (const auto& _archetype : a_snapshot.archetypes)
		{
			if (m_isHideEmptyArchetype && _archetype.entityCount == 0) continue;
			_archetypeVec.push_back(&_archetype);
		}
		if (m_isSortArchetypeByEntity)
		{
			std::stable_sort(_archetypeVec.begin(), _archetypeVec.end(),
				[](const ECS::ECSArchetypeProfile* a_l, const ECS::ECSArchetypeProfile* a_r) { return a_l->entityCount > a_r->entityCount; });
		}

		if (!ImGui::BeginChild("ArchetypeList"))
		{
			ImGui::EndChild();
			return;
		}

		for (const ECS::ECSArchetypeProfile* _pArchetype : _archetypeVec)
		{
			const ECS::ECSArchetypeProfile& _archetype = *_pArchetype;
			const std::string _label = ArchetypeLabel(_archetype);

			if (!m_archetypeFilter.PassFilter(_label.c_str())) continue;

			// 見出し : 番号 / エンティティ数 / チャンク数 / コンポーネント
			// 番号を ID にして、並び替えても開閉状態が付いて回るようにする
			ImGui::PushID(static_cast<int>(_archetype.index));
			const bool _isOpen = ImGui::TreeNodeEx("##Archetype", ImGuiTreeNodeFlags_SpanAvailWidth,
				"#%zu  [%u entities / %zu chunks]  %s",
				_archetype.index, _archetype.entityCount, _archetype.chunks.size(), _label.c_str());

			if (_isOpen)
			{
				// 容量とレイアウト
				Engine::Editor::Text("Capacity : %u / chunk   Stride : %zu B / entity   Max Align : %zu", _archetype.chunkCapacity, _archetype.entityStride, _archetype.maxAlign);

				const size_t _chunkBytes = a_snapshot.memory.chunkBytes;
				{
					char _overlay[64] = {};
					snprintf(_overlay, sizeof(_overlay), "Layout %s / %s",
						FormatBytes(_archetype.layoutBytes).c_str(), FormatBytes(_chunkBytes).c_str());
					Engine::Editor::ProgressBar("Layout", Ratio(static_cast<double>(_archetype.layoutBytes), static_cast<double>(_chunkBytes)), _overlay);
				}

				// コンポーネント配列の配置(チャンク内の並び順)
				if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (ImGui::BeginTable("LayoutTable", 6, TABLE_FLAGS))
					{
						ImGui::TableSetupColumn("Component");
						ImGui::TableSetupColumn("Offset");
						ImGui::TableSetupColumn("Stride");
						ImGui::TableSetupColumn("Size");
						ImGui::TableSetupColumn("Align");
						ImGui::TableSetupColumn("Array");
						ImGui::TableHeadersRow();

						// 先頭はエンティティ配列
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("(Entity)");
						ImGui::TableSetColumnIndex(1); ImGui::Text("0");
						ImGui::TableSetColumnIndex(2); ImGui::Text("%zu", sizeof(ECS::Entity));
						ImGui::TableSetColumnIndex(3); ImGui::Text("%zu", sizeof(ECS::Entity));
						ImGui::TableSetColumnIndex(4); ImGui::Text("%zu", alignof(ECS::Entity));
						ImGui::TableSetColumnIndex(5); ImGui::Text("%s", FormatBytes(sizeof(ECS::Entity) * _archetype.chunkCapacity).c_str());

						for (const auto& _comp : _archetype.components)
						{
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0); ImGui::Text("%s", _comp.name.c_str());
							ImGui::TableSetColumnIndex(1); ImGui::Text("%zu", _comp.offset);
							ImGui::TableSetColumnIndex(2); ImGui::Text("%zu", _comp.stride);
							ImGui::TableSetColumnIndex(3); ImGui::Text("%zu", _comp.size);
							ImGui::TableSetColumnIndex(4); ImGui::Text("%zu", _comp.align);
							ImGui::TableSetColumnIndex(5); ImGui::Text("%s", FormatBytes(_comp.stride * _archetype.chunkCapacity).c_str());
						}
						ImGui::EndTable();
					}
					ImGui::TreePop();
				}

				// チャンクごとの埋まり具合
				if (ImGui::TreeNodeEx("Chunks", ImGuiTreeNodeFlags_DefaultOpen, "Chunks (%zu)", _archetype.chunks.size()))
				{
					for (size_t _i = 0; _i < _archetype.chunks.size(); ++_i)
					{
						const ECS::ECSChunkProfile& _chunk = _archetype.chunks[_i];

						char _overlay[64] = {};
						snprintf(_overlay, sizeof(_overlay), "%u / %u%s",
							_chunk.count, _archetype.chunkCapacity, _chunk.isFreeChunk ? " (kept empty)" : "");

						Engine::Editor::Text("[%zu]", _i);
						Engine::Editor::SameLine();
						ImGui::ProgressBar(Ratio(_chunk.count, _archetype.chunkCapacity), ImVec2(-1.0f, 0.0f), _overlay);
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("Address : 0x%016llx", static_cast<unsigned long long>(_chunk.address));
						}
					}
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		ImGui::EndChild();
	}

	//======================================================================================
	// コンポーネント
	//======================================================================================
	void ECSProfilerView::DrawComponents(const ECS::ECSWorldSnapshot& a_snapshot)
	{
		Engine::Editor::Field("Hide Unused", m_isHideUnusedComponent);
		Engine::Editor::Line();

		if (!ImGui::BeginTable("ComponentTable", 6, TABLE_FLAGS | ImGuiTableFlags_ScrollY)) return;

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("ID");
		ImGui::TableSetupColumn("Component");
		ImGui::TableSetupColumn("Size");
		ImGui::TableSetupColumn("Align");
		ImGui::TableSetupColumn("Archetypes");
		ImGui::TableSetupColumn("Entities");
		ImGui::TableHeadersRow();

		for (const auto& _comp : a_snapshot.components)
		{
			if (m_isHideUnusedComponent && _comp.archetypeCount == 0) continue;

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("%u", static_cast<uint32_t>(_comp.typeID));
			ImGui::TableSetColumnIndex(1); ImGui::Text("%s", _comp.name.c_str());
			ImGui::TableSetColumnIndex(2); ImGui::Text("%zu", _comp.size);
			ImGui::TableSetColumnIndex(3); ImGui::Text("%zu", _comp.align);
			ImGui::TableSetColumnIndex(4); ImGui::Text("%u", _comp.archetypeCount);
			ImGui::TableSetColumnIndex(5); ImGui::Text("%u", _comp.entityCount);
		}
		ImGui::EndTable();
	}

	//======================================================================================
	// システム
	//======================================================================================
	void ECSProfilerView::DrawSystems(const ECS::ECSWorldSnapshot& a_snapshot)
	{
		Engine::Editor::HelpText("Timings are measured while the profiler is attached.");

		if (!ImGui::BeginChild("SystemList"))
		{
			ImGui::EndChild();
			return;
		}

		// タスクはフェーズ順 → 実行順に並んでいるので、フェーズの切れ目で区切る
		const auto& _taskVec = a_snapshot.systemTasks;
		size_t _begin = 0;
		while (_begin < _taskVec.size())
		{
			const ECS::ESystemType _phase = _taskVec[_begin].phase;

			size_t _end = _begin;
			double _phaseMs = 0.0;
			while (_end < _taskVec.size() && _taskVec[_end].phase == _phase)
			{
				_phaseMs += _taskVec[_end].lastMs;
				++_end;
			}

			ImGui::PushID(static_cast<int>(_phase));
			const bool _isOpen = ImGui::TreeNodeEx("##Phase", ImGuiTreeNodeFlags_SpanAvailWidth,
				"%s  [%zu tasks / %.3f ms]", magic_enum::enum_name(_phase).data(), _end - _begin, _phaseMs);

			if (_isOpen)
			{
				if (ImGui::BeginTable("TaskTable", 8, TABLE_FLAGS))
				{
					ImGui::TableSetupColumn("#");
					ImGui::TableSetupColumn("Task");
					ImGui::TableSetupColumn("Last(ms)");
					ImGui::TableSetupColumn("Avg(ms)");
					ImGui::TableSetupColumn("Max(ms)");
					ImGui::TableSetupColumn("Chunks");
					ImGui::TableSetupColumn("Entities");
					ImGui::TableSetupColumn("R / W");
					ImGui::TableHeadersRow();

					for (size_t _i = _begin; _i < _end; ++_i)
					{
						const ECS::ECSSystemTaskProfile& _task = _taskVec[_i];

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0); ImGui::Text("%u", _task.order);
						ImGui::TableSetColumnIndex(1); ImGui::Text("%s", _task.name.c_str());

						// 1度も回っていないものは時間を出しても意味が無い
						ImGui::TableSetColumnIndex(2);
						if (_task.callCount > 0) ImGui::Text("%.3f", _task.lastMs); else ImGui::TextDisabled("-");
						ImGui::TableSetColumnIndex(3);
						if (_task.callCount > 0) ImGui::Text("%.3f", _task.averageMs); else ImGui::TextDisabled("-");
						ImGui::TableSetColumnIndex(4);
						if (_task.callCount > 0) ImGui::Text("%.3f", _task.maxMs); else ImGui::TextDisabled("-");

						// クエリを持たないもの(カスタムタスク・未実行)は伏せる
						ImGui::TableSetColumnIndex(5);
						if (_task.hasQuery) ImGui::Text("%zu", _task.matchedChunkCount); else ImGui::TextDisabled("-");
						ImGui::TableSetColumnIndex(6);
						if (!_task.hasQuery)			ImGui::TextDisabled("-");
						else if (_task.isQueryStale)	ImGui::TextDisabled("(stale)");
						else							ImGui::Text("%u", _task.matchedEntityCount);

						// 依存 : 数だけ出して中身はツールチップ
						ImGui::TableSetColumnIndex(7);
						ImGui::Text("%zu / %zu", _task.readNames.size(), _task.writeNames.size());
						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							Engine::Editor::Value("Read", "%s", _task.readNames.empty() ? "-" : JoinNames(_task.readNames).c_str());
							Engine::Editor::Value("Write", "%s", _task.writeNames.empty() ? "-" : JoinNames(_task.writeNames).c_str());
							ImGui::EndTooltip();
						}
					}
					ImGui::EndTable();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();

			_begin = _end;
		}

		ImGui::EndChild();
	}

	//======================================================================================
	// リソース
	//======================================================================================
	void ECSProfilerView::DrawResources(const ECS::ECSWorldSnapshot& a_snapshot)
	{
		Engine::Editor::HelpText("Size is sizeof(T). Heap memory owned by the resource is not included.");

		if (!ImGui::BeginTable("ResourceTable", 3, TABLE_FLAGS)) return;

		ImGui::TableSetupColumn("ID");
		ImGui::TableSetupColumn("Type");
		ImGui::TableSetupColumn("Size");
		ImGui::TableHeadersRow();

		for (const auto& _resource : a_snapshot.resources)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("%u", _resource.id);
			ImGui::TableSetColumnIndex(1); ImGui::Text("%s", _resource.name.c_str());
			ImGui::TableSetColumnIndex(2); ImGui::Text("%s", FormatBytes(_resource.size).c_str());
		}
		ImGui::EndTable();
	}
}
