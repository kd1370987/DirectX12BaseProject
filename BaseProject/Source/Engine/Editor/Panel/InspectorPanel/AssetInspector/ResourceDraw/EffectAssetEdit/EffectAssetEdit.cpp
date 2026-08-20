#include "EffectAssetEdit.h"

#include "../../../../../Helper/EditorHelper.h"
#include "../../../../../EffectEditor/EffectEditor.h"
#include "../../../../../Editor.h"
#include "../../../../../../Resource/Manager/AssetDatabase/AssetDatabase.h"

namespace Engine::Editor::Inspector
{
	namespace
	{
		// 割り当てたアセットのファイル名を出す。GUIDだけでは何を指しているか分からないため
		void DrawAssignedName(const Engine::GUID& a_guid)
		{
			if (a_guid == Engine::DefaultGUID)
			{
				// 空欄はエラーではないことを明示しておく
				ImGui::TextDisabled("(empty : このパーツは出ない)");
				return;
			}

			const auto _fileName = Resource::AssetDatabase::Instance().GetFileNameFromGUID(a_guid);
			ImGui::TextDisabled("%s", _fileName.c_str());
		}

		//-----------------------------------------------------------------------------------------
		// 時間指定 : いつ出て、どれだけ続くか
		//-----------------------------------------------------------------------------------------
		bool TimingEdit(Resource::EffectTiming& a_timing)
		{
			bool _isChanged = false;

			if (ImGui::DragFloat("StartDelay (s)", &a_timing.startDelay, 0.01f, 0.0f)) _isChanged = true;
			if (ImGui::DragFloat("Duration (s, 0=infinite)", &a_timing.duration, 0.01f, 0.0f)) _isChanged = true;

			if (a_timing.duration <= 0.0f)
			{
				ImGui::TextDisabled("止めるまで出し続ける");
			}

			return _isChanged;
		}

		//-----------------------------------------------------------------------------------------
		// パーティクル1件
		//-----------------------------------------------------------------------------------------
		bool ParticlePartEdit(Resource::EffectParticlePart& a_part)
		{
			bool _isChanged = false;

			// ---- 何を出すか ----
			if (EditorHelper::DrawAssetSelectComboGUID("Particle", "ParticlesAsset", a_part.particleGUID))
			{
				_isChanged = true;
			}
			DrawAssignedName(a_part.particleGUID);

			if (a_part.IsValid())
			{
				ImGui::SameLine();
				if (EditorHelper::DeleteButton("Clear"))
				{
					a_part.particleGUID = Engine::DefaultGUID;
					_isChanged = true;
				}
			}

			ImGui::SeparatorText("Emit Source");

			// ---- どこから出すか ----
			EditorHelper::DrawEnumCombo("Space", a_part.space);
			if (a_part.space == Resource::EEffectSpace::LocalOffset)
			{
				ImGui::DragFloat3("PosOffset", &a_part.posOffset.x, 0.05f);
				ImGui::DragFloat3("EmitDir (local)", &a_part.emitDir.x, 0.05f);
			}
			else if (a_part.space == Resource::EEffectSpace::ReverseVelocity)
			{
				ImGui::DragFloat3("PosOffset", &a_part.posOffset.x, 0.05f);
				ImGui::TextDisabled("Dir : -Velocity (fallback : -Forward)");
			}
			else
			{
				ImGui::TextDisabled("Pos/Dir : 付いている相手の行列そのまま");
			}

			ImGui::SeparatorText("Emit Shape");

			// ---- どっちへ出すか ----
			EditorHelper::DrawEnumCombo("Shape", a_part.emitShape);
			switch (a_part.emitShape)
			{
			case Particle::EParticleEmitShape::Sphere:
				ImGui::TextDisabled("中心から全方向へ均等に飛び散る(爆発向き)");
				break;
			case Particle::EParticleEmitShape::Hemisphere:
				ImGui::TextDisabled("EmitDir 側の半球だけへ飛び散る(地面での爆発向き)");
				break;
			case Particle::EParticleEmitShape::Cone:
			default:
				ImGui::TextDisabled("EmitDir を軸にした円錐。広がりは DirectionAngle");
				break;
			}

			ImGui::SeparatorText("Emission");

			// ---- どれだけ出すか ----
			ImGui::DragInt("EmitCount", &a_part.emitCount, 1, 0);
			ImGui::DragFloat("EmitRate (/s, 0=Burst)", &a_part.emitRate, 0.5f, 0.0f);
			if (a_part.emitRate <= 0.0f)
			{
				ImGui::TextDisabled("出し始めに一度だけ EmitCount 個");
			}

			ImGui::SeparatorText("Timing");
			TimingEdit(a_part.timing);

			ImGui::SeparatorText("Shape");

			// ---- 散らばり方 ----
			// 1粒の速度・寿命はパーティクルアセット側なので、ここには出さない
			ImGui::DragFloat("BaseScale", &a_part.baseScale, 0.05f, 0.0f);
			ImGui::DragFloat("MinScale", &a_part.minScale, 0.01f, 0.0f);
			ImGui::DragFloat("MaxScale", &a_part.maxScale, 0.01f, 0.0f);
			ImGui::DragFloat("PositionRadius", &a_part.positionRadius, 0.05f, 0.0f);

			// 円錐のときしか効かない値なので、それ以外では触らせない
			ImGui::BeginDisabled(a_part.emitShape != Particle::EParticleEmitShape::Cone);
			ImGui::DragFloat("DirectionAngle (deg)", &a_part.directionAngle, 0.5f, 0.0f, 180.0f);
			ImGui::EndDisabled();
			if (a_part.emitShape != Particle::EParticleEmitShape::Cone)
			{
				ImGui::TextDisabled("(DirectionAngle は Cone のときだけ効きます)");
			}

			ImGui::TextDisabled("初速・寿命・絵・減衰・色はパーティクルアセット側");

			return _isChanged;
		}

		//-----------------------------------------------------------------------------------------
		// メッシュ1件
		//-----------------------------------------------------------------------------------------
		bool MeshPartEdit(Resource::EffectMeshPart& a_part)
		{
			bool _isChanged = false;

			// ---- 何を出すか ----
			if (EditorHelper::DrawAssetSelectComboGUID("Model", "Model", a_part.modelGUID))
			{
				_isChanged = true;
			}
			DrawAssignedName(a_part.modelGUID);

			if (a_part.IsValid())
			{
				ImGui::SameLine();
				if (EditorHelper::DeleteButton("Clear"))
				{
					a_part.modelGUID = Engine::DefaultGUID;
					_isChanged = true;
				}
			}

			ImGui::SeparatorText("Transform");
			ImGui::TextDisabled("付いている相手の行列基準のローカル配置");
			ImGui::DragFloat3("PosOffset", &a_part.posOffset.x, 0.05f);
			ImGui::DragFloat3("Rotation (deg)", &a_part.rotation.x, 1.0f);
			ImGui::DragFloat3("Scale", &a_part.scale.x, 0.05f);

			ImGui::SeparatorText("Timing");
			TimingEdit(a_part.timing);

			ImGui::SeparatorText("Look");
			ImGui::ColorEdit4("ColorScale", a_part.colorScale.Data());
			ImGui::ColorEdit3("EmissiveColor", &a_part.emissiveColor.x);
			ImGui::DragFloat("EmissiveIntensity", &a_part.emissiveIntensity, 0.05f, 0.0f);
			ImGui::TextDisabled("ブルームのしきい値(既定1.0)を超えると光る");

			ImGui::SeparatorText("End (Duration の終わりでの値)");
			if (a_part.timing.duration <= 0.0f)
			{
				// duration が無いと補間する区間が無い
				ImGui::TextDisabled("Duration が 0 の間は変化しない");
			}
			ImGui::DragFloat3("EndScale (倍率)", &a_part.endScale.x, 0.05f, 0.0f);
			ImGui::DragFloat("EndAlpha", &a_part.endAlpha, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("EndEmissiveIntensity", &a_part.endEmissiveIntensity, 0.05f, 0.0f);

			return _isChanged;
		}
	}

	//-----------------------------------------------------------------------------------------
	// エフェクトアセットの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void EffectAssetEdit(
		const Engine::GUID& a_guid,
		Resource::EffectAsset* a_pEffect,
		bool a_isShowOpenEditorButton)
	{
		if (!a_pEffect) { return; }

		const auto& _guid = a_guid;

		ImGui::Text("Effect : %s", a_pEffect->GetName().c_str());
		ImGui::Separator();

		// 保存ボタン
		if (ImGui::Button("Save Asset"))
		{
			auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			a_pEffect->Save(_filePath);
			ENGINE_LOG("Save EffectAsset : %s", _filePath.c_str());
		}

		// 単体確認用の画面を開く。
		// 開いている間はゲームのシーンが止まり、このエフェクトだけがゲームと同じ
		// レンダーグラフで描かれる。ここでの編集はそのまま向こうの見た目に反映される
		if (a_isShowOpenEditorButton)
		{
			ImGui::SameLine();
			if (ImGui::Button("Open Effect Editor"))
			{
				if (auto* _pEffectEditor = MainEditor::Instance().RefEffectEditor())
				{
					_pEffectEditor->Open(_guid);
				}
			}
		}

		ImGui::Spacing();

		// 参照アセットを引き直す必要があるか
		bool _isChanged = false;

		//------------------------------------------------------------------
		// パーティクルパーツ
		//------------------------------------------------------------------
		auto& _particleParts = a_pEffect->RefParticleParts();

		ImGui::SeparatorText("Particle Parts");
		ImGui::Text("%d / %d",
			static_cast<int>(_particleParts.size()),
			static_cast<int>(Resource::EFFECT_PARTICLE_MAX));

		// 上限まで来たら足せない(実体側の進行状態が固定長のため)
		ImGui::BeginDisabled(_particleParts.size() >= Resource::EFFECT_PARTICLE_MAX);
		if (EditorHelper::CreateButton("Add Particle Part"))
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
				if (EditorHelper::DeleteButton("Remove Part"))
				{
					_removeParticleIndex = static_cast<int>(_i);
				}

				if (ParticlePartEdit(_particleParts[_i])) _isChanged = true;

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

		ImGui::SeparatorText("Mesh Parts");
		ImGui::Text("%d / %d",
			static_cast<int>(_meshParts.size()),
			static_cast<int>(Resource::EFFECT_MESH_MAX));

		ImGui::BeginDisabled(_meshParts.size() >= Resource::EFFECT_MESH_MAX);
		if (EditorHelper::CreateButton("Add Mesh Part"))
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
				if (EditorHelper::DeleteButton("Remove Part"))
				{
					_removeMeshIndex = static_cast<int>(_i);
				}

				if (MeshPartEdit(_meshParts[_i])) _isChanged = true;

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (_removeMeshIndex >= 0)
		{
			a_pEffect->RemoveMeshPart(static_cast<size_t>(_removeMeshIndex));
		}

		// 参照を差し替えたらハンドルを引き直す。
		// 保存前でもエディター上ですぐ確認できるようにしておく
		if (_isChanged)
		{
			a_pEffect->ResolveReferences();
		}
	}
}
