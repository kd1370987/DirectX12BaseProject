#include "AimReticleHUD.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Option/OptionManager.h"			// ウィンドウ解像度(px)取得用

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Character/LockOnTargetComponent.h"

namespace App::Object
{
	namespace
	{
		// 新規追加時の既定表示サイズ(px)
		constexpr float DEFAULT_RETICLE_SIZE = 160.0f;
	}

	void AimReticleHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices) return;
		if (!a_context.pServices->pOptionManager || !a_context.pServices->pResourceManager) return;

		// 新規追加直後はサイズが0で何も見えないので、既定サイズと画面中央を入れておく。
		// シーン読み込み時はこの後の Archive で保存値に上書きされる。
		if (m_pixelSize.x <= 0.0f || m_pixelSize.y <= 0.0f)
		{
			m_pixelSize = { DEFAULT_RETICLE_SIZE, DEFAULT_RETICLE_SIZE };
			m_editSize  = m_pixelSize;

			const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
			m_pixelPos = {
				static_cast<float>(_winOp.windowWidth) * 0.5f,
				static_cast<float>(_winOp.windowHeight) * 0.5f
			};
		}

		// テクスチャは差し替え前提なので既定パスは持たない。
		// 実体の到着は待たない(描画側が IsReady を見てスキップする)
		if (m_texGUID.IsValid())
		{
			m_texRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_texGUID);
		}
	}

	float AimReticleHUD::CalcLockRadius() const
	{
		if (!m_isUseTextureSize) return std::max(m_lockRadius, 0.0f);

		// 縦横で違う場合は小さい方。円としてはみ出さない側に合わせる
		const float _half = std::min(m_pixelSize.x, m_pixelSize.y) * 0.5f;
		return std::max(_half * m_radiusScale, 0.0f);
	}

	void AimReticleHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		auto* _pWorld = a_context.pWorld;
		if (!_pWorld) return;

		const Math::Vector2 _center = m_pixelPos;
		const float         _radius = CalcLockRadius();

		//==================================================================
		// 判定の円をプレイヤーへ渡す
		//------------------------------------------------------------------
		// 読むのは LockOnTargetSystem(PostUpdate)。GameObjectManager::Update は
		// その後なので、実際に使われるのは次のフレーム。動かない値なので差は出ない。
		//==================================================================
		bool _isWritten = false;

		_pWorld->ForEach<const ActiveTag, const PlayerControllTag, LockOnTargetComponent>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const PlayerControllTag* a_playerTagArray,
				LockOnTargetComponent* a_lockOnArray
			)
			{
				// 操作しているプレイヤーは1体の想定。先に見つかったものへ渡す
				if (_isWritten || a_count == 0) return;
				_isWritten = true;

				// 保存値(reticleRadius)は触らない。実行中に書き換えると
				// エディターで見ている設定値が UI の値に置き換わってしまう
				LockOnTargetComponent& _lockOn = a_lockOnArray[0];
				_lockOn.reticleCenter    = _center;
				_lockOn.hudReticleRadius = _radius;
				_lockOn.isReticleFromHUD = true;
			}
		);
	}

	void AimReticleHUD::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// テクスチャ・色・サイズなどの共通ぶん
		UIBase::Archive(a_ar, a_context);

		a_ar.Field("IsUseTextureSize", m_isUseTextureSize);
		a_ar.Field("RadiusScale", m_radiusScale);
		a_ar.Field("LockRadius", m_lockRadius);
	}

	void AimReticleHUD::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("AutoAim");

		// 判定半径の作り方
		ImGui::Checkbox("UseTextureSize", &m_isUseTextureSize);
		if (m_isUseTextureSize)
		{
			ImGui::DragFloat("RadiusScale", &m_radiusScale, 0.01f, 0.0f, 4.0f);
		}
		else
		{
			ImGui::DragFloat("LockRadius", &m_lockRadius, 1.0f, 0.0f, 4096.0f);
		}

		ImGui::Text("Radius : %.0f px", CalcLockRadius());
		ImGui::TextDisabled("この円の内側に入った敵だけがロック対象になります");
		ImGui::TextDisabled("(中心は PixelPos。プレイヤーの LockOnTargetComponent へ毎フレーム渡します)");
	}
}
