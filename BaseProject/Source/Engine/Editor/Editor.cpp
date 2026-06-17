#include "Editor.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "ImGui/ImGuiContext.h"
#include "ImGui/Log/Log.h"
#include "ImGui/Watch/Watch.h"

#include "ECSView/ECSView.h"
#include "AssetResourceView/AssetResourceView.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "SceneView/SceneView.h"
#include "SceneView/EditorCamera/EditorCamera.h"

#include "WatchView/WatchView.h"

#include "../Option/OptionManager.h"

namespace Engine::Editor
{

	MainEditor::MainEditor()
	{}
	MainEditor::~MainEditor()
	{}


	bool MainEditor::Init(HWND a_hwnd)
	{
		if (m_isInit) return true;

		m_isInit = true;

		// ImGui関連
		if (!m_upImGuiContext)
		{
			m_upImGuiContext = std::make_unique<ImGuiContext>();
			m_upImGuiContext->Init(a_hwnd);
		}
		// ログ
		if (!m_upLog)
		{
			m_upLog = std::make_unique<Log>();
			m_upLog->Init();
		}
		// ECSビュー
		if (!m_upECSView)
		{
			m_upECSView = std::make_unique<ECSView>();
			m_upECSView->Init();
		}
		// アセットビュー
		if (!m_upAssetResourceView)
		{
			m_upAssetResourceView = std::make_unique<AssetResourceView>();
			m_upAssetResourceView->Init();
		}
		if (!m_upSceneView)
		{
			m_upSceneView = std::make_unique<SceneView>();
			m_upSceneView->Init();
		}
		if (!m_upWatchView)
		{
			m_upWatchView = std::make_unique<WatchView>();
			m_upWatchView->Init();
		}

		return true;
	}
	void MainEditor::Release()
	{
		m_upImGuiContext->Release();
	}
	void MainEditor::Draw(ID3D12GraphicsCommandList * a_pCmdList, UINT a_widht, UINT a_height)
	{
		// ImGui描画開始
		m_upImGuiContext->Begin(a_widht, a_height);

		// ECSビュー
		m_upECSView->Draw(a_widht, a_height);
		// アセットビュー
		m_upAssetResourceView->Draw(a_widht, a_height);

		// ログ表示
		m_upLog->Draw("Log");
		// 計測表示
		m_upWatchView->Draw();

		if (ImGui::Begin("Options"))
		{
			auto& _opManager = Option::OptionManager::GetInstance();
			_opManager.DrawEdit();
		}
		ImGui::End();

		// ImGui描画実行
		m_upImGuiContext->End(a_pCmdList);
	}
	const EditorCamera* MainEditor::GetEditorCamera()
	{
		return m_upSceneView->GetEditorCamera();
	}
	void MainEditor::AddLog(const char* a_fmt, ...)
	{
		// 初期化チェック
		if (!m_isInit || !m_upLog) return;

		char buffer[2048];
		// フォーマットと引数の結合
		va_list args;
		va_start(args, a_fmt);
		vsnprintf(buffer, sizeof(buffer), a_fmt, args);
		va_end(args);

		m_upLog->AddLog(buffer);
	}
	void MainEditor::AddLogVector(const float* a_data, const size_t& a_size)
	{
		if (!m_isInit) return;
		if (!m_upLog) return;

		for (size_t _i = 0; _i < a_size; ++_i)
		{
			AddLog("%f ,",a_data[_i]);
		}
		AddLog("\n");
	}
	void MainEditor::AddLogMatrix(const std::string & a_name, const DirectX::XMFLOAT4X4 & a_mat)
	{
		if (!m_isInit) return;
		if (!m_upLog) return;

		AddLog("MatrixName : %s\n", a_name.c_str());

		for (size_t _row = 0; _row < 4; ++_row)
		{
			for (size_t _col = 0; _col < 4; ++_col)
			{
				AddLog("%f ", a_mat.m[_row][_col]);
			}
			AddLog("\n");
		}
	}
	void MainEditor::WarningLog(const char* a_fmt, ...)
	{
		// 初期化済みチェック
		if (!m_isInit || !m_upLog) return;

		char _buffer[2048];

		va_list _args;
		va_start(_args, a_fmt);

		// 受け取ったフォーマットと引数を結合
		vsnprintf(_buffer,sizeof(_buffer),a_fmt,_args);
		va_end(_args);

		std::string _warnStr = std::string("[WARNING] ") + _buffer;

		// ログ
		AddLog(_warnStr.c_str());
		// visualスタジオ側にも一応出力
		OutputDebugStringA((_warnStr + "\n").c_str());
	}
	void MainEditor::ErrorLog(const char* a_fmt, ...)
	{
		char buffer[2048];

		va_list args;
		va_start(args, a_fmt);

		vsnprintf(buffer, sizeof(buffer), a_fmt, args);

		va_end(args);

#ifdef _DEBUG
		//assert(false && buffer);
#endif

		AddLog(buffer);
		OutputDebugStringA(buffer);
	}
	void MainEditor::StartWatch(const std::string & a_name)
	{
		if (!m_isInit) return;
		m_upWatchView->StartWatch(a_name);
	}
	void MainEditor::EndWatch(const std::string & a_name)
	{
		if (!m_isInit) return;
		m_upWatchView->EndWatch(a_name);
	}
}
