#include "SceneConfig.h"

#include "Engine/ECS/System/SystemContext.h"	// EngineServices
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/EditorField/EditorField.h"

namespace Engine::Scene
{
	namespace
	{
		// 設定ファイルの拡張子 : .oj / .ob はアーカイブが付ける
		constexpr const char* CONFIG_EXT = "scncfg";
	}

	//======================================================================================
	// 保存・読み込み
	//
	// 区切りで囲んであるので、後から足すものは区切りの末尾へ置けば古いファイルも読める
	//======================================================================================
	void SceneConfig::Archive(Persistence::Archive& a_ar)
	{
		Persistence::ArchiveSection _section(a_ar, "SceneConfig");

		a_ar.Field("IsRecordPreLoadAssets", m_isRecordPreLoadAssets);
		a_ar.GUIDVectorField("PreLoadAssetGUIDs", m_preLoadAssetGUIDVec);
	}

	bool SceneConfig::LoadFile(const std::string& a_fileDir, const std::string& a_fileName)
	{
		// 設定ファイルの無いシーン(作ってから一度も保存していないもの)は既定値のまま。
		// アーカイブはファイルが無いとエラーを出すので、先に確かめておく
		const std::string _basePath = a_fileDir + "/" + a_fileName;
		std::error_code _ec = {};
		if (!std::filesystem::exists(_basePath + ".oj" + CONFIG_EXT, _ec) &&
			!std::filesystem::exists(_basePath + ".ob" + CONFIG_EXT, _ec))
		{
			return false;
		}

		Persistence::Archive _ar(Persistence::Archive::EMode::Load, a_fileDir, a_fileName, CONFIG_EXT);
		Archive(_ar);
		return true;
	}

	void SceneConfig::SaveFile(const std::string& a_fileDir, const std::string& a_fileName)
	{
		Persistence::Archive _ar(Persistence::Archive::EMode::Save, a_fileDir, a_fileName, CONFIG_EXT);
		Archive(_ar);
	}

	//======================================================================================
	// エディター用の編集欄
	//======================================================================================
	void SceneConfig::DrawEdit(const ECS::EngineServices& a_services, bool a_isRecording)
	{
		Engine::EditorField::Field("Record PreLoad Assets", m_isRecordPreLoadAssets);
		Engine::EditorField::Tooltip(
			"立てて保存すると、次にこのシーンを開いてから閉じるまでに読み込まれたアセットを数え、\n"
			"閉じたところで先読み一覧を置き換えます(済んだらフラグは下ります)");

		if (a_isRecording)
		{
			Engine::EditorField::WarningText("計測中 : シーンを閉じたときに一覧を置き換えます");
		}

		Engine::EditorField::Header("PreLoad Assets");
		Engine::EditorField::Value("Count", "%zu", m_preLoadAssetGUIDVec.size());

		if (m_preLoadAssetGUIDVec.empty())
		{
			Engine::EditorField::HelpText("(なし)");
			return;
		}

		if (Engine::EditorField::DeleteSmallButton("Clear"))
		{
			m_preLoadAssetGUIDVec.clear();
			return;
		}

		//------------------------------------------------------------------
		// 一覧 : 種別と名前で出す。消えたアセットはGUIDのまま警告で出す
		// (先読みのときは読み飛ばされるだけなので、ここで気付けるようにしておく)
		//------------------------------------------------------------------
		size_t _removeIndex = m_preLoadAssetGUIDVec.size();
		for (size_t _i = 0; _i < m_preLoadAssetGUIDVec.size(); ++_i)
		{
			Engine::EditorField::IDScope _id(static_cast<int>(_i));

			if (Engine::EditorField::DeleteSmallButton("x")) _removeIndex = _i;
			Engine::EditorField::SameLine();

			const Core::GUID& _guid = m_preLoadAssetGUIDVec[_i];
			const Resource::AssetProperty* _pProp = a_services.pAssetDatabase
				? a_services.pAssetDatabase->FindAssetProperty(_guid)
				: nullptr;

			if (_pProp)
			{
				Engine::EditorField::Text("[%s] %s", _pProp->type.c_str(), _pProp->fileName.c_str());
			}
			else
			{
				Engine::EditorField::WarningText("見つかりません : %s", _guid.String().c_str());
			}
		}

		if (_removeIndex < m_preLoadAssetGUIDVec.size())
		{
			m_preLoadAssetGUIDVec.erase(m_preLoadAssetGUIDVec.begin() + static_cast<std::ptrdiff_t>(_removeIndex));
		}
	}
}
