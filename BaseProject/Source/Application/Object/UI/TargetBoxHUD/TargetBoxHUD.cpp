#include "TargetBoxHUD.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Character/LockOnTargetComponent.h"

namespace App::Object
{
	namespace
	{
		// 既定のターゲットボックステクスチャのパス
		constexpr const char* TARGET_BOX_TEXTURE_PATH = "Asset/Texture/Reticle/Robot01.png";

		// 新規追加時の既定表示サイズ(px)
		constexpr float DEFAULT_BOX_SIZE = 96.0f;
	}

	void TargetBoxHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		// リソース周りはコンテキストが運んできたサービスを使う
		if (!a_context.pServices) return;
		if (!a_context.pServices->pAssetDatabase || !a_context.pServices->pResourceManager) return;

		// 新規追加直後はサイズが0で何も見えないので、既定サイズを入れておく。
		// シーン読み込み時はこの後の Archive で保存値に上書きされる。
		if (m_pixelSize.x <= 0.0f || m_pixelSize.y <= 0.0f)
		{
			m_pixelSize = { DEFAULT_BOX_SIZE, DEFAULT_BOX_SIZE };
			m_editSize = m_pixelSize;
		}

		//--------------------------------------------------------------
		// 既定の枠を1つ用意する
		//
		// 作るのは飾りを1つも持っていないときだけ。
		// シーン読み込み時はこの後の Archive が保存された飾りで置き換える
		//--------------------------------------------------------------
		if (m_decorationVec.empty())
		{
			Decoration::Decoration& _box = AddDecoration(Decoration::EDecorationType::Image);
			_box.name = "TargetBox";
			_box.group = GROUP_NORMAL;
			_box.pixelSize = m_pixelSize;
			_box.texGUID = a_context.pServices->pAssetDatabase->GetGUIDFromFilePath(TARGET_BOX_TEXTURE_PATH);
		}

		// 実体の到着は待たない。描画側が IsReady を見てスキップする
		RequestDecorationResources(a_context);
	}

	//======================================================================================
	// その群の飾りを持っているか
	//======================================================================================
	bool TargetBoxHUD::HasDecorationGroup(uint32_t a_group) const
	{
		for (const Decoration::Decoration& _decoration : m_decorationVec)
		{
			if (_decoration.group == a_group) return true;
		}
		return false;
	}

	void TargetBoxHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// 飾りのアニメーションを進める
		UIBase::Update(a_context);

		// このフレームぶんを作り直す
		m_targetScreenPosVec.clear();
		m_isLocked = false;

		auto* _pWorld = a_context.pWorld;
		if (!_pWorld) return;

		//==================================================================
		// プレイヤーのロック結果を読む
		//------------------------------------------------------------------
		// 射影(ワールド→スクリーン)もレティクル内の判定も LockOnTargetSystem が
		// PostUpdate で済ませている。GameObjectManager::Update はその後なので、
		// ここでは同じフレームの確定済みの結果をそのまま使える。
		//==================================================================
		bool _hasPlayer = false;

		_pWorld->ForEach<const ActiveTag, const PlayerControllTag, const LockOnTargetComponent>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const PlayerControllTag* a_playerTagArray,
				const LockOnTargetComponent* a_lockOnArray
			)
			{
				// 操作しているプレイヤーは1体の想定。先に見つかったものを使う
				if (_hasPlayer || a_count == 0) return;
				_hasPlayer = true;

				const LockOnTargetComponent& _lockOn = a_lockOnArray[0];

				const int _count = std::clamp(
					_lockOn.targetCount, 0, LockOnTargetComponent::TARGET_MAX);

				for (int _i = 0; _i < _count; ++_i)
				{
					// ロック中の相手は赤い枠で別に描くので、黄色の枠からは外す
					if (_lockOn.IsLocked() && _lockOn.targets[_i] == _lockOn.lockedEntity) continue;

					m_targetScreenPosVec.push_back(Math::Vector2(_lockOn.screenPos[_i]));
				}

				if (_lockOn.IsLocked())
				{
					m_lockedScreenPos = Math::Vector2(_lockOn.lockedScreenPos);
					m_isLocked = true;
				}
			}
		);
	}

	void TargetBoxHUD::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_targetScreenPosVec.empty() && !m_isLocked) return;

		//--------------------------------------------------------------
		// 画面内の敵 : 群 0 の飾りを、位置だけ差し替えて敵の数ぶん出す
		//--------------------------------------------------------------
		Decoration::DrawOverride _normal = {};
		_normal.isUsePos = true;
		_normal.isUseGroup = true;
		_normal.group = GROUP_NORMAL;

		for (const Math::Vector2& _screenPos : m_targetScreenPosVec)
		{
			_normal.pixelPos = _screenPos;
			DrawDecorations(a_context, _normal);
		}

		if (!m_isLocked) return;

		//--------------------------------------------------------------
		// ロック中の相手
		//
		// 専用の飾り(群 1)があればそれを、無ければ通常枠を LockColor で染めて代用する。
		// 枠が消えてしまうより、色が変わったほうがロックされたことが分かるため
		//--------------------------------------------------------------
		const bool _hasLockDecoration = HasDecorationGroup(GROUP_LOCK);

		Decoration::DrawOverride _lock = {};
		_lock.isUsePos = true;
		_lock.pixelPos = m_lockedScreenPos;
		_lock.scale = m_lockSizeScale;
		_lock.tint = m_lockColor;
		_lock.isUseGroup = true;
		_lock.group = _hasLockDecoration ? GROUP_LOCK : GROUP_NORMAL;

		DrawDecorations(a_context, _lock);
	}

	void TargetBoxHUD::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// テクスチャ・色・サイズなどの共通ぶん
		UIBase::Archive(a_ar, a_context);

		// 旧形式(ロック枠テクスチャ1枚)の名残。並びを変えないため読み書きは続ける
		a_ar.GUIDField("LockTexGUID", m_legacyLockTexGUID);
		a_ar.Field("LockSizeScale", m_lockSizeScale);
		a_ar.Field("LockColor", m_lockColor);

		if (!a_ar.IsLoading()) return;

		//--------------------------------------------------------------
		// 旧形式からの引き継ぎ
		// ロック枠のテクスチャを持っていたシーンは、群 1 の飾りへ移し替える
		//--------------------------------------------------------------
		if (m_legacyLockTexGUID.IsValid() && !HasDecorationGroup(GROUP_LOCK))
		{
			Decoration::Decoration& _lockBox = AddDecoration(Decoration::EDecorationType::Image);
			_lockBox.name = "LockBox";
			_lockBox.group = GROUP_LOCK;
			_lockBox.pixelSize = m_pixelSize;
			_lockBox.texGUID = m_legacyLockTexGUID;

			RequestDecorationResources(a_context);
		}
	}

	void TargetBoxHUD::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("TargetBox");

		if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

		// ロック枠へ掛ける色。飾りの色へ乗算で乗る
		Engine::Editor::EditorHelper::DrawColorEdit("LockColor", m_lockColor);
		ImGui::DragFloat("LockSizeScale", &m_lockSizeScale, 0.01f, 0.0f, 8.0f);

		ImGui::TextDisabled("飾りの Group : 0 = 通常枠 / 1 = ロック枠");
		ImGui::Text("Lock decoration : %s",
			HasDecorationGroup(GROUP_LOCK) ? "yes" : "no (通常枠を LockColor で代用)");

		// 枠は画面内の敵すべてに出る。
		// ロック(赤枠)の判定半径と距離はプレイヤー側(LockOnTargetComponent)の設定
		ImGui::TextDisabled("Boxes : every enemy on screen (within MaxDistance)");
		ImGui::TextDisabled("Lock radius / range : Player's LockOnTargetComponent");
		ImGui::TextDisabled("PixelPos is unused (follows enemies)");
		ImGui::Text("Boxes : %d%s",
			static_cast<int>(m_targetScreenPosVec.size()),
			m_isLocked ? " (+lock)" : "");
	}
}
