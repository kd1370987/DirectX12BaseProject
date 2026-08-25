#include "AudioBehaviorEdit.h"

#include "../../../../../Helper/EditorHelper.h"

#include "../../AssetLink.h"
#include "../../../../../../Audio/AudioManager.h"
#include "../../../../../../Resource/Manager/AssetDatabase/AssetDatabase.h"

namespace Engine::Editor::Inspector
{
	namespace
	{
		//-----------------------------------------------------------------------------------------
		// 試聴用の実体
		//
		// 実際に鳴らしてみないと音のつながりは確かめられないので、
		// 編集中のアセット1つぶんだけインスタンスを持っておく。
		// 別のアセットへ移ったら作り直す(同時に鳴るのは常に1つ)
		//-----------------------------------------------------------------------------------------
		Engine::GUID						g_previewGUID = Engine::DefaultGUID;
		Resource::AudioBehaviorInstance		g_previewInstance = {};

		// 試聴用インスタンスを、今編集しているアセットのものに合わせる
		void SyncPreview(
			Engine::Audio::AudioManager& a_audioManager,
			Resource::AudioBehavior* a_pBehavior,
			const Engine::GUID& a_guid,
			bool a_isForce
		)
		{
			if (!a_isForce && g_previewGUID == a_guid) return;

			g_previewInstance.Release(a_audioManager);
			a_pBehavior->CreateInstance(a_audioManager, g_previewInstance);
			g_previewGUID = a_guid;
		}

		//-----------------------------------------------------------------------------------------
		// 1フェーズ分の編集UI
		//
		// 音が未設定でも編集欄はそのまま出す。
		// 「空欄なら鳴らさない」がこのアセットの前提なので、
		// 設定していない状態も正しい状態として見せる
		//-----------------------------------------------------------------------------------------
		bool SoundPartEdit(EditorContext& a_editContext, Resource::SoundPart& a_part, Resource::EAudioPhase a_phase)
		{
			bool _isChanged = false;

			ImGui::PushID(static_cast<int>(a_phase));

			// 音の差し替え
			if (EditorHelper::DrawAssetSelectComboGUID("Sound", "Sound", a_part.soundGUID))
			{
				_isChanged = true;
			}

			// 割り当てた音を出しておく(GUIDだけでは何の音か分からないため)。
			// そのまま押せば音アセットのインスペクターへ飛べる
			if (a_part.IsValid())
			{
				DrawAssetLink(&a_editContext, "", a_part.soundGUID);

				ImGui::SameLine();
				if (EditorHelper::DeleteButton("Clear"))
				{
					a_part.soundGUID = Engine::DefaultGUID;
					_isChanged = true;
				}
			}
			else
			{
				// 空欄はエラーではないことを明示しておく
				ImGui::TextDisabled("(empty : このフェーズは鳴らさない)");
			}

			if (ImGui::DragFloat("Volume", &a_part.vol, 0.01f, 0.0f, 1.0f))
			{
				_isChanged = true;
			}

			if (ImGui::Checkbox("3D Sound", &a_part.is3DSound))
			{
				_isChanged = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("(鳴らす側が位置を送る)");

			ImGui::PopID();

			return _isChanged;
		}
	}

	//-----------------------------------------------------------------------------------------
	// オーディオビヘイビアの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void AudioBehaviorEdit(EditorContext& a_editContext, Resource::AudioBehavior* a_pBehavior)
	{
		if (!a_pBehavior) { return; }

		const auto _guid = a_editContext.pAssetProp->guid;

		ImGui::Text("Audio Behavior : %s", a_pBehavior->GetName().c_str());
		ImGui::Separator();

		// 保存ボタン
		if (ImGui::Button("Save Asset"))
		{
			auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			a_pBehavior->Save(_filePath);
			ENGINE_LOG("Save AudioBehavior : %s", _filePath.c_str());
		}

		ImGui::Spacing();

		auto& _audioManager = Engine::Audio::AudioManager::Instance();

		// 編集対象が変わっていたら試聴用を作り直す
		SyncPreview(_audioManager, a_pBehavior, _guid, false);

		//------------------------------------------------------------------
		// 試聴
		//
		// 実際の呼ばれ方(始動 → 継続 → 終了)と同じ順で押せるようにしてある
		//------------------------------------------------------------------
		ImGui::Text("Preview");

		// 下のフェーズ見出しと同じ文字列(Start/Loop/End)を使うので、
		// ImGuiのIDがぶつからないようにここだけ別スコープにする
		// (IDのもとはラベル文字列なので、同名の項目は同じIDになる)
		ImGui::PushID("Preview");
		if (ImGui::Button("Start")) { a_pBehavior->Start(_audioManager, g_previewInstance); }
		ImGui::SameLine();
		if (ImGui::Button("Loop")) { a_pBehavior->Loop(_audioManager, g_previewInstance); }
		ImGui::SameLine();
		if (ImGui::Button("End")) { a_pBehavior->End(_audioManager, g_previewInstance); }
		ImGui::SameLine();
		if (ImGui::Button("Stop")) { g_previewInstance.StopAll(_audioManager); }
		ImGui::PopID();

		// 3D指定のパートは試聴では位置を送れないので、原点で鳴ることを断っておく
		ImGui::TextDisabled("3D指定の音は原点で鳴ります");

		ImGui::Separator();

		//------------------------------------------------------------------
		// フェーズごとの割り当て
		//------------------------------------------------------------------
		bool _isChanged = false;

		for (size_t _i = 0; _i < Resource::AUDIO_PHASE_COUNT; ++_i)
		{
			const auto _phase = static_cast<Resource::EAudioPhase>(_i);

			// 中身が入っているフェーズは開いた状態で出す
			auto _flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed;
			if (a_pBehavior->HasPart(_phase)) _flags |= ImGuiTreeNodeFlags_DefaultOpen;

			if (!ImGui::TreeNodeEx(Resource::ToString(_phase), _flags)) continue;

			switch (_phase)
			{
			case Resource::EAudioPhase::Start:
				ImGui::TextDisabled("始動した瞬間に一度だけ鳴る");
				break;
			case Resource::EAudioPhase::Loop:
				ImGui::TextDisabled("続いている間ループする");
				break;
			case Resource::EAudioPhase::End:
				ImGui::TextDisabled("終わった瞬間に一度だけ鳴る");
				break;
			default:
				break;
			}

			if (SoundPartEdit(a_editContext, a_pBehavior->RefPart(_phase), _phase))
			{
				_isChanged = true;
			}

			ImGui::TreePop();
		}

		// 割り当てを変えたら試聴用も作り直す。
		// (音量だけの変更でも作り直す : 発行時に音量を入れているため)
		if (_isChanged)
		{
			SyncPreview(_audioManager, a_pBehavior, _guid, true);
		}
	}
}
