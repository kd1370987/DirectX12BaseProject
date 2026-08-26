#include "CombatReticleHUD.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"
#include "Engine/Option/OptionManager.h"	// ウィンドウ解像度(px)取得用

#include "Application/ECS/World/World.h"

#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Character/Weapon/Missile/MissileLockComponent.h"

namespace App::Object
{
	namespace
	{
		// テスト用レティクルテクスチャのパス
		constexpr const char* RETICLE_TEXTURE_PATH = "Asset/Texture/Test/uiTest.png";
	}

	void CombatReticleHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		// リソース周りはコンテキストが運んできたサービスを使う
		if (!a_context.pServices) return;
		if (!a_context.pServices->pAssetDatabase || !a_context.pServices->pResourceManager) return;

		// 既定のレティクルを1つ用意する。作るのは飾りを1つも持っていないときだけで、
		// シーン読み込み時はこの後の Archive が保存された飾りで置き換える
		if (m_decorationVec.empty())
		{
			Decoration::Decoration& _reticle = AddDecoration(Decoration::EDecorationType::Image);
			_reticle.name = "Reticle";
			_reticle.pixelSize = m_pixelSize;
			_reticle.texGUID = a_context.pServices->pAssetDatabase->GetGUIDFromFilePath(RETICLE_TEXTURE_PATH);
		}

		// 実体の到着は待たない。描画側が IsReady を見てスキップする
		RequestDecorationResources(a_context);
	}

	float CombatReticleHUD::CalcCollectRadius() const
	{
		// 縦横で違う場合は小さい方。円としてはみ出さない側に合わせる
		const float _half = std::min(m_pixelSize.x, m_pixelSize.y) * 0.5f;
		return std::max(_half, 0.0f);
	}

	void CombatReticleHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// 飾りのアニメーションを進める
		UIBase::Update(a_context);

		auto* _pWorld = a_context.pWorld;
		if (!_pWorld) return;

		const Math::Vector2 _center = m_pixelPos;
		const float         _radius = CalcCollectRadius();

		//==================================================================
		// ミサイルの収集円をプレイヤーへ渡す
		//------------------------------------------------------------------
		// 読むのは MissileSalvoSystem(PostUpdate)。GameObjectManager::Update は
		// その後なので、実際に使われるのは次のフレーム。動かない値なので差は出ない。
		//==================================================================
		bool _isWritten = false;

		_pWorld->ForEach<const ActiveTag, const PlayerControllTag, MissileLockComponent>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const PlayerControllTag* a_playerTagArray,
				MissileLockComponent* a_missileArray
			)
			{
				// 操作しているプレイヤーは1体の想定。先に見つかったものへ渡す
				if (_isWritten || a_count == 0) return;
				_isWritten = true;

				// 保存値(reticleRadius)は触らない。実行中に書き換えると
				// エディターで見ている設定値が UI の値に置き換わってしまう
				MissileLockComponent& _missile = a_missileArray[0];
				_missile.reticleCenter    = _center;
				_missile.hudReticleRadius = _radius;
				_missile.isReticleFromHUD = true;
			}
		);
	}

	void CombatReticleHUD::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Missile Lock");
		ImGui::Text("Collect radius : %.0f px", CalcCollectRadius());
		ImGui::TextDisabled("この円の内側に入った敵をミサイルが溜めます(アンカーの PixelSize に内接)");
		ImGui::TextDisabled("中心は PixelPos。倍率や弾数はプレイヤーの MissileLockComponent");
	}
}
