#include "ProfilerPanel.h"

#include "../../Profiler/Profiler.h"

#include "../../../MainEngine.h"
#include "../../../Graphics/GraphicEngine.h"
#include "../../../Graphics/Core/GraphicsDevice/GraphicsDevice.h"
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

		// 下部にこれまでの「スコープごとの詳細な計測結果（ソート済みテーブル）」を表示する
		DrawTimerTable(a_editContext.pProfiler);
	}

	//======================================================================================
	// スコープごとの計測結果
	//
	// ENGINE_PROFILE_SCOPE が投げてくるのは「名前と1回ぶんの時間」だけで、
	// 平均や最小最大の組み立てと並べ替えは Profiler が済ませている。
	// ここは受け取った順に並べるだけ
	//======================================================================================
	void ProfilerPanel::DrawTimerTable(Profiler* a_pProfiler)
	{
		ImGui::Text("CPU Detail Timings");
		ImGui::TextDisabled("ENGINE_PROFILE_SCOPE");

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

		const auto& _results = a_pProfiler->GetResults();
		if (_results.empty())
		{
			ImGui::TextDisabled("No scope has been measured yet.");
			return;
		}

		// 描画
		constexpr ImGuiTableFlags _tableFlags =
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

		if (ImGui::BeginTable("TimerTable", 7, _tableFlags))
		{
			ImGui::TableSetupColumn("Title");
			ImGui::TableSetupColumn("CPU(ms)");
			ImGui::TableSetupColumn("Avg(ms)");
			ImGui::TableSetupColumn("Min(ms)");
			ImGui::TableSetupColumn("Max(ms)");
			ImGui::TableSetupColumn("Calls");
			ImGui::TableSetupColumn("Total");
			ImGui::TableHeadersRow();

			for (const auto& _result : _results)
			{
				const ScopeTimer& _timer = _result.timer;

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", _result.name.c_str());

				// このフレームで通らなかったものは、直近の値を出しても嘘になるので伏せる
				ImGui::TableSetColumnIndex(1);
				if (_timer.callCount > 0)
				{
					ImGui::Text("%.3f", _timer.time);
				}
				else
				{
					ImGui::TextDisabled("-");
				}

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.3f", _timer.averageTime);

				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.3f", _timer.minTime);

				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%.3f", _timer.maxTime);

				// このフレームで通った回数 : ループの中で計っているものはここが伸びる
				ImGui::TableSetColumnIndex(5);
				ImGui::Text("%d", _timer.callCount);

				// リセット以降の総計測回数
				ImGui::TableSetColumnIndex(6);
				ImGui::Text("%d", _timer.totalCallCount);
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
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		auto* _pDevice = _pGE ? _pGE->RefGraphicsDevice() : nullptr;
		auto* _pAdapter = _pDevice ? _pDevice->RefAdapter() : nullptr;
		if (!_pAdapter) return;

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
