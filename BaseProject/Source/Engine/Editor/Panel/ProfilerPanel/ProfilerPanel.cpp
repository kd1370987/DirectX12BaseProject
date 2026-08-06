#include "ProfilerPanel.h"

#include "../../Profiler/Profiler.h"

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

		// 下部にこれまでの「関数ごとの詳細な計測結果（ソート済みテーブル）」を表示する
		DrawTimerTable(a_editContext.pProfiler);
	}

	//======================================================================================
	// 関数ごとの計測結果
	//
	// 並べ替えまで Profiler が済ませているので、ここは受け取った順に並べるだけ
	//======================================================================================
	void ProfilerPanel::DrawTimerTable(Profiler* a_pProfiler)
	{
		ImGui::Text("CPU Detail Timings");

		if (!a_pProfiler)
		{
			ImGui::TextDisabled("Profiler is not available.");
			return;
		}

		// 平均を取り直す間隔
		int _avelageRate = a_pProfiler->GetAvelageRate();
		if (ImGui::DragInt("Avelage Rate (frame)", &_avelageRate, 1.0f, 1, 600))
		{
			a_pProfiler->SetAvelageRate(_avelageRate);
		}

		// 全体でリセット
		if (ImGui::Button("Reset"))
		{
			a_pProfiler->ResetAll();
		}
		ImGui::Separator();

		// 描画
		constexpr ImGuiTableFlags _tableFlags =
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

		if (ImGui::BeginTable("TimerTable", 6, _tableFlags))
		{
			ImGui::TableSetupColumn("Title");
			ImGui::TableSetupColumn("Now(ms)");
			ImGui::TableSetupColumn("Avg(ms)");
			ImGui::TableSetupColumn("Min(ms)");
			ImGui::TableSetupColumn("Max(ms)");
			ImGui::TableSetupColumn("Count");
			ImGui::TableHeadersRow();

			for (const auto& _result : a_pProfiler->GetResults())
			{
				const Timer& _timer = _result.timer;

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", _result.name.c_str());

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.3f", _timer.time);

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.3f", _timer.averageTime);

				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.3f", _timer.minTime);

				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%.3f", _timer.maxTime);

				ImGui::TableSetColumnIndex(5);
				ImGui::Text("%d", _timer.totalCount);
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
