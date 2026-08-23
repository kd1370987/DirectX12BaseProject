#include "HitEffectHUD.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Audio/AudioManager.h"
#include "Engine/Option/OptionManager.h"			// ウィンドウ解像度(px)取得用

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/InstanceResource/HitEventResource.h"
#include "Application/Components/Character/HealthComponent.h"

namespace App::Object
{
	namespace
	{
		// 新規追加時の既定表示サイズ(px)
		constexpr float DEFAULT_MARK_SIZE = 64.0f;
	}

	void HitEffectHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices) return;
		if (!a_context.pServices->pOptionManager || !a_context.pServices->pResourceManager) return;

		// 新規追加直後はサイズが0で何も見えないので、既定サイズと画面中央を入れておく。
		// シーン読み込み時はこの後の Archive で保存値に上書きされる。
		if (m_pixelSize.x <= 0.0f || m_pixelSize.y <= 0.0f)
		{
			m_pixelSize = { DEFAULT_MARK_SIZE, DEFAULT_MARK_SIZE };
			m_editSize  = m_pixelSize;

			const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
			m_pixelPos = {
				static_cast<float>(_winOp.windowWidth) * 0.5f,
				static_cast<float>(_winOp.windowHeight) * 0.5f
			};
		}

		// テクスチャ(クロスマーク)は差し替え前提なので既定パスは持たない
		if (m_texGUID.IsValid())
		{
			m_texRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_texGUID);
		}

		RequestSound(a_context);
	}

	void HitEffectHUD::Release(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::Release(a_context);

		// サウンドインスタンスのプールはアプリ寿命なので、借りた側が必ず返す
		if (a_context.pServices && a_context.pServices->pAudioManager)
		{
			a_context.pServices->pAudioManager->ReleaseSoundInstance(m_soundHandle);
		}
		m_soundHandle = {};
	}

	void HitEffectHUD::RequestSound(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices || !a_context.pServices->pAudioManager) return;

		auto* _pAudioManager = a_context.pServices->pAudioManager;

		// 借りていたものは先に返す(差し替えのたびに溜まらないように)
		_pAudioManager->ReleaseSoundInstance(m_soundHandle);
		m_soundHandle = {};

		if (m_soundGUID == Engine::DefaultGUID) return;

		// 画面に出す音なので 2D で発行する(定位を付けない)
		m_soundHandle = _pAudioManager->RequestSoundInstance(m_soundGUID, false);

		if (auto* _pInstance = _pAudioManager->RefInstance(m_soundHandle))
		{
			_pInstance->SetVolume(m_volume);
		}
	}

	void HitEffectHUD::OnHit(Engine::GameObject::ObjectContext& a_context)
	{
		++m_hitCount;

		// 出ている最中に当たったら、そこから出し直す
		m_remainTime = m_showTime;

		// 間引き中は鳴らさない(連射で頭出しを繰り返して潰れるのを防ぐ)
		if (m_coolTime > 0.0f) return;
		m_coolTime = m_minInterval;

		if (!a_context.pServices || !a_context.pServices->pAudioManager) return;

		if (auto* _pInstance = a_context.pServices->pAudioManager->RefInstance(m_soundHandle))
		{
			_pInstance->SetVolume(m_volume);
			_pInstance->Play(false);
		}
	}

	void HitEffectHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// 表示時間と間引きを進める
		if (m_remainTime > 0.0f) m_remainTime = std::max(m_remainTime - a_context.dt, 0.0f);
		if (m_coolTime > 0.0f)   m_coolTime   = std::max(m_coolTime - a_context.dt, 0.0f);

		auto* _pWorld = a_context.pWorld;
		if (!_pWorld) return;
		if (!_pWorld->HasResource<HitEventResource>()) return;

		const HitEventResource& _hitEvents = _pWorld->GetResource<HitEventResource>();
		if (_hitEvents.events.empty()) return;

		//==================================================================
		// 操作しているプレイヤーを引く
		//------------------------------------------------------------------
		// 「自分が発した弾か」は HitEvent.shooter(弾を撃った本体)で見る。
		// attacker は弾そのものなので、そちらでは判定できない。
		//==================================================================
		Engine::ECS::Entity _player = Engine::ECS::Limits::INVALID_ENTITY;

		_pWorld->ForEach<const ActiveTag, const PlayerControllTag>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const PlayerControllTag* a_playerTagArray
			)
			{
				if (_player != Engine::ECS::Limits::INVALID_ENTITY || a_count == 0) return;
				_player = a_pChunk->entityData[0];
			}
		);

		if (_player == Engine::ECS::Limits::INVALID_ENTITY) return;

		// 同じフレームに複数当たっても、出し直しは1回でよい
		for (const HitEvent& _event : _hitEvents.events)
		{
			if (_event.shooter != _player) continue;

			//--------------------------------------------------------------
			// 手応えを出すのは「ダメージが通る相手」に当てたときだけ
			//
			// 弾は壁でも地面でも味方の部品でも同じように当たる。
			// そこまで音とマークが出ると、何に当てても当たった気になってしまい、
			// 狙う価値のある相手に当てた合図として働かなくなる。
			//
			// 見るのは HealthComponent の有無。
			// 以前は ScoreTargetComponent(点数を持っている印)で見ていたが、
			// あれは「倒したら何点入るか」を表す得点側の都合で、
			// 手応えを出すかどうかとは別の話。実際どのプレハブにも付いておらず、
			// 条件が一度も成立しないためマーカーが出ていなかった。
			//--------------------------------------------------------------
			if (_event.victim == Engine::ECS::Limits::INVALID_ENTITY) continue;
			if (!_pWorld->HasComponent<HealthComponent>(_event.victim)) continue;

			OnHit(a_context);
			break;
		}
	}

	void HitEffectHUD::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_remainTime <= 0.0f) return;
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		// 残り時間の割合(1 → 0)。出た瞬間が 1
		const float _rate = (m_showTime > 1e-4f)
			? std::clamp(m_remainTime / m_showTime, 0.0f, 1.0f)
			: 1.0f;

		// 消えぎわに薄くする
		Math::Color _color = m_color;
		if (m_isFadeOut) _color.a *= _rate;

		// 出た瞬間だけ少し大きく見せる(punchScale → 等倍へ戻る)
		const float _scale = 1.0f + (m_punchScale - 1.0f) * _rate;

		_pGE->SubmitUI(
			m_texRef,
			m_pixelPos,
			m_pixelSize * _scale,
			_color,
			m_rotation,
			m_layer,
			m_uvOffset,
			m_pivot
		);
	}

	void HitEffectHUD::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// テクスチャ・色・サイズなどの共通ぶん
		UIBase::Archive(a_ar, a_context);

		a_ar.GUIDField("SoundGUID", m_soundGUID);
		a_ar.Field("Volume", m_volume);
		a_ar.Field("ShowTime", m_showTime);
		a_ar.Field("MinInterval", m_minInterval);
		a_ar.Field("IsFadeOut", m_isFadeOut);
		a_ar.Field("PunchScale", m_punchScale);

		// 読み込み時は復元したGUIDでサウンドを取り直す
		if (a_ar.IsLoading())
		{
			RequestSound(a_context);
		}
	}

	void HitEffectHUD::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("HitEffect");

		// ヒット音(アセットDBの Sound 一覧から選ぶ)
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
			"Hit Sound",
			"Sound",
			m_soundGUID))
		{
			RequestSound(a_context);
		}

		if (ImGui::DragFloat("Volume", &m_volume, 0.01f, 0.0f, 1.0f))
		{
			// 鳴らしながら調整できるよう、発行済みインスタンスへ即時反映する
			if (a_context.pServices && a_context.pServices->pAudioManager)
			{
				if (auto* _pInstance = a_context.pServices->pAudioManager->RefInstance(m_soundHandle))
				{
					_pInstance->SetVolume(m_volume);
				}
			}
		}

		ImGui::Separator();
		ImGui::DragFloat("ShowTime", &m_showTime, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("MinInterval", &m_minInterval, 0.01f, 0.0f, 1.0f);
		ImGui::Checkbox("FadeOut", &m_isFadeOut);
		ImGui::DragFloat("PunchScale", &m_punchScale, 0.01f, 0.1f, 4.0f);

		// 確認用に鳴らしてみる
		if (ImGui::Button("Test")) OnHit(a_context);

		ImGui::Separator();
		ImGui::Text("HitCount : %d", m_hitCount);
		ImGui::Text("Remain   : %.2f", m_remainTime);
		ImGui::TextDisabled("自分が撃った弾が ScoreTarget に当たったフレームに反応します");
	}
}
