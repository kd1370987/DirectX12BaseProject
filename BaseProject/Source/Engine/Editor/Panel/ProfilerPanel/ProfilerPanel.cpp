#include "ProfilerPanel.h"

#include "../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../../MainEngine.h"
#include "../../../Window/NativeWindow.h"

namespace Engine::Editor
{
	//======================================================================================
	// パネル描画
	// ウィンドウのBegin/EndはPanelManagerが行うのでここでは触らない
	//======================================================================================
	void ProfilerPanel::OnDrawImGui(EditorContext& a_editContext)
	{
		// システム全体の統計情報
		if (ImGui::CollapsingHeader("System Statistics", ImGuiTreeNodeFlags_DefaultOpen))
		{
			DrawFPSAndDeltaTime();
			ImGui::Separator();

			DrawCoreTimings();
			ImGui::Separator();

			DrawMemoryUsage();
			DrawVRAMUsage();
			DrawDescriptorHeapUsage();	// ディスクリプタヒープ
			ImGui::Separator();

			DrawRenderStats();			// DrawCall & Primitive
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 下部にこれまでの「関数ごとの詳細な計測結果（ソートテーブル）」を表示する
		DrawWatchTable();
	}

	//======================================================================================
	// 計測開始
	//======================================================================================
	void ProfilerPanel::StartWatch(const std::string& a_name)
	{
		auto _it = m_watchIndexMap.find(a_name);
		if (_it != m_watchIndexMap.end())
		{
			m_watchVec[_it->second].Start();
		}
		else
		{
			auto _size = m_watchVec.size();
			m_watchIndexMap[a_name] = static_cast<UINT>(_size);

			Watch _watch = {};
			m_watchVec.push_back(_watch);
			m_watchVec[_size].Start();
		}
	}

	//======================================================================================
	// 計測終了
	//======================================================================================
	void ProfilerPanel::EndWatch(const std::string& a_name)
	{
		auto _it = m_watchIndexMap.find(a_name);
		if (_it != m_watchIndexMap.end())
		{
			m_watchVec[_it->second].End();
			return;
		}
		else
		{
			Editor::MainEditor::Instance().AddLog("登録されていない計測 : %s", a_name.c_str());
		}
	}

	//======================================================================================
	// 関数ごとの計測結果
	//======================================================================================
	void ProfilerPanel::DrawWatchTable()
	{
		ImGui::Text("CPU Detail Timings");

		// 全体でリセット
		if (ImGui::Button("Reset"))
		{
			for (auto& _watch : m_watchVec)
			{
				_watch.Reset();
			}
		}
		ImGui::Separator();

		// 描画用構造体
		struct DisplayItem
		{
			std::string name;
			Watch* watchPtr;
		};
		std::vector<DisplayItem> _displayList;
		_displayList.reserve(m_watchIndexMap.size());

		// マップから名前とWatchのポインタをペアにしてリストに詰める
		for (const auto& _pair : m_watchIndexMap)
		{
			_displayList.push_back({ _pair.first,&m_watchVec[_pair.second] });
		}

		// 並べ替え
		std::sort(
			_displayList.begin(), _displayList.end(),
			[](const DisplayItem& a, const DisplayItem& b)
			{
				return a.watchPtr->GetAvelage() > b.watchPtr->GetAvelage();
			}
		);

		// 描画
		if (ImGui::BeginTable("WatchTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Title");
			ImGui::TableSetupColumn("Result");
			ImGui::TableHeadersRow();

			for (const auto& _item : _displayList)
			{
				ImGui::TableNextRow();

				// 1列目: 名前
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", _item.name.c_str());

				// 2列目: 計測結果（Watch側のDrawResultにお任せ）
				ImGui::TableSetColumnIndex(1);
				_item.watchPtr->DrawResult(_item.name);
			}
			ImGui::EndTable();
		}
	}

	//======================================================================================
	// メモリ使用率(Ram)
	//======================================================================================
	void ProfilerPanel::DrawMemoryUsage()
	{
		auto* _pWindow = MainEngine::Instance().RefNativeWindow();
		double _memUsed = _pWindow->GetMemoryUsage();

		// メモリ使用率
		// MBに変換して表示
		double _memInMB = _memUsed / (1024.0 * 1024.0);
		ImGui::Text("RAM Usage : %.2f MB", _memInMB);
	}

	//======================================================================================
	// VRAM使用率
	//======================================================================================
	void ProfilerPanel::DrawVRAMUsage()
	{
		auto* _pAdapter = D3D12::D3D12Wrapper::Instance().GetDXGIAdapter();
		// IDXGIAdapter3にキャスト
		ComPtr<IDXGIAdapter3> _adapter3;
		if (SUCCEEDED(_pAdapter->QueryInterface(IID_PPV_ARGS(&_adapter3))))
		{
			// ローカルビデオメモリの使用状況を取得
			DXGI_QUERY_VIDEO_MEMORY_INFO _videoMemInfo;
			if (SUCCEEDED(_adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &_videoMemInfo)))
			{
				double _vramUsedMB = static_cast<double>(_videoMemInfo.CurrentUsage) / (1024.0f * 1024.0f);

				// OSがゲームに対して割り当てられている
				double _vramBudgetMB = static_cast<double>(_videoMemInfo.Budget) / (1024.0 * 1024.0);

				ImGui::Text("VRAM Usage : %.2f / %.2f MB", _vramUsedMB, _vramBudgetMB);
			}
		}
	}

	//======================================================================================
	// CPU時間 : GPU時間
	//======================================================================================
	void ProfilerPanel::DrawCoreTimings()
	{}

	//======================================================================================
	// FPS & デルタタイム
	//======================================================================================
	void ProfilerPanel::DrawFPSAndDeltaTime()
	{
		float _dt = Engine::MainEngine::Instance().GetDeltaTime();
		int _fps = Engine::MainEngine::Instance().GetFPS();
		ImGui::Text("FPS : %d", _fps);
		ImGui::Text("DeltaTime : %f", _dt);
	}

	//======================================================================================
	// DrawCall数 : 総プリミティブ数 : アイテム
	//======================================================================================
	void ProfilerPanel::DrawRenderStats()
	{}

	//======================================================================================
	// ディスクリプタヒープ使用率
	//======================================================================================
	void ProfilerPanel::DrawDescriptorHeapUsage()
	{}
}
