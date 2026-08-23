#include "SequenceBgm.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/Audio/AudioManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

namespace App::Object
{
	namespace
	{
		//----------------------------------------------------------------------------------
		// 全体の絞り
		//
		// ポーズ画面は重ねて出るので、下のシーンの進行役へは手が届かない。
		// しかも重ねている間、下のシーンは更新されない = 下のBGMは自分では
		// 音量を送り直せないので、生存中のものをここで数えておいて送り込む
		//----------------------------------------------------------------------------------
		float g_globalDuck = 1.0f;
		std::vector<SequenceBgm*> g_livingBgmVec = {};
	}

	SequenceBgm::SequenceBgm()
	{
		g_livingBgmVec.push_back(this);
	}

	SequenceBgm::~SequenceBgm()
	{
		const auto _it = std::find(g_livingBgmVec.begin(), g_livingBgmVec.end(), this);
		if (_it != g_livingBgmVec.end()) g_livingBgmVec.erase(_it);
	}

	void SequenceBgm::SetGlobalDuck(float a_scale)
	{
		const float _scale = std::clamp(a_scale, 0.0f, 1.0f);
		if (g_globalDuck == _scale) return;

		g_globalDuck = _scale;

		// 更新が止まっているシーンのBGMにも効かせるため、その場で送り込む
		for (SequenceBgm* _pBgm : g_livingBgmVec)
		{
			if (_pBgm) _pBgm->ApplyVolume();
		}
	}

	float SequenceBgm::GetGlobalDuck()
	{
		return g_globalDuck;
	}

	void SequenceBgm::SetDuckTarget(bool a_isDuckTarget)
	{
		if (m_isDuckTarget == a_isDuckTarget) return;

		m_isDuckTarget = a_isDuckTarget;
		ApplyVolume();
	}

	//======================================================================================
	// 更新
	//======================================================================================
	void SequenceBgm::Update(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices || !a_context.pServices->pAudioManager) return;

		// 更新の外からも音量を送れるよう覚えておく
		m_pAudioManager = a_context.pServices->pAudioManager;

		//------------------------------------------------------------------
		// 鳴らし始め
		//
		// Init ではなくここで始めるのは、Init が Archive より先に走るため。
		// Init の時点ではまだ曲が決まっていない
		//------------------------------------------------------------------
		if (!m_isStarted)
		{
			if (m_isFailed) return;
			if (!m_guid.IsValid()) return;

			// 画面に流す音なので 2D で発行する(定位を付けない)。
			// BGM の札を付けておくと、設定画面の BGM 音量がそのまま効く
			m_handle = m_pAudioManager->RequestSoundInstance(
				m_guid, false, Engine::Audio::ESoundGroup::Bgm);

			auto* _pInstance = m_pAudioManager->RefInstance(m_handle);
			if (!_pInstance)
			{
				// 読めなかった。毎フレーム読み直すと重いので、ここで諦める
				ENGINE_WARNING("[SequenceBgm] BGMを発行できませんでした");
				m_isFailed = true;
				return;
			}

			// フェードインの起点。先に0にしてから鳴らさないと頭が出てしまう
			_pInstance->SetVolume(0.0f);
			_pInstance->Play(m_isLoop);

			m_isStarted = true;
			m_fadeTime = 0.0f;
			m_appliedVolume = 0.0f;
		}

		// フェードを進める
		if (m_fadeInTime > 0.0f)
		{
			m_fadeTime = std::min(m_fadeTime + a_context.dt, m_fadeInTime);
		}

		ApplyVolume();
	}

	//======================================================================================
	// 今の音量を送る
	//--------------------------------------------------------------------------------------
	// 変わったときだけ送る。毎フレーム送っても実害は無いが、
	// 絞りもフェードも動いていない間は何もしないほうが追いやすい
	//======================================================================================
	void SequenceBgm::ApplyVolume()
	{
		if (!m_isStarted || m_pAudioManager == nullptr) return;

		const float _volume = CalcVolume();
		if (std::fabs(_volume - m_appliedVolume) <= 1e-3f) return;

		auto* _pInstance = m_pAudioManager->RefInstance(m_handle);
		if (!_pInstance) return;

		_pInstance->SetVolume(_volume);
		m_appliedVolume = _volume;
	}

	//======================================================================================
	// 今送るべき音量
	//======================================================================================
	float SequenceBgm::CalcVolume() const
	{
		// フェードインの進み具合(0〜1)
		float _fadeRate = 1.0f;
		if (m_fadeInTime > 0.0f)
		{
			_fadeRate = std::clamp(m_fadeTime / m_fadeInTime, 0.0f, 1.0f);
		}

		const float _duck = m_isDuckTarget ? g_globalDuck : 1.0f;

		return std::clamp(m_volume * _fadeRate * _duck, 0.0f, 1.0f);
	}

	//======================================================================================
	// 解放
	//======================================================================================
	void SequenceBgm::Release(Engine::GameObject::ObjectContext& a_context)
	{
		// サウンドインスタンスのプールはアプリ寿命なので、借りた側が必ず返す
		if (a_context.pServices && a_context.pServices->pAudioManager)
		{
			a_context.pServices->pAudioManager->ReleaseSoundInstance(m_handle);
		}

		m_handle = {};
		m_pAudioManager = nullptr;
		m_fadeTime = 0.0f;
		m_isStarted = false;
		m_isFailed = false;
		m_appliedVolume = -1.0f;
	}

	//======================================================================================
	// シリアライズ
	//======================================================================================
	void SequenceBgm::Archive(Engine::Persistence::Archive& a_ar)
	{
		a_ar.GUIDField("BgmGUID", m_guid);
		a_ar.Field("BgmVolume", m_volume);
		a_ar.Field("BgmFadeInTime", m_fadeInTime);
		a_ar.Field("BgmIsLoop", m_isLoop);
		a_ar.Field("BgmIsDuckTarget", m_isDuckTarget);
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void SequenceBgm::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		ImGui::SeparatorText("BGM");

		// 曲を差し替えたら、借りている分を返して鳴らし直させる
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Bgm", "Sound", m_guid))
		{
			Release(a_context);
		}

		if (ImGui::DragFloat("BgmVolume", &m_volume, 0.01f, 0.0f, 1.0f))
		{
			// 鳴らしながら合わせられるよう、その場で送り直す
			ApplyVolume();
		}

		ImGui::DragFloat("BgmFadeInTime", &m_fadeInTime, 0.05f, 0.0f, 20.0f);
		ImGui::TextDisabled("鳴り始めに音量を上げきるまでの時間(秒)。0で即時");

		ImGui::Checkbox("BgmLoop", &m_isLoop);
		ImGui::SameLine();

		bool _isDuckTarget = m_isDuckTarget;
		if (ImGui::Checkbox("BgmDuckTarget", &_isDuckTarget)) SetDuckTarget(_isDuckTarget);
		ImGui::SetItemTooltip("ポーズ中の絞りを受けるか。ポーズ自身のBGMは切ること");

		// 鳴らし直し : 曲を変えずに頭から確かめたいとき用
		if (Engine::Editor::EditorHelper::CreateButton("Replay Bgm"))
		{
			Release(a_context);
		}

		ImGui::SameLine();
		ImGui::Text("Playing : %s", m_isStarted ? "yes" : (m_isFailed ? "failed" : "no"));
	}
}
