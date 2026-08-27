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
#include "Engine/Audio/AudioManager.h"				// 乗った音・押した音

#include "../../../Engine/Editor/Helper/EditorHelper.h"

namespace App::Object
{
	namespace
	{
		// 度で回す : IsPointInside と同じ向き(時計回り)
		Math::Vector2 RotateDeg(const Math::Vector2& a_value, float a_degree)
		{
			if (a_degree == 0.0f) return a_value;

			const float _rad = DirectX::XMConvertToRadians(a_degree);
			const float _cos = std::cos(_rad);
			const float _sin = std::sin(_rad);

			return {
				a_value.x * _cos - a_value.y * _sin,
				a_value.x * _sin + a_value.y * _cos
			};
		}
	}

	void UIBase::Release(Engine::GameObject::ObjectContext& a_context)
	{
		ReleaseUISounds(a_context);
	}

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
		// カーソルの当たり判定と押下の進行
		UpdateInteraction(a_context);

		// 飾りは今の状態を受け取って、そこへ寄っていく
		const Decoration::EUIState _state = GetUIState();

		for (Decoration::Decoration& _decoration : m_decorationVec)
		{
			Decoration::AdvanceAnimation(_decoration, _state, a_context.dt);
		}
	}

	//======================================================================================
	// 今の状態
	//======================================================================================
	Decoration::EUIState UIBase::GetUIState() const
	{
		if (!m_isInteractable) return Decoration::EUIState::Disabled;
		if (m_isPressed)       return Decoration::EUIState::Pressed;
		if (m_isHovered)       return Decoration::EUIState::Hovered;

		return Decoration::EUIState::Normal;
	}

	//======================================================================================
	// カーソルの当たり判定と押下の進行
	//--------------------------------------------------------------------------------------
	// ・当たり判定はアンカーの矩形(PixelPos / PixelSize / Pivot / Rotation)を使う。
	//   飾りは何枚でも生やせるので、そのどれかではなくアンカーを唯一の基準にしてある。
	//
	// ・押下は「押し始めも離しも矩形の内側」で成立させる。押したまま外へ逃がせば
	//   取り消せる、よくあるボタンの作法に合わせてある。
	//
	// ・入力はプレイモード中しか受け取らない(InputManager 側で止まる)。
	//   エディター操作でUIが光ったり押されたりしない。
	//======================================================================================
	void UIBase::UpdateInteraction(Engine::GameObject::ObjectContext& a_context)
	{
		// 鳴らし直しの間引きを進める。
		// 反応しない状態でも進めておかないと、無効にしている間に止まってしまう
		if (m_hoverSoundCoolTime > 0.0f) m_hoverSoundCoolTime = std::max(m_hoverSoundCoolTime - a_context.dt, 0.0f);
		if (m_pressSoundCoolTime > 0.0f) m_pressSoundCoolTime = std::max(m_pressSoundCoolTime - a_context.dt, 0.0f);

		// 「このフレームに押し切られたか」は毎フレーム作り直す
		m_isClicked = false;

		// 乗った瞬間を見るために、前のフレームの状態を控えておく
		const bool _wasHovered = m_isHovered;

		//==================================================================
		// 反応しない状態
		//------------------------------------------------------------------
		// ・出していない(見えていないものは触れない)
		// ・無効にされている
		// ・プレイモードでない / エディターで文字を打っている
		// このときは状態を全部落とす。押しっぱなしのまま無効にされて、
		// 有効へ戻した瞬間に押し切られたことにならないようにする。
		//==================================================================
		if (!a_context.pServices || !a_context.pServices->pInputManager ||
			!m_isVisible || !m_isInteractable ||
			!a_context.pServices->pInputManager->IsGameInputEnable())
		{
			m_isHovered = false;
			m_isPressed = false;
			m_isPressStartedInside = false;
			return;
		}

		auto& _input = *a_context.pServices->pInputManager;

		//==================================================================
		// カーソルが乗っているか
		//==================================================================
		Math::Vector2 _cursorPos = {};
		const bool _hasCursor = CalcCursorUIPos(a_context, _cursorPos);

		m_isHovered = _hasCursor && IsPointInsideSelf(_cursorPos);

		// 乗った瞬間だけ鳴らす。乗っている間ずっとだと鳴り続けてしまう
		if (m_isHovered && !_wasHovered)
		{
			PlayUISound(a_context, m_hoverSoundGUID, m_hoverSoundHandle, m_hoverSoundCoolTime);
		}

		//==================================================================
		// 押下の進行
		//==================================================================
		const bool _isPressMoment   = _input.IsPress(m_clickActionName);	// 押した瞬間
		const bool _isHoldMoment    = _input.IsHold(m_clickActionName);		// 押している間
		const bool _isReleaseMoment = _input.IsRelease(m_clickActionName);	// 離した瞬間

		// 内側で押し始めたときだけ受け付ける
		if (_isPressMoment && m_isHovered)
		{
			m_isPressStartedInside = true;

			// 押し切るまで待つと手応えが遅れるので、押した瞬間に鳴らす
			PlayUISound(a_context, m_pressSoundGUID, m_pressSoundHandle, m_pressSoundCoolTime);
		}

		m_isPressed = m_isPressStartedInside && _isHoldMoment;

		if (_isReleaseMoment)
		{
			// 押し始めと離しの両方が内側なら成立
			m_isClicked = (m_isPressStartedInside && m_isHovered);

			m_isPressStartedInside = false;
			m_isPressed = false;
		}

		// 押していないのに押し始めの記録が残っていたら落とす
		// (ボタンを離した瞬間を取りこぼした場合の保険)
		if (!_isHoldMoment && !_isReleaseMoment)
		{
			m_isPressStartedInside = false;
			m_isPressed = false;
		}
	}

	//======================================================================================
	// 自分の判定矩形の内側か
	//--------------------------------------------------------------------------------------
	// 判定そのものは静的な共通実装。ここは矩形を組み立てて渡すだけ。
	//
	// 使う矩形は3通り :
	//   HitFollowAnim が立っている … 飾りの今の範囲(アニメーション・反応込み)から作る
	//   PixelSize が入っている     … アンカーの矩形をそのまま使う(判定を絵とずらしたいとき用)
	//   PixelSize が 0            … 飾りが占めている範囲(素の大きさ)から作る
	//
	// 飾りから作る道を用意してあるのは、見た目を飾り側へ移したことで
	// アンカーの大きさを入れ忘れやすくなったため。
	// 幅0の矩形はどこにも当たらないので、そのままだと
	// 「置いて飾りを付けたのに、乗っても何も起きない」になる。
	//
	// HitFollowAnim は絵の大きさが動くもの用。アンカーの矩形は動かないので、
	// 切ったままだと大きくなった絵のふちがどこにも当たらない
	//======================================================================================
	bool UIBase::IsPointInsideSelf(const Math::Vector2& a_uiPos) const
	{
		// アンカーに大きさが入っていても、今の絵へ追従させる指示があれば飾りを優先する
		const bool _isUseDecorationBounds =
			m_isHitFollowAnim || m_pixelSize.x <= 0.0f || m_pixelSize.y <= 0.0f;

		if (_isUseDecorationBounds)
		{
			Math::Vector2 _center = {};
			Math::Vector2 _size = {};
			if (CalcDecorationBounds(_center, _size, m_isHitFollowAnim))
			{
				// 範囲の中心を指す点。回転と倍率はアンカーのものが乗る
				const Math::Vector2 _pos = m_pixelPos + RotateDeg(_center * m_scale, m_rotation);

				return UIBase::IsPointInside(
					a_uiPos,
					_pos,
					_size * m_scale,
					{ 0.5f, 0.5f },		// 中心を出しているのでピボットは中央
					m_rotation,
					m_hitPadding);
			}

			// 測れる飾りが1つも無ければアンカーの矩形へ戻る(文字だけのUIなど)
		}

		if (m_pixelSize.x <= 0.0f || m_pixelSize.y <= 0.0f) return false;

		return UIBase::IsPointInside(
			a_uiPos,
			m_pixelPos,
			m_pixelSize,
			m_pivot,
			m_rotation,
			m_hitPadding);
	}

	//======================================================================================
	// 飾りが占めている範囲
	//--------------------------------------------------------------------------------------
	// a_isIncludeAnim を立てると、素の矩形に加えて
	// 「アニメーション・反応を掛けた今の矩形」も範囲へ入れる(2つの合併)。
	//
	// 今の矩形だけに差し替えないのは、乗ると縮む反応を付けたときに
	// 「乗る→縮んで外れる→戻って乗る」が毎フレーム入れ替わってちらつくため。
	// 合併にしておけば判定が素の大きさより痩せないので、広がる側だけが効く
	//======================================================================================
	bool UIBase::CalcDecorationBounds(
		Math::Vector2& a_outCenter,
		Math::Vector2& a_outSize,
		bool a_isIncludeAnim) const
	{
		bool _hasAny = false;
		float _minX = 0.0f, _minY = 0.0f, _maxX = 0.0f, _maxY = 0.0f;

		// 矩形1つぶんを範囲へ足す : 回転を掛けた4隅を取り、それを囲む矩形にする
		const auto _addRect = [&](
			const Math::Vector2& a_offset,
			const Math::Vector2& a_size,
			const Math::Vector2& a_pivot,
			float a_rotation)
		{
			if (a_size.x <= 0.0f || a_size.y <= 0.0f) return;

			const Math::Vector2 _topLeft = {
				-a_pivot.x * a_size.x,
				-a_pivot.y * a_size.y
			};
			const Math::Vector2 _cornerArray[4] = {
				_topLeft,
				{ _topLeft.x + a_size.x, _topLeft.y },
				{ _topLeft.x,            _topLeft.y + a_size.y },
				{ _topLeft.x + a_size.x, _topLeft.y + a_size.y },
			};

			for (const Math::Vector2& _corner : _cornerArray)
			{
				const Math::Vector2 _point = a_offset + RotateDeg(_corner, a_rotation);

				if (!_hasAny)
				{
					_minX = _maxX = _point.x;
					_minY = _maxY = _point.y;
					_hasAny = true;
					continue;
				}

				_minX = std::min(_minX, _point.x);
				_minY = std::min(_minY, _point.y);
				_maxX = std::max(_maxX, _point.x);
				_maxY = std::max(_maxY, _point.y);
			}
		};

		for (const Decoration::Decoration& _decoration : m_decorationVec)
		{
			if (!_decoration.isVisible) continue;

			// 文字は大きさをフォントから組み立てるので、ここでは測れない。
			// 文字だけのUIに判定を持たせたいときは PixelSize を入れること
			if (_decoration.type == Decoration::EDecorationType::Text) continue;

			const Math::Vector2 _size = _decoration.pixelSize * _decoration.scale;
			if (_size.x <= 0.0f || _size.y <= 0.0f) continue;

			// 素の矩形
			_addRect(_decoration.offsetPos, _size, _decoration.pivot, _decoration.rotation);

			if (!a_isIncludeAnim) continue;

			//----------------------------------------------------------------------
			// 今の矩形 : 描画と同じ合成結果を使う
			//
			// 反応で消している飾り(visibleRate が 0 に寄っているもの)も
			// 大きさとしては数える。透明な枠のぶんまで判定が伸びるのが困るなら、
			// その飾りの isVisible を切ること
			//----------------------------------------------------------------------
			const Decoration::DecorationTransform _now =
				Decoration::CalcCurrentTransform(_decoration);

			_addRect(
				_decoration.offsetPos + _now.offsetAdd,
				_size * _now.scaleMul,
				_decoration.pivot,
				_decoration.rotation + _now.rotationAdd);
		}

		if (!_hasAny) return false;

		a_outCenter = { (_minX + _maxX) * 0.5f, (_minY + _maxY) * 0.5f };
		a_outSize   = { _maxX - _minX, _maxY - _minY };

		return true;
	}

	//======================================================================================
	// 音
	//======================================================================================
	void UIBase::PlayUISound(
		Engine::GameObject::ObjectContext& a_context,
		const Engine::GUID& a_guid,
		Engine::Handle<Engine::Resource::SoundInstance>& a_inoutHandle,
		float& a_inoutCoolTime)
	{
		if (!a_guid.IsValid()) return;
		if (!a_context.pServices || !a_context.pServices->pAudioManager) return;

		//--------------------------------------------------------------
		// 間引き
		//
		// 判定の縁でカーソルが揺れると、乗った/離れたが毎フレーム入れ替わる。
		// インスタンスは1つなので音自体は重ならないが、頭出しの鳴らし直しが
		// 連続すると残響が積み上がって、だんだん大きくなったように聞こえる
		//--------------------------------------------------------------
		if (a_inoutCoolTime > 0.0f) return;
		a_inoutCoolTime = m_soundMinInterval;

		auto* _pAudioManager = a_context.pServices->pAudioManager;

		// 初めて鳴らすときに借りる。画面に並ぶUI全部が先に確保すると席が尽きる
		if (!a_inoutHandle.IsValid())
		{
			// 画面に出す音なので 2D で発行する(定位を付けない)。
			// UI の札を付けておくと、設定画面の UI 音量がそのまま効く
			a_inoutHandle = _pAudioManager->RequestSoundInstance(
				a_guid, false, Engine::Audio::ESoundGroup::Ui);
		}

		if (auto* _pInstance = _pAudioManager->RefInstance(a_inoutHandle))
		{
			_pInstance->SetVolume(m_soundVolume);
			_pInstance->Play(false);
		}
	}

	void UIBase::ReleaseUISounds(Engine::GameObject::ObjectContext& a_context)
	{
		// サウンドインスタンスのプールはアプリ寿命なので、借りた側が必ず返す
		if (!a_context.pServices || !a_context.pServices->pAudioManager) return;

		auto* _pAudioManager = a_context.pServices->pAudioManager;
		_pAudioManager->ReleaseSoundInstance(m_hoverSoundHandle);
		_pAudioManager->ReleaseSoundInstance(m_pressSoundHandle);

		m_hoverSoundHandle = {};
		m_pressSoundHandle = {};
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

	void UIBase::DrawDecorationAt(
		Engine::GameObject::ObjectContext& a_context,
		size_t a_index,
		const Decoration::DrawOverride& a_override)
	{
		if (!m_isVisible) return;
		if (a_index >= m_decorationVec.size()) return;
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;
		if (!a_context.pServices->pResourceManager) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		Decoration::DrawDecoration(
			_pGE,
			a_context.pServices->pResourceManager,
			m_decorationVec[a_index],
			MakeParentTransform(),
			a_override);
	}

	int UIBase::FindDecorationIndex(const std::string& a_name) const
	{
		for (size_t _i = 0; _i < m_decorationVec.size(); ++_i)
		{
			if (m_decorationVec[_i].name == a_name) return static_cast<int>(_i);
		}
		return -1;
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

		// ---- カーソルへの反応 ----
		a_ar.StringField("ClickActionName", m_clickActionName);
		a_ar.Field("HitPadding", m_hitPadding);
		a_ar.Field("IsInteractable", m_isInteractable);
		a_ar.GUIDField("HoverSoundGUID", m_hoverSoundGUID);
		a_ar.GUIDField("PressSoundGUID", m_pressSoundGUID);
		a_ar.Field("SoundVolume", m_soundVolume);
		a_ar.Field("SoundMinInterval", m_soundMinInterval);

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

		// ---- ここから下は後から足したもの : 追加は必ず末尾へ ----
		a_ar.Field("HitFollowAnim", m_isHitFollowAnim);

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
		ImGui::DragFloat("Layer", &m_layer, 0.1f);
		ImGui::TextDisabled("重なり順。大きいほど手前(同じ値なら置いた順)");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		//----------------------------------------------------------------------
		// カーソルへの反応
		//
		// 見た目の変化は飾り側(Decoration の Reaction)。ここは判定と音だけ
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Interaction");

		ImGui::Checkbox("Interactable", &m_isInteractable);
		ImGui::SameLine();
		ImGui::TextDisabled("(切ると Disabled 扱いになる)");

		ImGui::InputText("ClickAction", &m_clickActionName);
		ImGui::TextDisabled("InputManager へ登録したアクション名");

		ImGui::DragFloat2("HitPadding", &m_hitPadding.x, 1.0f);
		ImGui::TextDisabled("判定の矩形へ足す余白(px)");

		ImGui::Checkbox("HitFollowAnim", &m_isHitFollowAnim);
		ImGui::TextDisabled("飾りのアニメ・反応で大きくなったぶんも判定に入れる(PixelSize より優先)");

		//----------------------------------------------------------------------
		// いま効いている判定を出す
		//
		// 幅0の矩形はどこにも当たらないので、乗らない原因がここだと分かるようにする
		//----------------------------------------------------------------------
		Math::Vector2 _hitCenter = {};
		Math::Vector2 _hitSize = {};
		const bool _hasHitBounds = CalcDecorationBounds(_hitCenter, _hitSize, m_isHitFollowAnim);

		if (m_isHitFollowAnim && _hasHitBounds)
		{
			// 実行中は毎フレーム変わる。止まっているときは素の大きさと同じ
			ImGui::Text("Hit : %.0f x %.0f (飾りの範囲/アニメ込み)",
				_hitSize.x * m_scale, _hitSize.y * m_scale);
		}
		else if (m_pixelSize.x > 0.0f && m_pixelSize.y > 0.0f)
		{
			ImGui::Text("Hit : %.0f x %.0f (PixelSize)", m_pixelSize.x, m_pixelSize.y);

			if (m_isHitFollowAnim)
			{
				ImGui::TextDisabled("HitFollowAnim は立っていますが、測れる飾りが無いので PixelSize です");
			}
		}
		else if (_hasHitBounds)
		{
			ImGui::Text("Hit : %.0f x %.0f (飾りの範囲)", _hitSize.x * m_scale, _hitSize.y * m_scale);
			ImGui::TextDisabled("PixelSize が 0 なので飾りの範囲を使っています");
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Hit : なし");
			ImGui::TextDisabled("PixelSize も飾りの大きさも 0 です。カーソルに反応しません");
		}

		ImGui::Spacing();

		// 音を差し替えたら、借りているインスタンスを返して取り直させる
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("HoverSound", "Sound", m_hoverSoundGUID))
		{
			ReleaseUISounds(a_context);
		}
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("PressSound", "Sound", m_pressSoundGUID))
		{
			ReleaseUISounds(a_context);
		}
		ImGui::DragFloat("SoundVolume", &m_soundVolume, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("SoundMinInterval", &m_soundMinInterval, 0.01f, 0.0f, 1.0f);
		ImGui::TextDisabled("鳴らし直す最短間隔(秒)。縁で揺れて鳴り続けるのを止める");

		// 実行中の状態は表示のみ
		static const char* _stateName[] = { "Normal", "Hovered", "Pressed", "Disabled" };
		ImGui::Text("State : %s", _stateName[static_cast<int>(GetUIState())]);

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
