#include "ProfilerPanel.h"

#include "../../Profiler/Profiler.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Device/GraphicsDevice/GraphicsDevice.h"
#include "Engine/Window/NativeWindow.h"
#include "Engine/Graphics/Particle/ParticleBufferManager.h"
#include "Engine/Graphics/Particle/GPU/EmitterSlotPool/EmitterSlotPool.h"
#include "Engine/Graphics/Particle/GPU/GPUParticlePool/GPUParticlePool.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Editor
{
	//======================================================================================
	// パネル描画
	// ウィンドウのBegin/EndはPanelManagerが行うのでここでは触らない
	//======================================================================================
	void ProfilerPanel::OnDrawImGui(EditorContext& a_editContext)
	{
		DrawMenuBar();

		switch (m_eView)
		{
		case EView::Engine:	DrawEngineView(a_editContext);	break;
		case EView::ECS:	m_ecsView.Draw(a_editContext.pServices ? a_editContext.pServices->pSceneManager : nullptr);	break;
		case EView::Thread:	m_threadView.Draw();			break;
		case EView::Graphics:	m_graphicsView.Draw();		break;
		}
	}

	//======================================================================================
	// メニューバー : 表示の切り替え
	//======================================================================================
	void ProfilerPanel::DrawMenuBar()
	{
		if (!ImGui::BeginMenuBar()) return;

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Engine", nullptr, m_eView == EView::Engine)) m_eView = EView::Engine;
			if (ImGui::MenuItem("ECS", nullptr, m_eView == EView::ECS)) m_eView = EView::ECS;
			if (ImGui::MenuItem("Thread", nullptr, m_eView == EView::Thread)) m_eView = EView::Thread;
			if (ImGui::MenuItem("Graphics", nullptr, m_eView == EView::Graphics)) m_eView = EView::Graphics;
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	//======================================================================================
	// エンジン全体の表示
	//======================================================================================
	void ProfilerPanel::DrawEngineView(EditorContext& a_editContext)
	{
		// システム全体の統計情報
		if (ImGui::CollapsingHeader("System Statistics", ImGuiTreeNodeFlags_DefaultOpen))
		{
			DrawFPSAndDeltaTime();
			Engine::EditorField::Line();

			DrawCoreTimings();
			Engine::EditorField::Line();

			DrawMemoryUsage();
			DrawVRAMUsage();
			DrawDescriptorHeapUsage();	// ディスクリプタヒープ
			Engine::EditorField::Line();

			DrawRenderStats();			// DrawCall & Primitive
		}

		// GPUパーティクル
		if (ImGui::CollapsingHeader("Particles"))
		{
			DrawParticleStats();
		}

		Engine::EditorField::Line();

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
		Engine::EditorField::Header("CPU Detail Timings");
		Engine::EditorField::HelpText("ENGINE_PROFILE_SCOPE");

		if (!a_pProfiler)
		{
			Engine::EditorField::HelpText("Profiler is not available.");
			return;
		}

		// 平均を取り直す間隔
		int _avelageRate = a_pProfiler->GetAvelageRate();
		if (Engine::EditorField::Field("Avelage Rate (frame)", _avelageRate, 1.0f, 1, 600))
		{
			a_pProfiler->SetAvelageRate(_avelageRate);
		}

		// 全体でリセット
		if (ImGui::Button("Reset"))
		{
			a_pProfiler->ResetAll();
		}
		Engine::EditorField::Line();

		const auto& _results = a_pProfiler->GetResults();
		if (_results.empty())
		{
			Engine::EditorField::HelpText("No scope has been measured yet.");
			return;
		}

		// 描画
		constexpr ImGuiTableFlags TABLE_FLAGS =
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

		if (ImGui::BeginTable("TimerTable", 7, TABLE_FLAGS))
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
		Engine::EditorField::Value("RAM Usage", "%.2f MB", _memInMB);
	}

	//======================================================================================
	// VRAM使用率
	//======================================================================================
	void ProfilerPanel::DrawVRAMUsage()
	{
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		auto* _pDevice = _pGE ? _pGE->RefRenderDevice()->RefGraphicsDevice() : nullptr;
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

				Engine::EditorField::Value("VRAM Usage", "%.2f / %.2f MB", _vramUsedMB, _vramBudgetMB);
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
		Engine::EditorField::Value("FPS", "%d", _fps);
		Engine::EditorField::Value("DeltaTime", "%f", _dt);
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

	//======================================================================================
	// GPUパーティクル
	//
	// 発生源の席 : 使用中・返却待ちが増え続けていないか(シーンを読み直しても一定か)を見る。
	//              増え続けるなら、どこかで席を返し忘れている。
	// プール     : アセットごとの容量と、準備が済んでいるか、命令があふれたことがあるか
	//======================================================================================
	void ProfilerPanel::DrawParticleStats()
	{
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		auto* _pPM = _pGE ? _pGE->RefParticleManager() : nullptr;
		if (!_pPM)
		{
			Engine::EditorField::HelpText("(パーティクルマネージャーがありません)");
			return;
		}

		//----------------------------------------------------------------------------------
		// 発生源の席
		//----------------------------------------------------------------------------------
		if (const auto* _pSlots = _pPM->GetEmitterSlotPool())
		{
			Engine::EditorField::Text("Emitter Slots");

			// 席 0(単位行列)は予約で常に1つ使っているので、持ち主の数からは外す
			const uint32_t _live = _pSlots->GetLiveCount();
			const uint32_t _pending = _pSlots->GetPendingCount();
			const uint32_t _owned = (_live > _pending + 1) ? (_live - _pending - 1) : 0;

			Engine::EditorField::Value("In Use", "%u", _owned);
			Engine::EditorField::Tooltip("持ち主のエフェクトがいる席(席0を除く)");

			Engine::EditorField::Value("Pending Return", "%u", _pending);
			Engine::EditorField::Tooltip("持ち主は消えたが、出した粒が消えきるまで待っている席");

			Engine::EditorField::Value("Transfer Range", "%u", _pSlots->GetUsedCount());
			Engine::EditorField::Tooltip("毎フレームGPUへ送っている席の数(配ったことのある最大の席番号 + 1)");

			Engine::EditorField::Value("Capacity CPU / GPU", "%u / %u  (block %u)",
				_pSlots->GetCPUCapacity(), _pSlots->GetGPUCapacity(), _pSlots->GetBlockSize());
			Engine::EditorField::Tooltip("足りなくなると block 単位で伸びる。GPU側はそのとき作り直す");

			Engine::EditorField::Value("GPU Size", "%.1f KB",
				static_cast<double>(_pSlots->GetGPUCapacity()) * sizeof(Graphics::Particle::EmitterTransform) / 1024.0);
		}
		else
		{
			Engine::EditorField::HelpText("(発生源の席がまだ作られていません)");
		}

		Engine::EditorField::Line();

		//----------------------------------------------------------------------------------
		// プール一覧
		//
		// マップの並びは毎回変わりうるので、名前で並べてから出す
		//----------------------------------------------------------------------------------
		struct PoolRow
		{
			std::string name;
			UINT capacity = 0;
			size_t requests = 0;
			bool isReady = false;
			bool isAwake = false;
			bool isOverflowed = false;
			bool isGrowPending = false;		// 伸ばす予定がある(次のシミュレーションで伸びる)
			bool isHitLimit = false;		// 上限に届いたことがある
			uint64_t estimated = 0;			// 直近の最大寿命の間に出した数(生きている数の上限)
			uint32_t growCount = 0;			// 伸ばした回数
			int sortOrder = 0;
			double sinceEmit = -1.0;
			bool isLocal = false;
			bool isAlphaBlend = false;
		};

		const auto* _pRM = _pGE->RefResourceManager();

		std::vector<PoolRow> _rows;
		_rows.reserve(_pPM->GetPoolMap().size());

		size_t _totalCapacity = 0;
		size_t _readyCount = 0;
		size_t _awakeCount = 0;		// 更新と描画を回しているプール
		size_t _awakeCapacity = 0;	// そのぶんの容量(更新と描画が実際に走る粒の数)
		for (const auto& [_handle, _upPool] : _pPM->GetPoolMap())
		{
			PoolRow _row = {};

			const auto* _pAsset = _pRM ? _pRM->Get(_handle) : nullptr;
			_row.name = _pAsset ? _pAsset->GetName() : "(不明)";
			_row.capacity = _upPool ? _upPool->GetMaxCapacity() : 0;
			_row.requests = _pPM->GetRequests(_handle).size();
			_row.isReady = _pPM->IsReady(_handle);
			_row.isAwake = _pPM->IsAwake(_handle);
			_row.isOverflowed = _pPM->HasOverflowed(_handle);
			_row.isGrowPending = _pPM->HasGrowTarget(_handle);
			_row.isHitLimit = _pPM->HasHitHardLimit(_handle);
			_row.estimated = _pPM->GetEstimatedLive(_handle);
			_row.growCount = _pPM->GetGrowCount(_handle);
			_row.sortOrder = _pAsset ? _pAsset->GetSortOrder() : 0;
			_row.sinceEmit = _pPM->GetSecondsSinceLastEmit(_handle);
			_row.isLocal = _pAsset && _pAsset->IsLocalSpace();
			_row.isAlphaBlend = _pAsset && (_pAsset->GetBlendMode() == Graphics::Particle::EParticleBlendMode::AlphaBlend);

			_totalCapacity += _row.capacity;
			if (_row.isReady) ++_readyCount;
			if (_row.isReady && _row.isAwake)
			{
				++_awakeCount;
				_awakeCapacity += _row.capacity;
			}

			_rows.push_back(std::move(_row));
		}

		std::sort(_rows.begin(), _rows.end(),
			[](const PoolRow& a_l, const PoolRow& a_r) { return a_l.name < a_r.name; });

		// 粒本体 + デッドリスト。命令バッファとカウンターは小さいので数えない
		constexpr size_t BYTES_PER_PARTICLE = sizeof(Graphics::Particle::ParticleData) + sizeof(uint32_t);

		Engine::EditorField::Value("Pools", "%u  (Ready %u / Loading %u)",
			static_cast<unsigned>(_rows.size()),
			static_cast<unsigned>(_readyCount),
			static_cast<unsigned>(_rows.size() - _readyCount));
		Engine::EditorField::Value("Total Capacity", "%u particles  (%.1f MB)",
			static_cast<unsigned>(_totalCapacity),
			static_cast<double>(_totalCapacity * BYTES_PER_PARTICLE) / (1024.0 * 1024.0));
		Engine::EditorField::Value("Awake", "%u pools  (%u particles)",
			static_cast<unsigned>(_awakeCount),
			static_cast<unsigned>(_awakeCapacity));
		Engine::EditorField::Tooltip(
			"更新と描画を回しているプール。更新はまだ粒の数ではなく、この容量ぶん走っている"
			"(描画は生きている粒の数ぶんだけ)。最後に出してから最大寿命が経ったプールは眠って(飛ばされて)いる");

		// 発生命令のバッファ(全プール共通の1本)。足りなくなると2のべき乗で伸びる
		Engine::EditorField::Value("Emit Buffer", "%u / %u requests  (max %u)",
			static_cast<unsigned>(_pPM->GetFrameEmitCount()),
			static_cast<unsigned>(_pPM->GetEmitBufferCapacity()),
			static_cast<unsigned>(Graphics::Particle::EMIT_BUFFER_MAX_CAPACITY));
		Engine::EditorField::Tooltip("このフレームに送った発生命令の数 / 命令バッファの容量。全プール共通の1本");

		if (_rows.empty()) return;

		constexpr ImGuiTableFlags TABLE_FLAGS =
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

		if (ImGui::BeginTable("ParticlePoolTable", 10, TABLE_FLAGS))
		{
			ImGui::TableSetupColumn("Asset");
			ImGui::TableSetupColumn("Capacity");
			ImGui::TableSetupColumn("Estimated");
			ImGui::TableSetupColumn("Grows");
			ImGui::TableSetupColumn("Space");
			ImGui::TableSetupColumn("Blend");
			ImGui::TableSetupColumn("Order");
			ImGui::TableSetupColumn("Requests");
			ImGui::TableSetupColumn("Last Emit");
			ImGui::TableSetupColumn("State");
			ImGui::TableHeadersRow();

			for (const auto& _row : _rows)
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", _row.name.c_str());

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%u", _row.capacity);

				// 生きている数の上限の見積もり(直近の最大寿命の間に出した数)。容量に近いほど伸びやすい
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%llu", static_cast<unsigned long long>(_row.estimated));

				// 伸ばした回数。上限に届いたことがあれば色を変える
				ImGui::TableSetColumnIndex(3);
				if (_row.isHitLimit)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%u (limit)", _row.growCount);
				}
				else
				{
					ImGui::Text("%u", _row.growCount);
				}

				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%s", _row.isLocal ? "Local" : "World");

				ImGui::TableSetColumnIndex(5);
				ImGui::Text("%s", _row.isAlphaBlend ? "Alpha" : "Add");

				ImGui::TableSetColumnIndex(6);
				ImGui::Text("%d", _row.sortOrder);

				// 表示した時点で積まれている命令の数(フレームのどこで描くかで 0 にもなる)。
				// 準備中のプールで上限に届いていたら、溜めきれずに捨てている
				ImGui::TableSetColumnIndex(7);
				if (!_row.isReady && _row.requests >= Graphics::Particle::EMIT_PENDING_REQUEST_MAX)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%u", static_cast<unsigned>(_row.requests));
				}
				else
				{
					ImGui::Text("%u", static_cast<unsigned>(_row.requests));
				}

				// 最後に粒を出してからの秒数
				ImGui::TableSetColumnIndex(8);
				if (_row.sinceEmit < 0.0)
				{
					ImGui::TextDisabled("-");
				}
				else
				{
					ImGui::Text("%.1f s", _row.sinceEmit);
				}

				ImGui::TableSetColumnIndex(9);
				if (!_row.isReady)
				{
					ImGui::TextDisabled("Loading");
				}
				else if (_row.isGrowPending)
				{
					// 足りないと見て、次のシミュレーションで伸ばす
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Growing");
				}
				else if (!_row.isAwake)
				{
					ImGui::TextDisabled("Sleep");
				}
				else if (_row.isOverflowed)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Overflowed");
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("1フレームの発生命令が全体の上限(%u 件)を超えて、捨てたことがある", static_cast<unsigned>(Graphics::Particle::EMIT_BUFFER_MAX_CAPACITY));
					}
				}
				else
				{
					ImGui::Text("Ready");
				}
			}
			ImGui::EndTable();
		}
	}
}
