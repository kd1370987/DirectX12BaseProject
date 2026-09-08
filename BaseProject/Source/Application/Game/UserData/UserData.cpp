#include "UserData.h"

namespace App::Game
{
	//==========================================================================================
	// 入力設定の読み書きに使う小物
	//------------------------------------------------------------------------------------------
	// アクションは enum のままだと数値で出てしまう。項目を足したり並べ替えたりした
	// だけで保存済みの設定が別のアクションのものになるため、magic_enum で名前へ直す。
	// 割り当ての種類(ボタン/軸)も同じ理由で文字列にしている。
	//==========================================================================================
	namespace
	{
		// 割り当ての種類
		constexpr const char* BINDING_TYPE_BUTTON = "Button";
		constexpr const char* BINDING_TYPE_AXIS = "Axis";

		//--------------------------------------------------------------------------------------
		// 割り当て1件(variantの中身)を読み書きする
		//--------------------------------------------------------------------------------------
		void BindingArchive(Engine::Persistence::Archive& a_ar, InputBindingData& a_binding)
		{
			// 種類 : 保存は今入っている方、読み込みは書いてあった方に合わせる
			std::string _type = std::holds_alternative<AxisInputData>(a_binding)
				? BINDING_TYPE_AXIS
				: BINDING_TYPE_BUTTON;

			a_ar.StringField("Type", _type);

			// 読み込みは、この後の取り出しが空振りしないよう先に中身を入れ替えておく
			if (a_ar.IsLoading())
			{
				if (_type == BINDING_TYPE_AXIS)	a_binding = AxisInputData{};
				else							a_binding = ButtonInputData{};
			}

			if (auto* _pAxis = std::get_if<AxisInputData>(&a_binding))
			{
				a_ar.Field("Up", _pAxis->up);
				a_ar.Field("Right", _pAxis->right);
				a_ar.Field("Down", _pAxis->down);
				a_ar.Field("Left", _pAxis->left);
			}
			else if (auto* _pButton = std::get_if<ButtonInputData>(&a_binding))
			{
				a_ar.Field("Key", _pButton->key);
			}
		}

		//--------------------------------------------------------------------------------------
		// アクションと割り当ての対応表を1つ読み書きする
		//--------------------------------------------------------------------------------------
		// JSONでは名前付きの配列になる。
		//   "Keyboard": [ { "Action":"Move", "Type":"Axis", "Up":87, ... }, ... ]
		//
		// unordered_map をそのまま回すと並びが毎回変わって差分が読めなくなるので、
		// 保存はアクションの並び(enumの値)で揃えてから書き出す。
		//--------------------------------------------------------------------------------------
		void ActionMapArchive(
			Engine::Persistence::Archive& a_ar,
			const std::string& a_name,
			std::unordered_map<EGameAction, InputBindingData>& a_map)
		{
			//----------------------------------------------------------------------------------
			// 保存
			//----------------------------------------------------------------------------------
			if (a_ar.IsSaving())
			{
				// 並びを固定する
				std::vector<EGameAction> _actionVec;
				_actionVec.reserve(a_map.size());
				for (const auto& [_action, _binding] : a_map) _actionVec.push_back(_action);
				std::sort(_actionVec.begin(), _actionVec.end());

				size_t _count = _actionVec.size();
				if (!a_ar.BeginArray(a_name, _count)) return;

				for (size_t _i = 0; _i < _count; ++_i)
				{
					if (!a_ar.BeginObject(_i)) continue;

					std::string _actionName{ magic_enum::enum_name(_actionVec[_i]) };
					a_ar.StringField("Action", _actionName);

					BindingArchive(a_ar, a_map[_actionVec[_i]]);

					a_ar.EndObject();
				}
				a_ar.EndArray();
				return;
			}

			//----------------------------------------------------------------------------------
			// 読み込み
			//----------------------------------------------------------------------------------
			size_t _count = 0;
			if (!a_ar.BeginArray(a_name, _count)) return;

			a_map.clear();

			for (size_t _i = 0; _i < _count; ++_i)
			{
				if (!a_ar.BeginObject(_i)) continue;

				std::string _actionName;
				a_ar.StringField("Action", _actionName);

				// 中身は BindingArchive が種類を見て入れ替える
				InputBindingData _binding = ButtonInputData{};
				BindingArchive(a_ar, _binding);

				a_ar.EndObject();

				// 名前が変わった・消えたアクションはここで落とす。
				// 拾ってしまうと関係のないアクションへ割り当てが移ってしまう
				// (バイナリは要素分を読み切ってから捨てないと後ろがずれる)
				const auto _action = magic_enum::enum_cast<EGameAction>(_actionName);
				if (!_action.has_value())
				{
					ENGINE_WARNING("知らない入力アクションを読み飛ばしました : %s / %s",
						a_name.c_str(), _actionName.c_str());
					continue;
				}

				a_map[_action.value()] = _binding;
			}
			a_ar.EndArray();
		}
	}

	//==========================================================================================
	// ユーザーデータすべて
	//==========================================================================================
	void UserData::Archive(Engine::Persistence::Archive& a_ar)
	{
		GameDataArchive(a_ar);
		SoundArchive(a_ar);
		InputArchive(a_ar);
	}

	void UserData::GameDataArchive(Engine::Persistence::Archive& a_ar)
	{}

	//==========================================================================================
	// サウンド設定
	//==========================================================================================
	void UserData::SoundArchive(Engine::Persistence::Archive& a_ar)
	{
		if (!a_ar.BeginGroup("Sound")) return;

		a_ar.Field("MainVol", m_soundSettings.mainVol);
		a_ar.Field("BgmVol", m_soundSettings.bgmVol);
		a_ar.Field("UiVol", m_soundSettings.uiVol);
		a_ar.Field("SeVol", m_soundSettings.seVol);

		a_ar.EndGroup();
	}

	//==========================================================================================
	// 入力設定
	//------------------------------------------------------------------------------------------
	// デバイスごとに1つの対応表を持つ
	void UserData::InputArchive(Engine::Persistence::Archive& a_ar)
	{
		if (!a_ar.BeginGroup("Input")) return;

		a_ar.Field("MouseSensitivity", m_inputSettings.mouseSensitivity);

		ActionMapArchive(a_ar, "Keyboard", m_inputSettings.keyboard);
		ActionMapArchive(a_ar, "Mouse", m_inputSettings.mouse);

		a_ar.EndGroup();
	}
}
