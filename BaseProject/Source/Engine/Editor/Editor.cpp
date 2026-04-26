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
		for (auto& [_name, _watch] : m_upWatchMap)
		{
			_watch->DrawResult(_name);
		}

		// ImGui描画実行
		m_upImGuiContext->End(a_pCmdList);
	}
	const EditorCamera* MainEditor::GetEditorCamera()
	{
		return m_upSceneView->GetEditorCamera();
	}
	void MainEditor::AddLog(const char* a_fmt, ...)
	{
		if (!m_isInit) return;
		if (!m_upLog) return;

		va_list _args;
		va_start(_args, a_fmt);
		m_upLog->AddLog(a_fmt);
		va_end(_args);
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
	void MainEditor::StartWatch(const std::string & a_name)
	{
		if (!m_isInit) return;
		auto _it = m_upWatchMap.find(a_name);
		if (_it != m_upWatchMap.end())
		{
			_it->second->Start();
		}
		else
		{
			m_upWatchMap[a_name] = std::make_unique<Watch>();
			m_upWatchMap[a_name]->Start();
		}
	}
	void MainEditor::EndWatch(const std::string & a_name)
	{
		if (!m_isInit) return;
		auto _it = m_upWatchMap.find(a_name);
		if (_it != m_upWatchMap.end())
		{
			_it->second->End();
			return;
		}
		assert(0 && "登録されていない計測です");
	}
}
