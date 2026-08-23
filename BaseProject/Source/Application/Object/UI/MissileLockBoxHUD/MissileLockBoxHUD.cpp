#include "MissileLockBoxHUD.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Character/Weapon/Missile/MissileLockComponent.h"

namespace App::Object
{
	namespace
	{
		// 既定の枠テクスチャのパス
		constexpr const char* LOCK_BOX_TEXTURE_PATH = "Asset/Texture/Reticle/Robot01.png";

		// 新規追加時の既定表示サイズ(px)
		constexpr float DEFAULT_BOX_SIZE = 80.0f;
	}

	void MissileLockBoxHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		// リソース周りはコンテキストが運んできたサービスを使う
		if (!a_context.pServices) return;
		if (!a_context.pServices->pAssetDatabase || !a_context.pServices->pResourceManager) return;

		// 新規追加直後はサイズが0で何も見えないので、既定サイズと色を入れておく。
		// シーン読み込み時はこの後の Archive で保存値に上書きされる。
		if (m_pixelSize.x <= 0.0f || m_pixelSize.y <= 0.0f)
		{
			m_pixelSize = { DEFAULT_BOX_SIZE, DEFAULT_BOX_SIZE };
			m_editSize  = m_pixelSize;

			// ミサイルの溜めは黄色の枠
			m_color = Math::Color(1.0f, 1.0f, 0.0f, 1.0f);
		}

		// 既定の枠を1つ用意する。作るのは飾りを1つも持っていないときだけで、
		// シーン読み込み時はこの後の Archive が保存された飾りで置き換える
		if (m_decorationVec.empty())
		{
			Decoration::Decoration& _box = AddDecoration(Decoration::EDecorationType::Image);
			_box.name = "LockBox";
			_box.pixelSize = m_pixelSize;
			_box.texGUID = a_context.pServices->pAssetDatabase->GetGUIDFromFilePath(LOCK_BOX_TEXTURE_PATH);
		}

		// 実体の到着は待たない。描画側が IsReady を見てスキップする
		RequestDecorationResources(a_context);
	}

	void MissileLockBoxHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// 飾りのアニメーションを進める
		UIBase::Update(a_context);

		// このフレームぶんを作り直す
		m_lockScreenPosVec.clear();

		auto* _pWorld = a_context.pWorld;
		if (!_pWorld) return;

		//==================================================================
		// プレイヤーの溜め結果を読む
		//------------------------------------------------------------------
		// 射影も円の内外判定も MissileSalvoSystem が PostUpdate で済ませている。
		// GameObjectManager::Update はその後なので、ここでは同じフレームの
		// 確定済みの結果をそのまま使える。
		//==================================================================
		bool _hasPlayer = false;

		_pWorld->ForEach<const ActiveTag, const PlayerControllTag, const MissileLockComponent>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const PlayerControllTag* a_playerTagArray,
				const MissileLockComponent* a_missileArray
			)
			{
				// 操作しているプレイヤーは1体の想定。先に見つかったものを使う
				if (_hasPlayer || a_count == 0) return;
				_hasPlayer = true;

				const MissileLockComponent& _missile = a_missileArray[0];

				// 押している間だけ出す。撃った瞬間に溜めは捨てられるので枠も消える
				if (!_missile.isCharging) return;

				const int _count = std::clamp(
					_missile.lockCount, 0, MissileLockComponent::MISSILE_MAX);

				for (int _i = 0; _i < _count; ++_i)
				{
					m_lockScreenPosVec.push_back(Math::Vector2(_missile.lockScreenPos[_i]));
				}
			}
		);
	}

	void MissileLockBoxHUD::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_lockScreenPosVec.empty()) return;

		// 見た目は飾りそのまま。位置だけ敵ごとに差し替える
		Decoration::DrawOverride _override = {};
		_override.isUsePos = true;

		for (const Math::Vector2& _screenPos : m_lockScreenPosVec)
		{
			_override.pixelPos = _screenPos;
			DrawDecorations(a_context, _override);
		}
	}

	void MissileLockBoxHUD::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("MissileLockBox");
		ImGui::TextDisabled("ミサイルキーを押している間、溜めた敵を囲みます");
		ImGui::TextDisabled("収集範囲は CombatReticleHUD / 弾数はプレイヤーの MissileLockComponent");
		ImGui::TextDisabled("PixelPos is unused (follows enemies)");
		ImGui::Text("Boxes : %d", static_cast<int>(m_lockScreenPosVec.size()));
	}
}
