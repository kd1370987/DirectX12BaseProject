#include "Editor.h"

#include "ImGui/ImGuiContext.h"
#include "Panel/LogPanel/LogPanel.h"

#include "Profiler/Profiler.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../MainEngine.h"
#include "../Option/OptionManager.h"

#include "../Graphics/GraphicEngine.h"
#include "../Graphics/RenderContext/RenderContext.h"

#include "../Scene/SceneManager/SceneManager.h"
#include "../ECS/World/World.h"

#include "Panel/PanelManager.h"
#include "EditorCamera/EditorCamera.h"
#include "EffectEditor/EffectEditor.h"

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
		// エディター用フリーカメラ
		if (!m_upEditorCamera)
		{
			m_upEditorCamera = std::make_unique<EditorCamera>();
			m_upEditorCamera->Init();
		}

		// エフェクト確認用のモーダル画面
		if (!m_upEffectEditor)
		{
			m_upEffectEditor = std::make_unique<EffectEditor>();
		}

		// プロファイラ
		// パネルより先に作る : パネルへは参照だけを渡す
		if (!m_upProfiler)
		{
			m_upProfiler = std::make_unique<Profiler>();
			m_upProfiler->Init();
		}

		// パネルの登録
		if (!m_upPanelManager)
		{
			m_upPanelManager = std::make_unique<PanelManager>();
			m_upPanelManager->Init(m_upEditorCamera.get(), m_upProfiler.get());
		}

		// ログパネルの参照を取得しておく。
		// ログの追加はここを経由するので、パネル登録より後で引くこと
		m_pLogPanel = m_upPanelManager->RefPanel<LogPanel>();

		m_editFuncVec.clear();

		Debug::SetLogCallback(
			[this](const char* a_msg)
			{
				if (!m_pLogPanel) return;
				m_pLogPanel->AddLogRow(a_msg);
			}
		);
		return true;
	}
	void MainEditor::Release()
	{
		Debug::SetLogCallback(nullptr);

		// パネル本体より先に参照を切っておく
		m_pLogPanel = nullptr;

		// プロファイラのGPUリソース(リードバックバッファ)を先に手放す
		if (m_upProfiler)
		{
			m_upProfiler->Release();
		}

		// エフェクトエディターが抱えている確認用ワールドを捨てる
		if (m_upEffectEditor)
		{
			m_upEffectEditor->Release();
		}

		m_upImGuiContext->Release();
	}
	void MainEditor::Update(float a_dt)
	{
		//===================================================================
		// デバッグプレイ中はエディターへマウスを渡さない
		//-------------------------------------------------------------------
		// あちらはエディターを出したまま遊ぶモードで、カーソルは毎フレーム
		// 画面中央へ固定されている。そのままだとカーソルが乗っている
		// パネルがずっとホバー扱いになり、撃つつもりの左クリックが
		// インスペクターのドラッグ欄を掴んで値を書き換えてしまう。
		//
		// キーボードは渡したままにしておく。抜けるための Ctrl+P を拾うのは
		// アプリ側(InputManager のシステム入力)だが、ImGui のショートカットも生かしておきたい。
		//
		// 毎フレーム入れ直しているのは、どのモードから来ても辻褄が合うようにするため
		//===================================================================
		if (ImGui::GetCurrentContext() != nullptr)
		{
			ImGuiIO& _io = ImGui::GetIO();
			if (MainEngine::Instance().GetMode() == EAppMode::DebugPlay)
			{
				_io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
			}
			else
			{
				_io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			}
		}

		//===================================================================
		// プレイモード中はエディター操作を一切受け付けない
		//-------------------------------------------------------------------
		// プレイ中は Draw を呼ばない = ImGui::NewFrame が回らないので、
		// ImGui の入力状態(押しているボタン・MouseDelta)は最後にエディターを
		// 描いたフレームのまま止まる。そこでフリーカメラを回し続けると、
		// 「右クリックを押したまま切り替えた」状態が凍って毎フレーム同じ回転量が
		// 入り続け、画面が回りっぱなしになる。
		//
		// さらに、ウィンドウメッセージは ImGui へ流れ続けるので、プレイ中の
		// マウス移動(カーソル中央固定のため毎フレーム発生する)が入力イベントの
		// 待ち行列に溜まる。ImGui は1フレームに処理できる量が決まっているため、
		// エディターへ戻った後もその再生に何フレームもかかり、
		// 「しばらく操作を受け付けない」状態になる。
		//
		// どちらもプレイ中に「触らない・溜めない」で断てるので、ここで捨てておく。
		//===================================================================
		if (MainEngine::Instance().GetMode() == EAppMode::Game)
		{
			if (m_upEditorCamera) m_upEditorCamera->CancelControl();

			// プレイ中に届いた入力イベントは毎フレーム捨てる(溜めない)
			if (ImGui::GetCurrentContext() != nullptr)
			{
				ImGui::GetIO().ClearEventsQueue();
			}
			return;
		}

		//===================================================================
		// デバッグプレイ中はフリーカメラを動かさない
		//-------------------------------------------------------------------
		// 映しているのはゲームのカメラ(ExecuteDrawCmd は Editor のときしか
		// 割り込まない)なので、ここで回しても画は変わらない。
		// それでも止めておくのは、右クリックがゲーム側では武器の引き金だから。
		// 撃つたびにフリーカメラが見えないところで回っていると、
		// 抜けた瞬間に明後日の方向を向いている。
		//
		// Game モードと違って入力イベントは捨てない。エディターは描き続けているので
		// 毎フレーム消化されており、溜まって遅れることが無いため
		//===================================================================
		if (MainEngine::Instance().GetMode() == EAppMode::DebugPlay)
		{
			if (m_upEditorCamera) m_upEditorCamera->CancelControl();
			return;
		}

		// モーダルな画面(エフェクトエディター)が出ている間は、そちらのカメラを回す。
		// あちらも同じ EditorCamera だが実体は別。シーンビュー側の位置を動かさないため、
		// ここでシーンビューのフリーカメラは止めておく
		if (IsModalActive())
		{
			if (m_upEditorCamera) m_upEditorCamera->CancelControl();
			if (m_upEffectEditor) m_upEffectEditor->UpdateCamera(a_dt);
			return;
		}

		// フリーカメラの更新。
		// ここは ExecuteDrawCmd より前に呼ばれるので、この結果がそのフレームの描画に間に合う。
		// 参照している ImGui の入力は前フレームの NewFrame 時点のもの
		// (シーンビューのホバー状態も同じく前フレーム基準なので、ずれは生じない)。
		if (m_upEditorCamera)
		{
			m_upEditorCamera->Update(a_dt);
		}
	}

	//======================================================================================
	// モーダルな画面が出ているか
	//======================================================================================
	bool MainEditor::IsModalActive() const
	{
		return m_upEffectEditor && m_upEffectEditor->IsOpen();
	}

	//======================================================================================
	// エディター側に残っている入力を捨てる
	//--------------------------------------------------------------------------------------
	// モードの切り替え時に呼ぶ。切り替えを跨いで押しっぱなし扱いが残らないようにする。
	//======================================================================================
	void MainEditor::ResetInput()
	{
		if (m_upEditorCamera) m_upEditorCamera->CancelControl();

		if (ImGui::GetCurrentContext() == nullptr) return;

		ImGuiIO& _io = ImGui::GetIO();

		_io.ClearEventsQueue();		// 未処理の入力イベント(切り替え前の操作)を捨てる
		_io.ClearInputKeys();		// 押しっぱなしのキーを離した扱いにする
		_io.ClearInputMouse();		// マウスのボタンと座標も同様
	}
	//======================================================================================
	// シーン切り替えの通知
	//--------------------------------------------------------------------------------------
	// 選択していたものは切り替え先には無い。次の描画が触りにいく前に捨てる
	//======================================================================================
	void MainEditor::OnSceneChanged()
	{
		if (!m_upPanelManager) return;

		m_upPanelManager->ClearSceneContext();
	}

	void MainEditor::Draw(D3D12::GraphicsCommandList * a_pCmdList)
	{
		// ImGui描画開始
		m_upImGuiContext->Begin();

		// パネルマネージャー(ログパネルもここで描画される)
		m_upPanelManager->OnDrawPanels();

		// モーダル画面はパネルの後に出す。
		// ImGui のポップアップは「開く要求を出したときのIDスタック」に紐づくので、
		// パネルの中(インスペクター)からではなくここで開くこと
		if (m_upEffectEditor)
		{
			m_upEffectEditor->OnDrawImGui();
		}

		// 各登録された関数を実行
		for (auto _func : m_editFuncVec)
		{
			if (!_func) continue;
			_func();
		}

		// ImGui描画実行
		m_upImGuiContext->End(a_pCmdList);
	}
	void MainEditor::AddLog(const char* a_fmt, ...)
	{
		// 初期化チェック
		if (!m_isInit || !m_pLogPanel) return;

		char buffer[2048];
		// フォーマットと引数の結合
		va_list args;
		va_start(args, a_fmt);
		vsnprintf(buffer, sizeof(buffer), a_fmt, args);
		va_end(args);

		// 解決済みの文字列なので、書式として解釈させないこと。
		// AddLog へ渡すと '%' を含むログ(パスや割合など)で
		// 存在しない可変引数を読みにいって落ちる
		m_pLogPanel->AddLogRow(buffer);
	}
	void MainEditor::AddLogVector(const float* a_data, const size_t& a_size)
	{
		if (!m_isInit) return;
		if (!m_pLogPanel) return;

		for (size_t _i = 0; _i < a_size; ++_i)
		{
			AddLog("%f ,",a_data[_i]);
		}
		AddLog("\n");
	}
	void MainEditor::AddLogMatrix(const std::string & a_name, const DirectX::XMFLOAT4X4 & a_mat)
	{
		if (!m_isInit) return;
		if (!m_pLogPanel) return;

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
		if (!m_isInit || !m_pLogPanel) return;

		char _buffer[2048];

		va_list _args;
		va_start(_args, a_fmt);

		// 受け取ったフォーマットと引数を結合
		vsnprintf(_buffer,sizeof(_buffer),a_fmt,_args);
		va_end(_args);

		std::string _warnStr = std::string("[WARNING] ") + _buffer;

		// ログ : 解決済みなので書式としては解釈させない
		if (m_pLogPanel) m_pLogPanel->AddLogRow(_warnStr.c_str());
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

		// 解決済みなので書式としては解釈させない
		if (m_isInit && m_pLogPanel) m_pLogPanel->AddLogRow(buffer);
		OutputDebugStringA(buffer);
	}
	void MainEditor::BeginProfileFrame()
	{
		if (!m_isInit || !m_upProfiler) return;
		m_upProfiler->BeginFrame();
	}
	void MainEditor::EndProfileFrame()
	{
		if (!m_isInit || !m_upProfiler) return;
		m_upProfiler->EndFrame();
	}
	void MainEditor::CollectGPUProfileResult()
	{
		if (!m_isInit || !m_upProfiler) return;
		m_upProfiler->CollectGPUResult();
	}
	void MainEditor::StartTimer(const std::string & a_name, D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!m_isInit || !m_upProfiler) return;
		m_upProfiler->StartTimer(a_name, a_pCmdList);
	}
	void MainEditor::StopTimer(const std::string & a_name, D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!m_isInit || !m_upProfiler) return;
		m_upProfiler->StopTimer(a_name, a_pCmdList);
	}
	void MainEditor::DrawLine(
		const DirectX::SimpleMath::Vector3& a_startPos,
		const DirectX::SimpleMath::Vector3& a_endPos, 
		const DirectX::SimpleMath::Color& a_color
	)
	{
		if (!CanPushDebugShape()) return;

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

		// 向きと位置を適用。
		// ラインメッシュはローカル +Z 方向に伸びているので、+Z を _dirNorm に向けたい。
		// ただし CreateWorld は右手系で、渡した forward を反転して Z 軸に入れる(zaxis = -forward)。
		// このエンジンは左手系なので、反転を打ち消すために -_dirNorm を渡す。
		DXSM::Matrix _worldMat = DXSM::Matrix::CreateWorld(a_startPos, -_dirNorm, _up);

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
		if (!CanPushDebugShape()) return;

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
		if (!CanPushDebugShape()) return;

		// データ作成
		Graphics::DebugLineData _data = {};
		_data.color = a_color;
		_data.shapeType = static_cast<UINT>(Graphics::EShapeType::Capsule);
		_data.worldMat = a_worldMat.Transpose();
		m_debugLineDataVec.push_back(_data);
	}
	void MainEditor::DrawSphere(const DirectX::SimpleMath::Matrix & a_worldMat, const DirectX::SimpleMath::Color & a_color)
	{
		if (!CanPushDebugShape()) return;

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
		if (!CanPushDebugShape()) return;

		auto _endPos = a_startPos + (a_dir * a_length);
		DrawLine(a_startPos,_endPos,a_color);

		if (a_isHit)
		{
			auto _mat = DXSM::Matrix::CreateTranslation(_endPos);
			DrawSphere(_mat, Color::RED);
		}
	}
	bool MainEditor::CanPushDebugShape()
	{
		// オプションで切られていれば1本も積まない。
		// 空のままなら RenderContext::DrawShape も早期リターンするので、
		// 描画コマンドごと止まる
		if (!Option::OptionManager::GetInstance().GetDebugDrawOption().drawWire) return false;

		if (m_debugLineDataVec.size() >= m_debugLineDataCapacity)
		{
			ENGINE_LOG("これ以上のデバッグラインは描画できません");
			return false;
		}

		return true;
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
	void MainEditor::RegisterEditFunc(std::function<void()> a_func)
	{
		m_editFuncVec.push_back(a_func);
	}
}
