#pragma once

#include "Engine/ECS/World/World.h"
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"
#include "Application/Components/Persistence/GUIDComponent.h"
#include "Application/Components/Persistence/NameComponent.h"

// アタッチメント1スロット分。
// GUID を保存し、PostDeserialize で AttachmentSlotLinkSystem が id を解決する。
struct AttachmentSlot
{
	Engine::GUID		 guid = {};												// シリアライズ用
	Engine::ECS::Entity	 id   = Engine::ECS::Limits::INVALID_ENTITY;		// ランタイム用(GUIDから解決)
};

struct AttachmentSlotsComponent
{
	// 肩のブースター
	AttachmentSlot rightShoulderBoost;
	AttachmentSlot leftShoulderBoost;

	// 足のブースター
	AttachmentSlot rightLegBoost;
	AttachmentSlot leftLegBoost;

	// 武器
	AttachmentSlot mainGun;
	AttachmentSlot missile;
};

template<>
struct Engine::ECS::ComponentTraits<AttachmentSlotsComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AttachmentSlotsComponent& _comp = Engine::Editor::GetValue<AttachmentSlotsComponent>(a_pData);

		// 各スロットは GUID のみ保存する(id はランタイムで解決)
		a_ar.Field("rightShoulderBoostGUID", _comp.rightShoulderBoost.guid);
		a_ar.Field("leftShoulderBoostGUID",  _comp.leftShoulderBoost.guid);
		a_ar.Field("rightLegBoostGUID",      _comp.rightLegBoost.guid);
		a_ar.Field("leftLegBoostGUID",       _comp.leftLegBoost.guid);
		a_ar.Field("mainGunGUID",            _comp.mainGun.guid);
		a_ar.Field("missileGUID",            _comp.missile.guid);
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		AttachmentSlotsComponent& _comp = Engine::Editor::GetValue<AttachmentSlotsComponent>(a_context.pData);

		auto* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "World is null");
			return;
		}

		// エンティティのラベル(名前があれば名前、無ければGUID)を返す
		auto _entityLabel = [&](Engine::ECS::Entity a_e, const Engine::GUID& a_guid) -> std::string
		{
			if (a_e != Engine::ECS::Limits::INVALID_ENTITY &&
				_pWorld->HasComponent<NameComponent>(a_e))
			{
				if (auto* _pName = _pWorld->RefData<NameComponent>(a_e))
				{
					return _pName->name;
				}
			}
			return a_guid.String();
		};

		// 候補に出すエンティティ(GUIDを持つもの)を先に集める。
		// 名前は重複し得る(武器を複製すれば同名が並ぶ)ので、
		// 同名のものにだけGUIDの頭を添えて選び分けられるようにする
		struct SlotCandidate
		{
			Engine::ECS::Entity entity = Engine::ECS::Limits::INVALID_ENTITY;
			Engine::GUID        guid   = {};
			std::string         name   = {};
		};

		std::vector<SlotCandidate> _candidateVec;
		for (const auto& _loc : _pWorld->GetEntityList())
		{
			Engine::ECS::Entity _e = _pWorld->GetEntity(_loc);
			if (_e == Engine::ECS::Limits::INVALID_ENTITY) continue;
			if (!_pWorld->HasComponent<GUIDComponent>(_e)) continue;

			auto* _pGuid = _pWorld->RefData<GUIDComponent>(_e);
			if (!_pGuid) continue;

			_candidateVec.push_back({ _e, _pGuid->guid, _entityLabel(_e, _pGuid->guid) });
		}

		const auto _duplicatedSet = Engine::Editor::EditorHelper::CollectDuplicatedNames(
			_candidateVec, [](const SlotCandidate& a_candidate) { return a_candidate.name; });

		// 同名を見分けるための手掛かり。GUIDは長いので頭だけ出す
		auto _guidHint = [](const Engine::GUID& a_guid) -> std::string
		{
			return a_guid.String().substr(0, 8);
		};

		// 1スロット分の選択コンボを描画
		auto _drawSlot = [&](const char* a_label, AttachmentSlot& a_slot)
		{
			// 現在の選択表示。同名の候補があるならGUIDの頭まで出す
			std::string _current = "None";
			if (a_slot.guid != Engine::DefaultGUID)
			{
				_current = Engine::Editor::EditorHelper::MakeUniqueLabel(
					_duplicatedSet, _entityLabel(a_slot.id, a_slot.guid), _guidHint(a_slot.guid));
			}

			if (ImGui::BeginCombo(a_label, _current.c_str()))
			{
				// クリア用
				if (ImGui::Selectable("None", a_slot.guid == Engine::DefaultGUID))
				{
					a_slot.guid = {};
					a_slot.id = Engine::ECS::Limits::INVALID_ENTITY;
				}

				// GUIDComponent を持つ全エンティティが候補
				for (const SlotCandidate& _candidate : _candidateVec)
				{
					// 名前が同じでもImGuiのIDがぶつからないようにエンティティIDでPushID
					ImGui::PushID(static_cast<int>(_candidate.entity));

					bool _selected = (a_slot.guid == _candidate.guid);

					const std::string _label = Engine::Editor::EditorHelper::MakeUniqueLabel(
						_duplicatedSet, _candidate.name, _guidHint(_candidate.guid));

					if (ImGui::Selectable(_label.c_str(), _selected))
					{
						a_slot.guid = _candidate.guid;
						a_slot.id = _candidate.entity;
					}
					if (_selected) ImGui::SetItemDefaultFocus();
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}

			// 参考: 解決済みのランタイムID
			ImGui::SameLine();
			ImGui::TextDisabled("id:%llu", static_cast<unsigned long long>(a_slot.id));
		};

		ImGui::Text("Boosters");
		_drawSlot("R Shoulder", _comp.rightShoulderBoost);
		_drawSlot("L Shoulder", _comp.leftShoulderBoost);
		_drawSlot("R Leg",      _comp.rightLegBoost);
		_drawSlot("L Leg",      _comp.leftLegBoost);

		ImGui::Separator();

		ImGui::Text("Weapons");
		_drawSlot("Main Gun", _comp.mainGun);
		_drawSlot("Missile",  _comp.missile);
	}
};
