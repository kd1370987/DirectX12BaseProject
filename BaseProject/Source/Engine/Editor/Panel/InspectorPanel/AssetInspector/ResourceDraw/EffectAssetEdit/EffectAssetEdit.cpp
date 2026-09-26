#include "EffectAssetEdit.h"

#include "../../../../../Helper/EditorHelper.h"

#include "../../AssetLink.h"
#include "../../../../../EffectEditor/EffectEditor.h"
#include "../../../../../Editor.h"
#include "../../../../../../Resource/Manager/AssetDatabase/AssetDatabase.h"

namespace Engine::Editor::Inspector
{
	namespace
	{
		//-----------------------------------------------------------------------------------------
		// 割り当てたアセットを出す。GUIDだけでは何を指しているか分からないため
		//
		// アセットインスペクターから来ているときはリンクになり、押すと中身へ飛べる。
		// エフェクトエディターから来ているときは飛び先が無いので名前のまま
		//-----------------------------------------------------------------------------------------
		void DrawAssignedName(EditorContext* a_pEditContext, const Engine::GUID& a_guid)
		{
			if (a_guid == Engine::DefaultGUID)
			{
				// 空欄はエラーではないことを明示しておく
				Engine::Editor::HelpText("(empty : このパーツは出ない)");
				return;
			}

			DrawAssetLink(a_pEditContext, "", a_guid);
		}

		//-----------------------------------------------------------------------------------------
		// 時間指定 : いつ出て、どれだけ続くか
		//-----------------------------------------------------------------------------------------
		bool TimingEdit(Resource::EffectTiming& a_timing)
		{
			bool _isChanged = false;

			if (Engine::Editor::Field("StartDelay (s)", a_timing.startDelay, 0.01f, 0.0f)) _isChanged = true;
			if (Engine::Editor::Field("Duration (s, 0=infinite)", a_timing.duration, 0.01f, 0.0f)) _isChanged = true;

			if (a_timing.duration <= 0.0f)
			{
				Engine::Editor::HelpText("止めるまで出し続ける");
			}

			return _isChanged;
		}

		//-----------------------------------------------------------------------------------------
		// パーティクル1件
		//-----------------------------------------------------------------------------------------
		bool ParticlePartEdit(const ECS::EngineServices& a_services, EditorContext* a_pEditContext, Resource::EffectParticlePart& a_part)
		{
			bool _isChanged = false;

			// ---- 何を出すか ----
			if (AssetField(a_services, "Particle", "ParticlesAsset", a_part.particleGUID))
			{
				_isChanged = true;
			}
			DrawAssignedName(a_pEditContext, a_part.particleGUID);

			if (a_part.IsValid())
			{
				Engine::Editor::SameLine();
				if (DeleteButton("Clear"))
				{
					a_part.particleGUID = Engine::DefaultGUID;
					_isChanged = true;
				}
			}

			Engine::Editor::Header("Emit Source");

			// ---- どこから出すか ----
			Field("Space", a_part.space);
			if (a_part.space == Resource::EEffectSpace::LocalOffset)
			{
				Engine::Editor::Field("PosOffset", a_part.posOffset, 0.05f);
				Engine::Editor::Field("EmitDir (local)", a_part.emitDir, 0.05f);
			}
			else if (a_part.space == Resource::EEffectSpace::ReverseVelocity)
			{
				Engine::Editor::Field("PosOffset", a_part.posOffset, 0.05f);
				Engine::Editor::Tooltip("Dir : -Velocity (fallback : -Forward)");
			}
			else
			{
				Engine::Editor::HelpText("Pos/Dir : 付いている相手の行列そのまま");
			}

			Engine::Editor::Header("Emit Shape");

			// ---- どっちへ出すか ----
			Field("Shape", a_part.emitShape);
			switch (a_part.emitShape)
			{
			case Particle::EParticleEmitShape::Sphere:
				Engine::Editor::HelpText("中心から全方向へ均等に飛び散る(爆発向き)");
				break;
			case Particle::EParticleEmitShape::Hemisphere:
				Engine::Editor::HelpText("EmitDir 側の半球だけへ飛び散る(地面での爆発向き)");
				break;
			case Particle::EParticleEmitShape::Cone:
			default:
				Engine::Editor::HelpText("EmitDir を軸にした円錐。広がりは DirectionAngle");
				break;
			}

			Engine::Editor::Header("Emission");

			// ---- どれだけ出すか ----
			Engine::Editor::Field("EmitCount", a_part.emitCount, 1, 0);
			Engine::Editor::Field("EmitRate (/s, 0=Burst)", a_part.emitRate, 0.5f, 0.0f);
			if (a_part.emitRate <= 0.0f)
			{
				Engine::Editor::HelpText("出し始めに一度だけ EmitCount 個");
			}

			Engine::Editor::Header("Timing");
			TimingEdit(a_part.timing);

			Engine::Editor::Header("Shape");

			// ---- 散らばり方 ----
			// 1粒の速度・寿命はパーティクルアセット側なので、ここには出さない
			Engine::Editor::Field("BaseScale", a_part.baseScale, 0.05f, 0.0f);
			Engine::Editor::Field("MinScale", a_part.minScale, 0.01f, 0.0f);
			Engine::Editor::Field("MaxScale", a_part.maxScale, 0.01f, 0.0f);
			Engine::Editor::Field("PositionRadius", a_part.positionRadius, 0.05f, 0.0f);

			// 円錐のときしか効かない値なので、それ以外では触らせない
			ImGui::BeginDisabled(a_part.emitShape != Particle::EParticleEmitShape::Cone);
			Engine::Editor::Field("DirectionAngle (deg)", a_part.directionAngle, 0.5f, 0.0f, 180.0f);
			ImGui::EndDisabled();
			if (a_part.emitShape != Particle::EParticleEmitShape::Cone)
			{
				Engine::Editor::HelpText("(DirectionAngle は Cone のときだけ効きます)");
			}

			Engine::Editor::HelpText("初速・寿命・絵・減衰・色はパーティクルアセット側");

			return _isChanged;
		}

		//-----------------------------------------------------------------------------------------
		// メッシュ1件
		//-----------------------------------------------------------------------------------------
		bool MeshPartEdit(const ECS::EngineServices& a_services, EditorContext* a_pEditContext, Resource::EffectMeshPart& a_part)
		{
			bool _isChanged = false;

			// ---- 何を出すか ----
			if (AssetField(a_services, "Model", "Model", a_part.modelGUID))
			{
				_isChanged = true;
			}
			DrawAssignedName(a_pEditContext, a_part.modelGUID);

			if (a_part.IsValid())
			{
				Engine::Editor::SameLine();
				if (DeleteButton("Clear"))
				{
					a_part.modelGUID = Engine::DefaultGUID;
					_isChanged = true;
				}
			}

			Engine::Editor::Header("Transform");
			Engine::Editor::HelpText("付いている相手の行列基準のローカル配置");
			Engine::Editor::Field("PosOffset", a_part.posOffset, 0.05f);
			Engine::Editor::Field("Rotation (deg)", a_part.rotation, 1.0f);
			Engine::Editor::Field("Scale", a_part.scale, 0.05f);

			Engine::Editor::Header("Timing");
			TimingEdit(a_part.timing);

			Engine::Editor::Header("Look");
			Engine::Editor::ColorField("ColorScale", a_part.colorScale);
			Engine::Editor::ColorField("EmissiveColor", a_part.emissiveColor);
			Engine::Editor::Field("EmissiveIntensity", a_part.emissiveIntensity, 0.05f, 0.0f);
			Engine::Editor::Tooltip("ブルームのしきい値(既定1.0)を超えると光る");

			Engine::Editor::Header("End (Duration の終わりでの値)");
			if (a_part.timing.duration <= 0.0f)
			{
				// duration が無いと補間する区間が無い
				Engine::Editor::HelpText("Duration が 0 の間は変化しない");
			}
			Engine::Editor::Field("EndScale (倍率)", a_part.endScale, 0.05f, 0.0f);
			Engine::Editor::Field("EndAlpha", a_part.endAlpha, 0.01f, 0.0f, 1.0f);
			Engine::Editor::Field("EndEmissiveIntensity", a_part.endEmissiveIntensity, 0.05f, 0.0f);

			return _isChanged;
		}

		//-----------------------------------------------------------------------------------------
		// サウンド1件
		//-----------------------------------------------------------------------------------------
		bool SoundPartEdit(const ECS::EngineServices& a_services, EditorContext* a_pEditContext, Resource::EffectSoundPart& a_part)
		{
			bool _isChanged = false;

			// ---- 何を鳴らすか ----
			if (AssetField(a_services, "Sound", "Sound", a_part.soundGUID))
			{
				_isChanged = true;
			}
			DrawAssignedName(a_pEditContext, a_part.soundGUID);

			if (a_part.IsValid())
			{
				Engine::Editor::SameLine();
				if (DeleteButton("Clear"))
				{
					a_part.soundGUID = Engine::DefaultGUID;
					_isChanged = true;
				}
			}

			Engine::Editor::Header("Timing");
			Engine::Editor::HelpText("StartDelay : 再生から何秒後に鳴らすか");
			Engine::Editor::Field("StartDelay (s)", a_part.timing.startDelay, 0.01f, 0.0f);

			// Duration はループ音を止めるための長さ。単発音では使わない
			ImGui::BeginDisabled(!a_part.isLoop);
			Engine::Editor::Field("Duration (s, 0=infinite)", a_part.timing.duration, 0.01f, 0.0f);
			ImGui::EndDisabled();
			if (!a_part.isLoop)
			{
				Engine::Editor::HelpText("(Duration は Loop のときだけ効きます)");
			}
			else if (a_part.timing.duration <= 0.0f)
			{
				Engine::Editor::HelpText("エフェクトを止めるまで鳴らし続ける");
			}

			Engine::Editor::Header("Play");
			Engine::Editor::Field("Volume", a_part.vol, 0.01f, 0.0f, 1.0f);
			Engine::Editor::Field("Loop", a_part.isLoop);
			Engine::Editor::Tooltip(a_part.isLoop
				? "鳴りっぱなし。エフェクトを止めると一緒に止まる"
				: "一度だけ鳴らす。エフェクトを止めても鳴りきる");

			Engine::Editor::Field("3D Sound", a_part.is3DSound);
			Engine::Editor::Tooltip(a_part.is3DSound
				? "エフェクトの居場所で鳴る(定位・距離減衰あり)"
				: "常に同じ音量で鳴る(UI・全体演出向き)");

			ImGui::BeginDisabled(!a_part.is3DSound);
			Engine::Editor::Field("DistanceScaler", a_part.distanceScaler, 0.05f, 0.0f);
			ImGui::EndDisabled();
			if (a_part.is3DSound)
			{
				Engine::Editor::HelpText("大きいほど遠くまで届く(1 = 通常)");
			}

			Engine::Editor::Header("Finish");
			Engine::Editor::Field("WaitFinish", a_part.isWaitFinish);
			Engine::Editor::Tooltip(a_part.isWaitFinish
				? "この音が鳴り終わるまでエフェクトを終わらせない"
				: "音の長さを見ない(絵が終わればエフェクトも終わる)");
			Engine::Editor::HelpText("DestroyOnFinish のエフェクトで音が途切れるのを防ぐ設定");

			return _isChanged;
		}
	}

	//-----------------------------------------------------------------------------------------
	// エフェクトアセットの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void EffectAssetEdit(
		const ECS::EngineServices& a_services,
		const Engine::GUID& a_guid,
		Resource::EffectAsset* a_pEffect,
		bool a_isShowOpenEditorButton,
		EditorContext* a_pEditContext)
	{
		if (!a_pEffect) { return; }

		const auto& _guid = a_guid;

		Engine::Editor::Value("Effect", "%s", a_pEffect->GetName().c_str());
		Engine::Editor::Line();

		// 保存ボタン
		if (ImGui::Button("Save Asset") && a_services.pAssetDatabase)
		{
			auto _filePath = a_services.pAssetDatabase->GetFilePathFromGUID(_guid);
			a_pEffect->Save(_filePath);
			ENGINE_LOG("Save EffectAsset : %s", _filePath.c_str());
		}

		// 単体確認用の画面を開く。
		// 開いている間はゲームのシーンが止まり、このエフェクトだけがゲームと同じ
		// レンダーグラフで描かれる。ここでの編集はそのまま向こうの見た目に反映される
		if (a_isShowOpenEditorButton)
		{
			Engine::Editor::SameLine();
			if (ImGui::Button("Open Effect Editor"))
			{
				if (auto* _pEffectEditor = MainEditor::Instance().RefEffectEditor())
				{
					_pEffectEditor->Open(_guid);
				}
			}
		}

		// 参照アセットを引き直す必要があるか
		bool _isChanged = false;

		//------------------------------------------------------------------
		// パーティクルパーツ
		//------------------------------------------------------------------
		auto& _particleParts = a_pEffect->RefParticleParts();

		Engine::Editor::Header("Particle Parts");
		Engine::Editor::Text("%d / %d", static_cast<int>(_particleParts.size()), static_cast<int>(Resource::EFFECT_PARTICLE_MAX));

		// 上限まで来たら足せない(実体側の進行状態が固定長のため)
		ImGui::BeginDisabled(_particleParts.size() >= Resource::EFFECT_PARTICLE_MAX);
		if (CreateButton("Add Particle Part"))
		{
			a_pEffect->AddParticlePart();
		}
		ImGui::EndDisabled();

		// 反復中に消すと崩れるので削除は予約する
		int _removeParticleIndex = -1;

		for (size_t _i = 0; _i < _particleParts.size(); ++_i)
		{
			ImGui::PushID(static_cast<int>(_i));

			const std::string _label = "Particle " + std::to_string(_i);
			if (ImGui::TreeNodeEx(_label.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				if (DeleteButton("Remove Part"))
				{
					_removeParticleIndex = static_cast<int>(_i);
				}

				if (ParticlePartEdit(a_services, a_pEditContext, _particleParts[_i])) _isChanged = true;

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (_removeParticleIndex >= 0)
		{
			a_pEffect->RemoveParticlePart(static_cast<size_t>(_removeParticleIndex));
		}

		//------------------------------------------------------------------
		// メッシュパーツ
		//------------------------------------------------------------------
		auto& _meshParts = a_pEffect->RefMeshParts();

		Engine::Editor::Header("Mesh Parts");
		Engine::Editor::Text("%d / %d", static_cast<int>(_meshParts.size()), static_cast<int>(Resource::EFFECT_MESH_MAX));

		ImGui::BeginDisabled(_meshParts.size() >= Resource::EFFECT_MESH_MAX);
		if (CreateButton("Add Mesh Part"))
		{
			a_pEffect->AddMeshPart();
		}
		ImGui::EndDisabled();

		int _removeMeshIndex = -1;

		for (size_t _i = 0; _i < _meshParts.size(); ++_i)
		{
			// パーティクル側と番号が被るので、IDの土台をずらしておく
			ImGui::PushID(static_cast<int>(_i + Resource::EFFECT_PARTICLE_MAX));

			const std::string _label = "Mesh " + std::to_string(_i);
			if (ImGui::TreeNodeEx(_label.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				if (DeleteButton("Remove Part"))
				{
					_removeMeshIndex = static_cast<int>(_i);
				}

				if (MeshPartEdit(a_services, a_pEditContext, _meshParts[_i])) _isChanged = true;

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (_removeMeshIndex >= 0)
		{
			a_pEffect->RemoveMeshPart(static_cast<size_t>(_removeMeshIndex));
		}

		//------------------------------------------------------------------
		// サウンドパーツ
		//
		// 絵と音がいつも一緒に出るものを1枚にまとめるための欄。
		// 出す側は再生を伝えるだけでよく、音を別に鳴らしに行かなくてよくなる
		//------------------------------------------------------------------
		auto& _soundParts = a_pEffect->RefSoundParts();

		Engine::Editor::Header("Sound Parts");
		Engine::Editor::Text("%d / %d", static_cast<int>(_soundParts.size()), static_cast<int>(Resource::EFFECT_SOUND_MAX));

		// 上限まで来たら足せない(実体側の声の席が固定長のため)
		ImGui::BeginDisabled(_soundParts.size() >= Resource::EFFECT_SOUND_MAX);
		if (CreateButton("Add Sound Part"))
		{
			a_pEffect->AddSoundPart();
		}
		ImGui::EndDisabled();

		int _removeSoundIndex = -1;

		for (size_t _i = 0; _i < _soundParts.size(); ++_i)
		{
			// 上2つと番号が被らないように、IDの土台をずらしておく
			ImGui::PushID(static_cast<int>(_i + Resource::EFFECT_PARTICLE_MAX + Resource::EFFECT_MESH_MAX));

			const std::string _label = "Sound " + std::to_string(_i);
			if (ImGui::TreeNodeEx(_label.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				if (DeleteButton("Remove Part"))
				{
					_removeSoundIndex = static_cast<int>(_i);
				}

				if (SoundPartEdit(a_services, a_pEditContext, _soundParts[_i])) _isChanged = true;

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (_removeSoundIndex >= 0)
		{
			a_pEffect->RemoveSoundPart(static_cast<size_t>(_removeSoundIndex));
		}

		// 参照を差し替えたらハンドルを引き直す。
		// 保存前でもエディター上ですぐ確認できるようにしておく
		if (_isChanged)
		{
			a_pEffect->ResolveReferences(*a_services.pResourceManager);
		}
	}
}
