#include "UIBase.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"
#include "Engine/Option/OptionManager.h"	// ウィンドウ解像度(px)取得用
#include "Engine/Input/InputManager/InputManager.h"	// カーソル位置の取得用
#include "Engine/Window/NativeWindow.h"				// クライアント領域の実サイズ取得用

#include "../../../Engine/Editor/Helper/EditorHelper.h"

namespace App::Object
{
	void UIBase::Release(Engine::GameObject::ObjectContext& a_context)
	{}

	//======================================================================================
	// 更新 : 飾りのアニメーションを進める
	//--------------------------------------------------------------------------------------
	// 出していないフレームも進める。止めてしまうと、
	// 出した瞬間に前回止まったところから続いてしまう。
	//
	// 継承先で Update を持つ場合は、先頭で UIBase::Update を呼ぶこと
	//======================================================================================
	void UIBase::Update(Engine::GameObject::ObjectContext& a_context)
	{
		for (Decoration::Decoration& _decoration : m_decorationVec)
		{
			Decoration::AdvanceAnimation(_decoration, a_context.dt);
		}
	}

	void UIBase::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		// 出さない指示が出ているものは描かない
		if (!m_isVisible) return;

		DrawDecorations(a_context);
	}

	//======================================================================================
	// 飾りの操作
	//======================================================================================
	Decoration::Decoration& UIBase::AddDecoration(Decoration::EDecorationType a_type)
	{
		Decoration::Decoration& _decoration = m_decorationVec.emplace_back();
		_decoration.type = a_type;

		// 名前が全部同じだと一覧で見分けられないので、種類と番号を入れておく
		const char* _typeName = "Decoration";
		switch (a_type)
		{
		case Decoration::EDecorationType::Image:   _typeName = "Image";   break;
		case Decoration::EDecorationType::Text:    _typeName = "Text";    break;
		case Decoration::EDecorationType::Polygon:
		default:                                   _typeName = "Polygon"; break;
		}
		_decoration.name = std::string(_typeName) + std::to_string(m_decorationVec.size());

		return _decoration;
	}

	Decoration::Decoration* UIBase::FindDecoration(const std::string& a_name)
	{
		for (Decoration::Decoration& _decoration : m_decorationVec)
		{
			if (_decoration.name == a_name) return &_decoration;
		}
		return nullptr;
	}

	//======================================================================================
	// 飾りの描画
	//======================================================================================
	Decoration::ParentTransform UIBase::MakeParentTransform() const
	{
		Decoration::ParentTransform _parent = {};
		_parent.pixelPos = m_pixelPos;
		_parent.rotation = m_rotation;
		_parent.scale = m_scale;
		_parent.layer = m_layer;
		_parent.color = m_color;

		return _parent;
	}

	void UIBase::DrawDecorations(
		Engine::GameObject::ObjectContext& a_context,
		const Decoration::DrawOverride& a_override)
	{
		// 出さない指示はここでまとめて弾く。
		// 継承先が Draw を自前で持っていても、切れば必ず消えるようにするため
		if (!m_isVisible) return;
		if (m_decorationVec.empty()) return;
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;
		if (!a_context.pServices->pResourceManager) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		const Decoration::ParentTransform _parent = MakeParentTransform();

		// 配列の順に積む : 後ろにあるものほど手前に出る
		for (const Decoration::Decoration& _decoration : m_decorationVec)
		{
			Decoration::DrawDecoration(
				_pGE,
				a_context.pServices->pResourceManager,
				_decoration,
				_parent,
				a_override);
		}
	}

	void UIBase::RequestDecorationResources(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

		for (Decoration::Decoration& _decoration : m_decorationVec)
		{
			Decoration::RequestResources(_decoration, a_context.pServices->pResourceManager);
		}
	}

	//======================================================================================
	// シリアライズ
	//--------------------------------------------------------------------------------------
	// 前半はテクスチャを1枚だけ持っていた頃の並びをそのまま残してある。
	// 順番を崩すと、既に保存されているシーンが読めなくなるため。
	// 飾りの配列は末尾へ足し、配列を持たない古いシーンだけ TexGUID から作り直す
	//======================================================================================
	void UIBase::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// ---- 旧形式の名残(読み書きは続けるが、使うのは引き継ぎのときだけ) ----
		a_ar.GUIDField("TexGUID", m_legacyTexGUID);

		a_ar.Field("Color", m_color);

		a_ar.Field("PosPixel", m_pixelPos);
		a_ar.Field("SizePixel", m_pixelSize);
		a_ar.Field("m_rotation", m_rotation);
		a_ar.Field("m_pivot", m_pivot);
		a_ar.Field("m_uvOffset", m_legacyUvOffset);
		a_ar.Field("m_layer", m_layer);
		a_ar.Field("m_scale", m_scale);

		// 出し分けの状態。※ 追加は必ずここより上でなく末尾へ
		//    (バイナリは並び順で読むので、間に挟むと既存のデータがずれる)
		a_ar.Field("IsVisible", m_isVisible);

		// ---- 飾り ----
		size_t _decorationCount = m_decorationVec.size();
		const bool _hasDecorationArray = a_ar.BeginArray("Decorations", _decorationCount);
		if (_hasDecorationArray)
		{
			m_decorationVec.resize(_decorationCount);

			for (size_t _i = 0; _i < _decorationCount; ++_i)
			{
				if (!a_ar.BeginObject(_i)) continue;

				Decoration::ArchiveDecoration(a_ar, m_decorationVec[_i]);

				a_ar.EndObject();
			}
			a_ar.EndArray();
		}

		m_editSize = m_pixelSize;

		if (!a_ar.IsLoading()) return;

		//----------------------------------------------------------------------------------
		// 旧形式からの引き継ぎ
		//
		// 飾りの配列を持たないシーンだけが対象。
		// Init が既定の飾りを作っている継承先では、その画像へ保存されていたGUIDを移す
		// (作り直すと、継承先が入れた大きさや色まで消えてしまうため)
		//----------------------------------------------------------------------------------
		if (!_hasDecorationArray && m_legacyTexGUID.IsValid())
		{
			Decoration::Decoration* _pImage = nullptr;
			for (Decoration::Decoration& _decoration : m_decorationVec)
			{
				if (_decoration.type != Decoration::EDecorationType::Image) continue;
				_pImage = &_decoration;
				break;
			}

			if (_pImage == nullptr)
			{
				_pImage = &AddDecoration(Decoration::EDecorationType::Image);
				_pImage->pixelSize = m_pixelSize;
				_pImage->pivot = m_pivot;
			}

			_pImage->texGUID = m_legacyTexGUID;
			_pImage->uvOffset = m_legacyUvOffset;
		}

		// 読み込み時は復元したGUIDでテクスチャ・フォントを引き直す。
		// 実体が届くのを待つ必要はないので、要求だけ出して先へ進む
		// (描画側は IsReady を見て、まだのフレームは描かない)
		RequestDecorationResources(a_context);
	}

	//======================================================================================
	// カーソル位置をUIのピクセル座標へ直す
	//--------------------------------------------------------------------------------------
	// クライアント領域(実際のウィンドウの大きさ) → 描画解像度(UIの座標系)。
	// バックバッファは描画解像度で作られ、クライアント領域へ引き伸ばして表示されるので、
	// 単純に比率を掛ければよい。
	// (ウィンドウサイズを変えても判定がずれないよう、毎フレーム実測する)
	//======================================================================================
	bool UIBase::CalcCursorUIPos(Engine::GameObject::ObjectContext& a_context, Math::Vector2& a_outPos)
	{
		if (!a_context.pServices) return false;
		if (!a_context.pServices->pInputManager || !a_context.pServices->pOptionManager) return false;
		if (!a_context.pServices->pMainEngine) return false;

		// カーソル(クライアント座標)
		Math::Vector2 _clientPos = {};
		if (!a_context.pServices->pInputManager->GetCursorClientPos(_clientPos)) return false;

		// 描画解像度
		const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
		const float _renderW = static_cast<float>(_winOp.windowWidth);
		const float _renderH = static_cast<float>(_winOp.windowHeight);
		if (_renderW <= 0.0f || _renderH <= 0.0f) return false;

		// クライアント領域の実サイズ
		const auto* _pWind = a_context.pServices->pMainEngine->GetNativeWindow();
		if (!_pWind) return false;

		const float _clientW = static_cast<float>(_pWind->GetClientWidth());
		const float _clientH = static_cast<float>(_pWind->GetClientHeight());

		// 最小化中は0になる
		if (_clientW <= 0.0f || _clientH <= 0.0f) return false;

		a_outPos.x = _clientPos.x * (_renderW / _clientW);
		a_outPos.y = _clientPos.y * (_renderH / _clientH);

		return true;
	}

	//======================================================================================
	// 矩形の内側か
	//--------------------------------------------------------------------------------------
	// GraphicsEngine::PushUIData がクアッドを組み立てるのと同じ式で軸を作り、
	// その軸へ射影した長さで判定する。回転もピボットもそのまま効く。
	// (判定用に別の値を持たせると「絵はここなのに押せない」ズレが必ず出るため、
	//  描画に渡すものと同じ値をそのまま受け取る形にしてある)
	//
	//   ローカル+X(画面右) を回したもの : ( cos,  sin)
	//   ローカル+Y(画面上) を回したもの : ( sin, -cos)
	//   クアッド中心 : ピボット位置 + R * ピボットからのずれ
	//======================================================================================
	bool UIBase::IsPointInside(
		const Math::Vector2& a_uiPos,
		const Math::Vector2& a_pixelPos,
		const Math::Vector2& a_pixelSize,
		const Math::Vector2& a_pivot,
		float a_rotationDeg,
		const Math::Vector2& a_hitPadding)
	{
		// 判定の半サイズ(余白ぶんを足す)
		const Math::Vector2 _half = {
			a_pixelSize.x * 0.5f + a_hitPadding.x,
			a_pixelSize.y * 0.5f + a_hitPadding.y
		};

		// 大きさが無ければ触りようがない
		if (_half.x <= 0.0f || _half.y <= 0.0f) return false;

		const float _rad = DirectX::XMConvertToRadians(a_rotationDeg);
		const float _cos = std::cos(_rad);
		const float _sin = std::sin(_rad);

		// ピボットからクアッド中心までのずれ(回転前)
		const Math::Vector2 _pivotOff = {
			(0.5f - a_pivot.x) * a_pixelSize.x,
			(0.5f - a_pivot.y) * a_pixelSize.y
		};

		// 回転はピボットを中心に行われる
		const Math::Vector2 _center = {
			a_pixelPos.x + (_pivotOff.x * _cos - _pivotOff.y * _sin),
			a_pixelPos.y + (_pivotOff.x * _sin + _pivotOff.y * _cos)
		};

		const Math::Vector2 _diff = { a_uiPos.x - _center.x, a_uiPos.y - _center.y };

		// 各軸へ射影した長さ(軸はどちらも単位ベクトル)
		const float _u = _diff.x * _cos + _diff.y * _sin;
		const float _v = _diff.x * _sin - _diff.y * _cos;

		return (std::fabs(_u) <= _half.x) && (std::fabs(_v) <= _half.y);
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void UIBase::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices) return;
		if (!a_context.pServices->pOptionManager || !a_context.pServices->pResourceManager) return;

		// ウィンドウサイズの取得
		const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);

		// 表示するか : 出し分けを持つ画面(ホームなど)は進行役がここを切り替える
		ImGui::Checkbox("Visible", &m_isVisible);
		ImGui::SameLine();
		ImGui::TextDisabled("(切ると描画も入力も止まる)");

		ImGui::Spacing();

		// 色 : 全ての飾りへ乗算で掛かる。畳まずに常に出しておく
		// (白い板ポリを1つ置いて、色だけで作り分けられるようにするため)
		Engine::Editor::EditorHelper::DrawColorEdit("Color", m_color);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 座標系
		ImGui::DragFloat2("PixelPos", &m_pixelPos.x, 1.0f);						// スクリーン座標
		ImGui::Spacing();

		ImGui::DragFloat("Rotation", &m_rotation, 0.1f, -360.0f, 360.0f);
		if (m_rotation >= 360) m_rotation -= 360;
		if (m_rotation <= -360) m_rotation += 360;

		ImGui::Spacing();
		if (ImGui::DragFloat("Scale", &m_scale, 0.01f, 0.0f))						// 等倍拡縮
		{
			m_pixelSize = m_editSize * m_scale;
		}
		if (ImGui::DragFloat2("PixelSize", &m_pixelSize.x, 1.0f, 0.0f, 8192.0f))	// ピクセルサイズ
		{
			m_editSize = m_pixelSize / m_scale;
		}
		ImGui::TextDisabled("アンカー自身の矩形(当たり判定・判定円の基準)。見た目は飾り側のサイズ");

		ImGui::Spacing();

		// 初期化用ボタン
		if (ImGui::Button("RefreshTransform"))
		{
			m_pixelPos = { _w / 2.0f,_h / 2.0f };
			m_pixelSize = { _w / 4 ,_h / 4 };
			m_rotation = 0.0f;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ピボット : 正規化[0,1]。(0.5,0.5)=中心, (0,0)=左上, (1,1)=右下。
		// この点が PixelPos に配置され、回転の中心にもなる。
		ImGui::DragFloat2("Pivot (0-1)", &m_pivot.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("LayerZ", &m_layer, 1.0f);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 飾り
		DrawDecorationListInspector(a_context);
	}

	//======================================================================================
	// 飾りの一覧
	//--------------------------------------------------------------------------------------
	// 描く順は配列順なので、並べ替えがそのまま重なり順になる。
	// 開いている1つだけ中身を出す形にしてあるのは、飾りが増えると
	// 全部展開したときにインスペクターが縦に流れて使えなくなるため
	//======================================================================================
	void UIBase::DrawDecorationListInspector(Engine::GameObject::ObjectContext& a_context)
	{
		auto* _pResourceManager = a_context.pServices ? a_context.pServices->pResourceManager : nullptr;

		ImGui::SeparatorText("Decorations");
		ImGui::TextDisabled("配列の順に描きます(下にあるものほど手前)");

		// ---- 追加 ----
		if (Engine::Editor::EditorHelper::CreateButton("Add Polygon"))
		{
			AddDecoration(Decoration::EDecorationType::Polygon);
			m_editDecorationIndex = static_cast<int>(m_decorationVec.size()) - 1;
		}
		ImGui::SameLine();
		if (Engine::Editor::EditorHelper::CreateButton("Add Image"))
		{
			AddDecoration(Decoration::EDecorationType::Image);
			m_editDecorationIndex = static_cast<int>(m_decorationVec.size()) - 1;
		}
		ImGui::SameLine();
		if (Engine::Editor::EditorHelper::CreateButton("Add Text"))
		{
			AddDecoration(Decoration::EDecorationType::Text);
			m_editDecorationIndex = static_cast<int>(m_decorationVec.size()) - 1;
		}

		// ---- 全削除 ----
		// 戻せないので Ctrl を押している間だけ効かせる
		if (!m_decorationVec.empty())
		{
			ImGui::SameLine();
			if (Engine::Editor::EditorHelper::DeleteButton("Clear All") && ImGui::GetIO().KeyCtrl)
			{
				m_decorationVec.clear();
				m_editDecorationIndex = -1;
			}
			ImGui::SetItemTooltip("Ctrl+クリックで全部消す");
		}

		ImGui::Spacing();

		// 一覧を回している間に配列を触ると足元が崩れるので、操作は覚えておいて後でまとめて行う
		int _removeIndex = -1;
		int _swapIndex = -1;		// この番号と次の番号を入れ替える

		for (int _i = 0; _i < static_cast<int>(m_decorationVec.size()); ++_i)
		{
			Decoration::Decoration& _decoration = m_decorationVec[_i];

			ImGui::PushID(_i);

			//----------------------------------------------------------------------
			// 1行ぶん : [X][↑][↓] 名前
			//
			// ボタンを先に置くこと。
			// Selectable は残りの幅を全部使うので、後ろへ並べると
			// ボタンが行の外まで押し出されて押せなくなる
			//----------------------------------------------------------------------
			if (Engine::Editor::EditorHelper::DeleteSmallButton("X")) _removeIndex = _i;
			ImGui::SetItemTooltip("この飾りを消す");

			ImGui::SameLine();
			if (ImGui::ArrowButton("##Up", ImGuiDir_Up) && _i > 0) _swapIndex = _i - 1;

			ImGui::SameLine();
			if (ImGui::ArrowButton("##Down", ImGuiDir_Down) &&
				_i + 1 < static_cast<int>(m_decorationVec.size()))
			{
				_swapIndex = _i;
			}

			// 開閉 : 開いているものだけ中身を出す
			ImGui::SameLine();
			const bool _isOpen = (m_editDecorationIndex == _i);
			const std::string _label =
				std::to_string(_i) + " : " + (_decoration.name.empty() ? "(no name)" : _decoration.name);

			if (ImGui::Selectable(_label.c_str(), _isOpen))
			{
				m_editDecorationIndex = _isOpen ? -1 : _i;
			}

			if (_isOpen)
			{
				ImGui::Indent();
				Decoration::DrawDecorationInspector(_decoration, _pResourceManager);
				ImGui::Unindent();
				ImGui::Separator();
			}

			ImGui::PopID();
		}

		if (_swapIndex >= 0)
		{
			std::swap(m_decorationVec[_swapIndex], m_decorationVec[_swapIndex + 1]);

			// 開いていたものを追いかける
			if (m_editDecorationIndex == _swapIndex)          m_editDecorationIndex = _swapIndex + 1;
			else if (m_editDecorationIndex == _swapIndex + 1) m_editDecorationIndex = _swapIndex;
		}

		if (_removeIndex >= 0)
		{
			m_decorationVec.erase(m_decorationVec.begin() + _removeIndex);

			// 消したぶん番号がずれる
			if (m_editDecorationIndex == _removeIndex)     m_editDecorationIndex = -1;
			else if (m_editDecorationIndex > _removeIndex) --m_editDecorationIndex;
		}
	}

	bool UIBase::DrawGizmo(const Engine::GameObject::ObjectGizmoContext& a_ctx, Engine::GameObject::ObjectContext& a_context)
	{
		// シーンビュー上にドラッグ可能なハンドルを出してピクセル座標を編集する
		if (a_ctx.viewportSize.x <= 0.0f || a_ctx.viewportSize.y <= 0.0f) return false;
		if (!a_context.pServices || !a_context.pServices->pOptionManager) return false;

		// ウィンドウサイズの取得
		const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);
		if (_w <= 0.0f || _h <= 0.0f) return false;

		// ゲーム内ピクセル(左上原点) から シーンビュー上ピクセルへ。
		// m_pixelPos はピボットのスクリーン座標なので、ハンドルはそのままピボット位置を指す。
		ImVec2 _handle = {};
		_handle.x = a_ctx.viewportPos.x + (m_pixelPos.x / _w) * a_ctx.viewportSize.x;
		_handle.y = a_ctx.viewportPos.y + (m_pixelPos.y / _h) * a_ctx.viewportSize.y;

		// ギズモハンドルの半径 : ピクセル
		static const float _handleRadius = 9.0f;

		// ドラッグ操作用の透明ボタン
		ImGui::SetCursorScreenPos(ImVec2(_handle.x - _handleRadius,_handle.y - _handleRadius));
		ImGui::InvisibleButton("##UIGizmo", ImVec2(_handleRadius * 2.0f, _handleRadius * 2.0f));
		const bool _active = ImGui::IsItemActive();			// 選択されているかどうか
		const bool _hovered = ImGui::IsItemHovered();		// カーソルが重なっているかどうか

		// ハンドル描画(十字 + 円)
		ImDrawList* _dl = ImGui::GetWindowDrawList();
		const ImU32 _col = _active ? IM_COL32(255, 200, 0, 255)
			: _hovered ? IM_COL32(255, 255, 255, 255)
			: IM_COL32(0, 200, 255, 255);
		_dl->AddCircle(_handle, _handleRadius, _col, 20, 2.0f);
		_dl->AddLine(ImVec2(_handle.x - _handleRadius * 1.8f, _handle.y), ImVec2(_handle.x + _handleRadius * 1.8f, _handle.y), _col, 1.5f);
		_dl->AddLine(ImVec2(_handle.x, _handle.y - _handleRadius * 1.8f), ImVec2(_handle.x, _handle.y + _handleRadius * 1.8f), _col, 1.5f);

		// ドラッグ中はマウス位置からピクセル座標を逆算して更新
		if (_active)
		{
			const ImVec2 _mouse = ImGui::GetMousePos();
			const float _u = (_mouse.x - a_ctx.viewportPos.x) / a_ctx.viewportSize.x;	// 0..1
			const float _v = (_mouse.y - a_ctx.viewportPos.y) / a_ctx.viewportSize.y;	// 0..1
			m_pixelPos.x = std::clamp(_u * _w, 0.0f, _w);
			m_pixelPos.y = std::clamp(_v * _h, 0.0f, _h);
		}

		return true;
	}
}
