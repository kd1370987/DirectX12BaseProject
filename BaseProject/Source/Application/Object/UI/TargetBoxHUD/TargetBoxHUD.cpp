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

		// GUID未設定(新規追加)なら既定テクスチャを引く。
		// シーン読み込み時は Archive で復元済みのGUIDを尊重する。
		if (!m_texGUID.IsValid())
		{
			m_texGUID = a_context.pServices->pAssetDatabase->GetGUIDFromFilePath(TARGET_BOX_TEXTURE_PATH);
		}

		// 新規追加直後はサイズが0で何も見えないので、既定サイズを入れておく。
		// シーン読み込み時はこの後の Archive で保存値に上書きされる。
		if (m_pixelSize.x <= 0.0f || m_pixelSize.y <= 0.0f)
		{
			m_pixelSize = { DEFAULT_BOX_SIZE, DEFAULT_BOX_SIZE };
			m_editSize = m_pixelSize;
		}

		// 実体の到着は待たない。描画側が IsReady を見てスキップする
		if (m_texGUID.IsValid())
		{
			m_texRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_texGUID);
		}

		// ロック枠は差し替え前提なので既定パスは持たない。
		// 未設定の間はロック中でも通常枠のテクスチャで描く。
		if (m_lockTexGUID.IsValid())
		{
			m_lockTexRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_lockTexGUID);
		}
	}

	void TargetBoxHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
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
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		// 見た目(サイズ・色・回転など)は全ボックス共通。位置だけ敵ごとに差し替える
		for (const Math::Vector2& _screenPos : m_targetScreenPosVec)
		{
			_pGE->SubmitUI(
				m_texRef,
				_screenPos,
				m_pixelSize,
				m_color,
				m_rotation,
				m_layer,
				m_uvOffset,
				m_pivot
			);
		}

		// ロック中の相手だけロック用テクスチャで上に描く。
		// 未設定なら通常のテクスチャで代用する(枠が消えるより分かりやすい)
		if (m_isLocked)
		{
			const auto& _lockTex = m_lockTexGUID.IsValid() ? m_lockTexRef : m_texRef;

			_pGE->SubmitUI(
				_lockTex,
				m_lockedScreenPos,
				m_pixelSize * m_lockSizeScale,
				m_lockColor,
				m_rotation,
				m_layer,
				m_uvOffset,
				m_pivot
			);
		}
	}

	void TargetBoxHUD::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// テクスチャ・色・サイズなどの共通ぶん
		UIBase::Archive(a_ar, a_context);

		a_ar.GUIDField("LockTexGUID", m_lockTexGUID);
		a_ar.Field("LockSizeScale", m_lockSizeScale);
		a_ar.Field("LockColor", m_lockColor);

		// 読み込み時は復元したGUIDでロック枠のテクスチャを引き直す
		if (a_ar.IsLoading())
		{
			if (!m_lockTexGUID.IsValid()) return;
			if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

			m_lockTexRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_lockTexGUID);
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

		// 通常枠(画面内の敵)の色は UIBase の Color。ロック枠だけここで別に持つ
		Engine::Editor::EditorHelper::DrawColorEdit("LockColor", m_lockColor);

		// ロック中に使うテクスチャ(赤い枠)
		if (ImGui::CollapsingHeader("Lock Texture"))
		{
			if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
				"Change Lock Texture",
				"Texture",
				m_lockTexGUID))
			{
				m_lockTexRef =
					a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_lockTexGUID);
			}
			Engine::Editor::EditorHelper::DrawTexture(m_lockTexRef, 256, 256);
		}

		ImGui::DragFloat("LockSizeScale", &m_lockSizeScale, 0.01f, 0.0f, 8.0f);

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
