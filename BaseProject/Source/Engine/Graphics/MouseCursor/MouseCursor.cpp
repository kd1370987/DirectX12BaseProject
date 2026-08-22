#include "MouseCursor.h"

#include "../GraphicEngine.h"
#include "../../MainEngine.h"
#include "../../Window/NativeWindow.h"
#include "../../Input/InputManager/InputManager.h"
#include "../../Option/OptionManager.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../Resource/Data/Texture/Texture.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Graphics
{
	void MouseCursor::Init()
	{
		// 実際の読み込み要求は Update で出す。
		// 設定はエディターから触れるので、初回だけでなく「変わったら読み直す」形に
		// 寄せておいたほうが分岐が一箇所で済む
		m_texRef = {};
		m_loadedGUID = {};
		m_isHideOSCursor = false;
		m_isDraw = false;
	}

	void MouseCursor::Release()
	{
		// ResourceRef のデストラクタが参照を返すので、空を入れて手放す
		m_texRef = {};
		m_loadedGUID = {};
		m_isHideOSCursor = false;
		m_isDraw = false;
	}

	//======================================================================================
	// 毎フレームの更新
	//======================================================================================
	void MouseCursor::Update()
	{
		m_isHideOSCursor = false;
		m_isDraw = false;

		const auto& _cursorOp = Option::OptionManager::GetInstance().GetCursorOption();

		// 切られている / 画像が未設定なら、OSのカーソルをそのまま出す
		if (!_cursorOp.isEnable || !_cursorOp.textureGUID.IsValid())
		{
			if (m_loadedGUID.IsValid())
			{
				m_texRef = {};
				m_loadedGUID = {};
			}
			return;
		}

		// 設定が差し替わっていたら読み直す
		if (!(m_loadedGUID == _cursorOp.textureGUID))
		{
			m_texRef = Resource::ResourceManager::Instance()
				.RequestLoad<Resource::Texture>(_cursorOp.textureGUID);
			m_loadedGUID = _cursorOp.textureGUID;
		}

		// 読み込みが終わるまではOSのカーソルを消さない。
		// 消してから絵が出るまでの間、カーソルが1つも無い状態になってしまうため
		if (!Resource::ResourceManager::Instance().IsReady(m_texRef)) return;

		// ここまで来たら自前の絵を出せる
		m_isHideOSCursor = true;

		// 視点操作でカーソルを画面中央へ固定している間は絵を描かない。
		// 毎フレーム中央へ戻されるので位置に意味が無く、画面中央に矢印が
		// 貼り付いて見えるだけになる。OSのカーソルは消したままにしておく
		if (Input::InputManager::Instance().IsCursorLockActive()) return;

		// クライアント領域の外に出ているならこちらで描くものは無い
		// (OSのカーソルはそのウィンドウの上でしか消えないので、外は元から普通に出る)
		if (!TryGetCursorClientPos(m_clientPos)) return;

		m_isDraw = true;
	}

	//======================================================================================
	// カーソルのクライアント座標を取る
	//======================================================================================
	bool MouseCursor::TryGetCursorClientPos(Math::Vector2& a_outClientPos) const
	{
		Math::Vector2 _clientPos = {};
		if (!Input::InputManager::Instance().GetCursorClientPos(_clientPos)) return false;

		const auto* _pWindow = MainEngine::Instance().GetNativeWindow();
		if (!_pWindow) return false;

		const float _clientW = static_cast<float>(_pWindow->GetClientWidth());
		const float _clientH = static_cast<float>(_pWindow->GetClientHeight());

		// 最小化中は0になる
		if (_clientW <= 0.0f || _clientH <= 0.0f) return false;

		// クライアント領域の外(ウィンドウの枠や他のウィンドウの上)は範囲外
		if (_clientPos.x < 0.0f || _clientPos.x >= _clientW) return false;
		if (_clientPos.y < 0.0f || _clientPos.y >= _clientH) return false;

		a_outClientPos = _clientPos;
		return true;
	}

	//======================================================================================
	// ゲーム画面へ描く
	//--------------------------------------------------------------------------------------
	// UIパスは深度を切ってあるので、積んだ順がそのまま前後になる。
	// 呼び出し元がUIを全部積み終えた後に呼ぶことで最前面になる。
	//======================================================================================
	void MouseCursor::SubmitUI(GraphicsEngine* a_pGraphicsEngine) const
	{
		if (!m_isDraw || !a_pGraphicsEngine) return;

		const auto& _cursorOp = Option::OptionManager::GetInstance().GetCursorOption();

		// クライアント領域(実際のウィンドウの大きさ) → 描画解像度(UIの座標系)。
		// バックバッファは描画解像度で作られ、クライアント領域へ引き伸ばして
		// 表示されるので、比率を掛ければよい
		// (UIの当たり判定でも同じ変換をしている : UIBase::CalcCursorUIPos)
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const float _renderW = static_cast<float>(_winOp.windowWidth);
		const float _renderH = static_cast<float>(_winOp.windowHeight);
		if (_renderW <= 0.0f || _renderH <= 0.0f) return;

		const auto* _pWindow = MainEngine::Instance().GetNativeWindow();
		if (!_pWindow) return;

		const float _clientW = static_cast<float>(_pWindow->GetClientWidth());
		const float _clientH = static_cast<float>(_pWindow->GetClientHeight());
		if (_clientW <= 0.0f || _clientH <= 0.0f) return;

		const Math::Vector2 _renderPos = {
			m_clientPos.x * (_renderW / _clientW),
			m_clientPos.y * (_renderH / _clientH)
		};

		// ホットスポットをピボットに渡す。
		// SubmitUI はピボットの位置が指定座標に来るように描くので、
		// 画像の中の尖端がそのままカーソル位置に重なる
		a_pGraphicsEngine->SubmitUI(
			m_texRef,
			_renderPos,
			Math::Vector2(_cursorOp.sizePixel, _cursorOp.sizePixel),
			_cursorOp.color,
			0.0f,
			0.0f,
			Math::Vector2(0.0f, 0.0f),
			_cursorOp.hotspot
		);
	}

	//======================================================================================
	// エディター画面へ描く
	//--------------------------------------------------------------------------------------
	// パネルより手前に出す必要があるので、最前面レイヤーへ直接積む。
	// (ゲームのUIパスへ積んでも、その絵はシーンビューのパネルの中身になってしまい、
	//  画面上の位置とカーソルの位置が合わない)
	//======================================================================================
	void MouseCursor::DrawImGui() const
	{
		// OSのカーソルを消している間は、ImGuiにも「カーソルは出さない」と伝える。
		//
		// ImGuiのWin32バックエンドは、求めるカーソルの形が前フレームから変わると
		// ::SetCursor を呼び直す(NewFrame の中)。黙っているとパネルの端をまたいで
		// リサイズ矢印を求めた瞬間などにOSのカーソルが戻ってきてしまう。
		// None を渡しておけば、あちらも ::SetCursor(nullptr) を呼ぶ側に回る。
		//
		// 絵を描かないフレーム(視点操作でカーソルを中央固定している間)でも
		// 消したままにしたいので、m_isDraw ではなく m_isHideOSCursor で見る。
		if (m_isHideOSCursor)
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
		}

		if (!m_isDraw) return;

		auto& _resMgr = Resource::ResourceManager::Instance();
		if (!_resMgr.IsReady(m_texRef)) return;

		auto* _pTex = _resMgr.Get(m_texRef);
		if (!_pTex) return;

		const auto& _cursorOp = Option::OptionManager::GetInstance().GetCursorOption();

		const ImGuiIO& _io = ImGui::GetIO();

		// 設定の大きさは描画解像度基準。ImGuiはクライアント領域基準で描くので、
		// バックバッファとクライアント領域の比(DisplayFramebufferScale)で割って合わせる。
		// こうしないとウィンドウの大きさによって見た目の大きさが変わってしまう
		const float _scale = (_io.DisplayFramebufferScale.x > 0.0f) ? _io.DisplayFramebufferScale.x : 1.0f;
		const float _size = _cursorOp.sizePixel / _scale;

		// ImGuiのマウス座標はクライアント領域基準なのでそのまま使える
		const ImVec2 _min = {
			_io.MousePos.x - _cursorOp.hotspot.x * _size,
			_io.MousePos.y - _cursorOp.hotspot.y * _size
		};
		const ImVec2 _max = { _min.x + _size, _min.y + _size };

		const auto _gpuHandle = D3D12::DescriptorHeapManager::Instance()
			.GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());

		ImGui::GetForegroundDrawList()->AddImage(
			(ImTextureID)(_gpuHandle.ptr),
			_min,
			_max,
			ImVec2(0.0f, 0.0f),
			ImVec2(1.0f, 1.0f),
			ImGui::ColorConvertFloat4ToU32(
				ImVec4(_cursorOp.color.r, _cursorOp.color.g, _cursorOp.color.b, _cursorOp.color.a))
		);
	}
}
