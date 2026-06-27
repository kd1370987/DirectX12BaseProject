#include "Editor.h"

#include "ImGui/ImGuiContext.h"
#include "ImGui/Log/Log.h"
#include "ImGui/Watch/Watch.h"

#include "ECSView/ECSView.h"
#include "AssetResourceView/AssetResourceView.h"
#include "RenderGraphResourceView/RenderGraphResourceView.h"
#include "SceneView/SceneView.h"
#include "SceneView/EditorCamera/EditorCamera.h"
#include "WatchView/WatchView.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../MainEngine.h"
#include "../Option/OptionManager.h"

#include "../Graphics/GraphicEngine.h"
#include "../Graphics/RenderContext/RenderContext.h"
#include "SceneManagerEditor/SceneManagerEditor.h"

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
		if (!m_upRenderGraphResourceView)
		{
			m_upRenderGraphResourceView = std::make_unique<RenderGraphResourceView>();
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
		if (!m_upSceneManagerEditor)
		{
			m_upSceneManagerEditor = std::make_unique<SceneManagerEditor>();
			m_upSceneManagerEditor->Init();
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
		m_upRenderGraphResourceView->Draw(a_widht,a_height);
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

		m_upSceneManagerEditor->Draw();

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
	void MainEditor::DrawLine(
		const DirectX::SimpleMath::Vector3& a_startPos,
		const DirectX::SimpleMath::Vector3& a_endPos, 
		const DirectX::SimpleMath::Color& a_color
	)
	{
		if (m_debugLineDataVec.size() >= m_debugLineDataCapacity)
		{
			ENGINE_LOG("これ以上のデバッグラインは描画できません");
			return;
		}

		// 方向と長さを求める
		DXSM::Vector3 _dir = a_endPos - a_startPos;
		float _length = _dir.Length();

		// 長さがゼロに近い場合は描画をスキップ
		if (_length < 0.0001f) return;

		// 正規化された方向ベクトル
		DXSM::Vector3 _dirNorm = _dir / _length;

		// 真上・真下を向いている時のジンバルロックを防ぐためのUpベクトル
		DXSM::Vector3 _up = (std::abs(_dirNorm.y) > 0.999f) ? DXSM::Vector3::UnitX : DXSM::Vector3::UnitY;

		// Z軸方向に伸びるようにスケール
		DXSM::Matrix _scaleMat = DXSM::Matrix::CreateScale(1.0f, 1.0f, _length);

		// 向きと位置を適用
		DXSM::Matrix _worldMat = DXSM::Matrix::CreateWorld(a_startPos, _dirNorm, _up);

		// データ作成
		Graphics::DebugLineData _data = {};
		_data.color = a_color;												// 色指定
		_data.shapeType = static_cast<UINT>(Graphics::EShapeType::Line);	// 形状指定
		_data.worldMat = (_scaleMat * _worldMat).Transpose();								// 行列合成

		// 配列に追加
		m_debugLineDataVec.push_back(_data);
	}
	void MainEditor::DrawBox(const DirectX::SimpleMath::Matrix & a_worldMat, const DirectX::SimpleMath::Color & a_color)
	{
		if (m_debugLineDataVec.size() >= m_debugLineDataCapacity)
		{
			ENGINE_LOG("これ以上のデバッグラインは描画できません");
			return;
		}

		// データ作成
		Graphics::DebugLineData _data = {};
		_data.color = a_color;
		_data.shapeType = static_cast<UINT>(Graphics::EShapeType::Box);
		_data.worldMat = a_worldMat.Transpose();
		m_debugLineDataVec.push_back(_data);
	}
	void MainEditor::DrawBox(const DirectX::BoundingBox& a_aabb, const DirectX::SimpleMath::Color& a_color)
	{
		// Extents は「中心からの半分の長さ」なので、全体サイズにするために 2倍 してスケールにする
		DirectX::SimpleMath::Vector3 _scale = DirectX::SimpleMath::Vector3(a_aabb.Extents) * 2.0f;

		DirectX::SimpleMath::Matrix _worldMat =
			DirectX::SimpleMath::Matrix::CreateScale(_scale) *
			DirectX::SimpleMath::Matrix::CreateTranslation(a_aabb.Center);

		// 既存の行列受け取り用DrawBoxへ委譲
		DrawBox(_worldMat, a_color);
	}
	void MainEditor::DrawBox(const DirectX::BoundingOrientedBox& a_obb, const DirectX::SimpleMath::Color& a_color)
	{
		// OBBは回転も持っているので、クォータニオンから回転行列を作成して挟む
		DirectX::SimpleMath::Vector3 _scale = DirectX::SimpleMath::Vector3(a_obb.Extents) * 2.0f;

		DirectX::SimpleMath::Matrix _worldMat =
			DirectX::SimpleMath::Matrix::CreateScale(_scale) *
			DirectX::SimpleMath::Matrix::CreateFromQuaternion(a_obb.Orientation) *
			DirectX::SimpleMath::Matrix::CreateTranslation(a_obb.Center);

		DrawBox(_worldMat, a_color);
	}
	void MainEditor::DrawCapsule(const DirectX::SimpleMath::Matrix & a_worldMat, const DirectX::SimpleMath::Color & a_color)
	{
		if (m_debugLineDataVec.size() >= m_debugLineDataCapacity)
		{
			ENGINE_LOG("これ以上のデバッグラインは描画できません");
			return;
		}

		// データ作成
		Graphics::DebugLineData _data = {};
		_data.color = a_color;
		_data.shapeType = static_cast<UINT>(Graphics::EShapeType::Capsule);
		_data.worldMat = a_worldMat.Transpose();
		m_debugLineDataVec.push_back(_data);
	}
	void MainEditor::DrawSphere(const DirectX::SimpleMath::Matrix & a_worldMat, const DirectX::SimpleMath::Color & a_color)
	{
		if (m_debugLineDataVec.size() >= m_debugLineDataCapacity)
		{
			ENGINE_LOG("これ以上のデバッグラインは描画できません");
			return;
		}

		// データ作成
		Graphics::DebugLineData _data = {};
		_data.color = a_color;
		_data.shapeType = static_cast<UINT>(Graphics::EShapeType::Sphere);
		_data.worldMat = a_worldMat.Transpose();
		m_debugLineDataVec.push_back(_data);
	}
	void MainEditor::DrawSphere(const DirectX::BoundingSphere& a_sphere, const DirectX::SimpleMath::Color& a_color)
	{
		// HLSL側のスフィアが直径1.0（半径0.5）で作られているため、
		// Radiusに合わせるために直径分のスケールをかける
		float _scale = a_sphere.Radius * 2.0f;

		DirectX::SimpleMath::Matrix _worldMat =
			DirectX::SimpleMath::Matrix::CreateScale(_scale) *
			DirectX::SimpleMath::Matrix::CreateTranslation(a_sphere.Center);

		DrawSphere(_worldMat, a_color);
	}
	void MainEditor::DrawRay(
		const DirectX::SimpleMath::Vector3 & a_startPos, 
		const DirectX::SimpleMath::Vector3& a_dir,
		float a_length, 
		bool a_isHit,
		const DirectX::SimpleMath::Color & a_color
	)
	{
		if (m_debugLineDataVec.size() >= m_debugLineDataCapacity)
		{
			ENGINE_LOG("これ以上のデバッグラインは描画できません");
			return;
		}

		auto _endPos = a_startPos + (a_dir * a_length);
		DrawLine(a_startPos,_endPos,a_color);

		if (a_isHit)
		{
			auto _mat = DXSM::Matrix::CreateTranslation(_endPos);
			DrawSphere(_mat, Color::RED);
		}
	}
	void MainEditor::ClearBuffer()
	{
		m_debugLineDataVec.clear();
		m_debugLineDataVec.reserve(m_debugLineDataCapacity);
	}
	const std::vector<Graphics::DebugLineData>& MainEditor::GetDebugLineDataVec() const
	{
		return m_debugLineDataVec;
	}
}
