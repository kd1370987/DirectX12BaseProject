#include "SceneConfigPanel.h"

#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/Scene/BaseScene/BaseScene.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

//==========================================================================================
// SceneConfigPanel
//
// 触るのは一番上のシーンの設定。
// 計測は開くときに始まるので、フラグを立てただけでは今開いているシーンは計測されない。
// 計測中かどうかは SceneManager が知っているので、そちらに聞いて出す。
//==========================================================================================
void Editor::SceneConfigPanel::OnDrawImGui(EditorContext& a_editContext)
{
	auto& _sceneManager = Engine::Scene::SceneManager::Instance();

	Engine::Scene::BaseScene* _pScene = _sceneManager.RefCurrentTopScene();
	if (!_pScene || !a_editContext.pServices)
	{
		Engine::EditorField::HelpText("No scene");
		return;
	}

	// シーンの名前 : GUID からファイルを引いて出す
	std::string _sceneName = _pScene->GetGUID().String();
	if (a_editContext.pServices->pAssetDatabase)
	{
		const std::string _path = a_editContext.pServices->pAssetDatabase->GetFilePathFromGUID(_pScene->GetGUID());
		if (!_path.empty()) _sceneName = Core::File::GetFileNameWithoutExtension(_path);
	}
	Engine::EditorField::Value("Scene", "%s", _sceneName.c_str());

	Engine::EditorField::HelpText("シーンを保存すると一緒に保存されます");
	Engine::EditorField::Line();

	const bool _isRecording = (_sceneManager.GetPreLoadRecordingScene() == _pScene);
	_pScene->RefConfig().DrawEdit(*a_editContext.pServices, _isRecording);
}
