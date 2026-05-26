#include "WatchView.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../MainEngine.h"
#include "../../Window/NativeWindow.h"

void Engine::Editor::WatchView::Init()
{}

void Engine::Editor::WatchView::Draw()
{
	if (ImGui::Begin("Engine Profiler"))
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
			DrawDescriptorHeapUsage(); // ディスクリプタヒープ
			ImGui::Separator();

			DrawRenderStats(); // DrawCall & Primitive
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 下部にこれまでの「関数ごとの詳細な計測結果（ソートテーブル）」を表示する
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
			_displayList.push_back({_pair.first,&m_watchVec[_pair.second]});
		}

		// 並べ替え
		std::sort(
			_displayList.begin(),_displayList.end(),
			[](const DisplayItem& a,const DisplayItem& b)
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

			for (const auto& item : _displayList)
			{
				ImGui::TableNextRow();

				// 1列目: 名前
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", item.name.c_str());

				// 2列目: 計測結果（Watch側のDrawResultにお任せ）
				ImGui::TableSetColumnIndex(1);
				item.watchPtr->DrawResult(item.name);
			}
			ImGui::EndTable();

		}

	
		

		ImGui::End();
	}
}

void Engine::Editor::WatchView::StartWatch(const std::string & a_name)
{
	auto _it = m_watchIndexMap.find(a_name);
	if (_it != m_watchIndexMap.end())
	{
		m_watchVec[_it->second].Start();
	}
	else
	{
		auto _size = m_watchVec.size();
		m_watchIndexMap[a_name] = _size;

		Watch _watch = {};
		m_watchVec.push_back(_watch);
		m_watchVec[_size].Start();
	}
}

void Engine::Editor::WatchView::EndWatch(const std::string & a_name)
{
	auto _it = m_watchIndexMap.find(a_name);
	if (_it != m_watchIndexMap.end())
	{
		m_watchVec[_it->second].End();
		return;
	}
	else
	{
		Editor::MainEditor::Instance().AddLog("登録されていない計測 : %s",a_name.c_str());
	}
}

void Engine::Editor::WatchView::DrawMemoryUsage()
{
	auto* _pWindow = MainEngine::Instance().RefNativeWindow();
	double _memUsed = _pWindow->GetMemoryUsage();

	// メモリ使用率
	// MBに変換して表示
	double _memInMB = _memUsed / (1024.0 * 1024.0);
	ImGui::Text("RAM Usage : %.2f MB",_memInMB);
}

void Engine::Editor::WatchView::DrawVRAMUsage()
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

void Engine::Editor::WatchView::DrawCoreTimings()
{}

void Engine::Editor::WatchView::DrawFPSAndDeltaTime()
{
	float _dt = Engine::MainEngine::Instance().GetDeltaTime();
	int _fps = Engine::MainEngine::Instance().GetFPS();
	ImGui::Text("FPS : %d",_fps);
	ImGui::Text("DeltaTime : %f",_dt);
}

void Engine::Editor::WatchView::DrawRenderStats()
{}

void Engine::Editor::WatchView::DrawDescriptorHeapUsage()
{}

