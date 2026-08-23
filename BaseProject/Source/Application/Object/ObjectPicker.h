#pragma once

#include "../../Engine/GameObject/GameObjectManager/GameObjectManager.h"
#include "../../Engine/Editor/Helper/EditorHelper.h"

//==========================================================================================
// シーンに置いたオブジェクトを GUID で指すための小物
//
// 進行役(HomeSequence / MissionSelect)は、相手のポインタではなく GUID を保存して
// 実行時に引き直す。ポインタを持つと読み込み順に縛られるうえ、保存もできないため。
//
// 引く側の書き方が毎回同じなので、ここへまとめてある。
//==========================================================================================
namespace App::Object::Picker
{
	/// <summary>
	/// GUID から T を引く
	/// </summary>
	/// <returns>見つからない・型が違うなら nullptr(設定ミスでも落ちないように)</returns>
	template<typename T>
	inline T* Find(Engine::GameObject::GameObjectManager* a_pObjectManager, const Engine::GUID& a_guid)
	{
		if (a_pObjectManager == nullptr) return nullptr;
		if (!a_guid.IsValid()) return nullptr;

		return dynamic_cast<T*>(a_pObjectManager->FindByGUID(a_guid));
	}

	/// <summary>
	/// 指しているものが全部そろっているか
	/// </summary>
	/// <remarks>
	/// 設定されているのに引けないものが1つでもあれば、まだ読み込みの途中とみなす。
	/// 差し込み(SetOnClick)は全部そろってから一度に行うので、差し込み漏れが起きない
	/// </remarks>
	template<typename T>
	inline bool IsReady(Engine::GameObject::GameObjectManager* a_pObjectManager, const Engine::GUID& a_guid)
	{
		if (!a_guid.IsValid()) return true;	// 設定されていないものは待たない

		return Find<T>(a_pObjectManager, a_guid) != nullptr;
	}

	//======================================================================================
	// エディター用
	//======================================================================================

	// 一覧に出す見出し : 型名 + GUIDの頭だけ(生のGUIDは長すぎて見分けられない)
	inline std::string MakeLabel(const Engine::GameObject::BaseObject* a_pObject)
	{
		if (a_pObject == nullptr) return "None";

		const std::string _guid = a_pObject->GetGUID().String();
		return std::string(a_pObject->GetEditorName()) + " [" + _guid.substr(0, 8) + "]";
	}

	/// <summary>
	/// シーンに置いた T のなかから1つ選ぶコンボ
	/// </summary>
	/// <returns>選び直したら true</returns>
	template<typename T>
	inline bool DrawCombo(
		const char* a_label,
		Engine::GameObject::GameObjectManager* a_pObjectManager,
		Engine::GUID& a_inoutGUID)
	{
		bool _isChanged = false;

		const auto* _pCurrent = Find<T>(a_pObjectManager, a_inoutGUID);

		// 指しているものが見つからないときは、設定ミスと読み込み前を見分けられるようにしておく
		std::string _currentLabel = "None";
		if (a_inoutGUID.IsValid())
		{
			_currentLabel = _pCurrent ? MakeLabel(_pCurrent) : ("(missing) " + a_inoutGUID.String().substr(0, 8));
		}

		if (!ImGui::BeginCombo(a_label, _currentLabel.c_str())) return false;

		if (ImGui::Selectable("None", !a_inoutGUID.IsValid()))
		{
			a_inoutGUID = {};
			_isChanged = true;
		}

		if (a_pObjectManager)
		{
			const auto& _objectVec = a_pObjectManager->GetObjects();
			for (size_t _i = 0; _i < _objectVec.size(); ++_i)
			{
				auto* _pObject = dynamic_cast<T*>(_objectVec[_i].get());
				if (!_pObject) continue;

				// 同名でもIDがぶつからないようにする
				ImGui::PushID(static_cast<int>(_i));

				const bool _isSelected = (a_inoutGUID == _pObject->GetGUID());
				if (ImGui::Selectable(MakeLabel(_pObject).c_str(), _isSelected))
				{
					a_inoutGUID = _pObject->GetGUID();
					_isChanged = true;
				}
				if (_isSelected) ImGui::SetItemDefaultFocus();

				ImGui::PopID();
			}
		}

		ImGui::EndCombo();

		return _isChanged;
	}

	/// <summary>
	/// GUIDの配列を編集する(差し替え・削除・追加)
	/// </summary>
	/// <returns>中身が変わったら true</returns>
	template<typename T>
	inline bool DrawList(
		const char* a_label,
		Engine::GameObject::GameObjectManager* a_pObjectManager,
		std::vector<Engine::GUID>& a_inoutGUIDVec)
	{
		bool _isChanged = false;

		if (!ImGui::TreeNode(a_label)) return false;

		// 回している間に配列を触ると足元が崩れるので、削除は覚えておいて後で行う
		int _removeIndex = -1;

		for (size_t _i = 0; _i < a_inoutGUIDVec.size(); ++_i)
		{
			ImGui::PushID(static_cast<int>(_i));

			// ボタンを先に置く : コンボが残り幅を全部使うため
			if (Engine::Editor::EditorHelper::DeleteSmallButton("X"))
			{
				_removeIndex = static_cast<int>(_i);
			}

			ImGui::SameLine();
			if (DrawCombo<T>("##Object", a_pObjectManager, a_inoutGUIDVec[_i])) _isChanged = true;

			ImGui::PopID();
		}

		if (_removeIndex >= 0)
		{
			a_inoutGUIDVec.erase(a_inoutGUIDVec.begin() + _removeIndex);
			_isChanged = true;
		}

		if (Engine::Editor::EditorHelper::CreateButton("Add"))
		{
			a_inoutGUIDVec.push_back({});
			_isChanged = true;
		}

		ImGui::TreePop();

		return _isChanged;
	}
}
