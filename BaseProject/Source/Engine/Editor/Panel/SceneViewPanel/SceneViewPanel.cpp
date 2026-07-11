#include "SceneViewPanel.h"
#include "../../../MainEngine.h"
#include "../../../Graphics/GraphicEngine.h"
#include "../../../Graphics/RenderGraph/RenderGraph.h"
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../../Scene/BaseScene/BaseScene.h"
#include "../../../Scene/SceneManager/SceneManager.h"

#include "../../../ECS/World/World.h"
#include "../../../../Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../Application/Components/Transform/WorldMatrixComponent.h"
namespace Engine::Editor
{
	void Engine::Editor::SceneViewPanel::OnDrawImGui(EditorContext& a_editContext)
	{
		// シーンファイルメニューバー
		SceneFileMenu(a_editContext);

		// ワールド取得
		Engine::ECS::World* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit()) return;

		ImVec2 _windowPos = ImGui::GetCursorScreenPos(); // ウィンドウの左上（タブなどを除いた純粋な描画領域のスタート位置）
		ImVec2 _windowSize = ImGui::GetContentRegionAvail(); // 残りのサイズ

		// 現在の最終出力テクスチャを取得
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE)
		{
			ImGui::End();
			return;
		}
		auto* _pRG = _pGE->RefRenderGraph();
		if (!_pRG)
		{
			ImGui::End();
			return;
		}
		const auto* _pTex = _pRG->GetTmepTexture("AffterTAAColor");
		if (!_pTex)
		{
			ImGui::End();
			return;
		}

		// テクスチャの描画 : 実際に描画した範囲も取得
		auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
		ImVec2 _actualRenderSize = Helper::DrawSRVView(_gpuHandle, static_cast<UINT>(1980), static_cast<UINT>(1080));

		// ギズモ描画
		GuizmoDraw(_windowPos, _actualRenderSize, a_editContext.entity, _pWorld);
	}
	void SceneViewPanel::GuizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect, const ECS::Entity& a_currentSelectEntity, Engine::ECS::World* a_pWorld)
	{
		// 現在のウィンドウの描画リストにギズモを追加するよう指示
		ImGuizmo::SetDrawlist();

		// ギズモの操作・描画領域を、テクスチャの領域にぴったり合わせる
		ImGuizmo::SetRect(a_pos.x, a_pos.y, a_rect.x, a_rect.y);

		if (!a_pWorld) return;

		// トランスフォームを持っているかチェック
		if (a_currentSelectEntity == ECS::Limits::INVALID_ENTITY) return;
		if (!a_pWorld->HasComponent<LocalTransformComponent>(a_currentSelectEntity)) return;

		// 取得
		auto* _pTrsComp = a_pWorld->RefData<LocalTransformComponent>(a_currentSelectEntity);
		if (!_pTrsComp) return;

		auto* _pWorldComp = a_pWorld->RefData<WorldMatrixComponent>(a_currentSelectEntity);
		if (!_pWorldComp) return;

		// 現在のカメラ行列を取得
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;
		const auto& _camData = _pGE->GetCPUCameraData();

		// Pos/Quat/Scale から 4x4ワールド行列を合成
		DirectX::XMVECTOR _vScale = DirectX::XMLoadFloat3(&_pTrsComp->scale);
		DirectX::XMVECTOR _vQuat = DirectX::XMLoadFloat4(&_pTrsComp->quat);
		DirectX::XMVECTOR _vPos = DirectX::XMLoadFloat3(&_pTrsComp->pos);

		// アフィン変換行列を作成
		DirectX::XMMATRIX _mWorld = DirectX::XMMatrixAffineTransformation(_vScale, DirectX::XMVectorZero(), _vQuat, _vPos);

		DirectX::XMFLOAT4X4 _worldFloat4x4;
		DirectX::XMStoreFloat4x4(&_worldFloat4x4, _mWorld);
		float _snapValues[3] = { 1.0f, 1.0f, 1.0f };
		bool _isSnap = ImGui::IsKeyDown(ImGuiKey_LeftCtrl); // Ctrlキーを押している時だけスナップ
		// マニピュレーターの操作
		ImGuizmo::Manipulate(
			&_camData.viewMat._11,
			&_camData.projMat._11,
			ImGuizmo::OPERATION::TRANSLATE,		// 移動モード
			ImGuizmo::MODE::WORLD,				// ワールド座標系
			&_worldFloat4x4._11,				// 操作したい行列（結果もここに入る）
			nullptr,
			_isSnap ? &_snapValues[0] : nullptr		// スナップ値
		);

		// ギズモをドラッグ中ならコンポーネントを更新
		if (ImGuizmo::IsUsing()) {
			DirectX::XMMATRIX updatedWorld = DirectX::XMLoadFloat4x4(&_worldFloat4x4);

			DirectX::XMVECTOR outScale, outRotQuat, outTrans;

			// 行列を分解
			DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, updatedWorld);

			DirectX::XMStoreFloat3(&_pTrsComp->pos, outTrans);
			DirectX::XMStoreFloat4(&_pTrsComp->quat, outRotQuat);
			DirectX::XMStoreFloat3(&_pTrsComp->scale, outScale);

			_pTrsComp->isDirty = true;

			DirectX::XMStoreFloat4x4(&_pWorldComp->worldMat, updatedWorld);
		}
	}
	void SceneViewPanel::SceneFileMenu(EditorContext& a_editContext)
	{
		if (m_currentSceneGUID == Engine::DefaultGUID)
		{
			auto* _pScene = Engine::Scene::SceneManager::Instance().GetCurrentTopScene();
			if (!_pScene) return;

			m_currentSceneGUID = _pScene->GetGUID();
			m_canOverwrite = true; // 上書き可能にする
			ENGINE_LOG("シーンのGUIDがセットされました : %s", m_currentSceneGUID.String().c_str());
		}

		// --- ショートカット判定 ---
		bool _isCtrl = ImGui::GetIO().KeyCtrl;
		m_isSaveShortcut = _isCtrl && ImGui::IsKeyPressed(ImGuiKey_S);
		m_doOverwrite = false;

		// Ctr + S が押されたら
		if (m_isSaveShortcut)
		{
			if (m_canOverwrite) m_doOverwrite = true;	// 現在のシーンにデータファイルがあるのなら、上書き
			else OpenSavePopup();						// なければ新規保存ポップアップ
		}

		// シーンの走査
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Create new scene"))
				{
					// TODO: 新規シーン生成処理、m_currentSceneGUIDのクリアなど
					ENGINE_LOG("新規シーンを作成しました。");
				}

				ImGui::Separator();

				// ロード
				if (ImGui::MenuItem("Load scene..."))
				{
					m_openLoadPopup = true;
				}

				ImGui::Separator();

				// セーブ
				// 上書き保存
				if (ImGui::MenuItem("Save scene", "Ctrl+S", false, m_canOverwrite))
				{
					m_doOverwrite = true;
				}
				// 名前を付けて保存
				if (ImGui::MenuItem("Save scene with Name..."))
				{
					OpenSavePopup();
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		// ポップアップ処理
		LoadScenePopup();
		SaveScenePopup();

		// 実際のセーブ処理の実行
		if (m_doOverwrite)
		{
			SaveScene(m_currentSceneGUID);
		}
	}
	void SceneViewPanel::LoadScenePopup()
	{
		if (m_openLoadPopup) { ImGui::OpenPopup("Load Scene Asset"); m_openLoadPopup = false; }
		if (ImGui::BeginPopupModal("Load Scene Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			const auto& _sceneMetaVec = Resource::AssetDatabase::Instance().GetTypeMetaVec("Scene");
			if (_sceneMetaVec.empty())
			{
				ImGui::TextDisabled("Not find SceneAsset");
			}

			for (const auto& _sceneMeta : _sceneMetaVec)
			{
				if (ImGui::Selectable(_sceneMeta.fileName.c_str(), m_currentSceneGUID == _sceneMeta.guid))
				{
					auto* _pScene = Engine::Scene::SceneManager::Instance().GetCurrentTopScene();
					if (_pScene)
					{
						// ロード処理
						Engine::Scene::SceneManager::Instance().SetNextScene(_sceneMeta.guid, Scene::SceneChangeType::Replace);
						ENGINE_LOG("シーンを読み込みました : %s", _sceneMeta.fileName.c_str());

						m_currentSceneGUID = _sceneMeta.guid; // 現在のGUIDを更新
						m_canOverwrite = true; // 上書き可能にする
					}
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::Separator();
			if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}
	}
	void SceneViewPanel::SaveScenePopup()
	{
		if (m_openSaveAsPopup) { ImGui::OpenPopup("Save Scene As"); m_openSaveAsPopup = false; }
		if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Input Filename (.scene) : ");

			// Enterキーで決定できるようにフラグを追加すると便利です
			bool isEnterPressed = ImGui::InputText("##scenename", &m_sceneNameInput, ImGuiInputTextFlags_EnterReturnsTrue);

			if (ImGui::Button("Save", ImVec2(120, 0)) || isEnterPressed)
			{
				if (!m_sceneNameInput.empty())
				{
					std::string dirPath = "Asset/Scenes/" + m_sceneNameInput;
					std::string filepath = dirPath + "/" + m_sceneNameInput;

					// サブディレクトリを作成するように修正
					std::filesystem::create_directories(dirPath);

					Engine::GUID _guid = Resource::AssetDatabase::Instance().AddMetaData(filepath, "Scene");
					m_currentSceneGUID = _guid;
					m_canOverwrite = true;

					SaveScene(m_currentSceneGUID);
					ImGui::CloseCurrentPopup(); // 保存後に閉じる
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); } // タイポ修正
			ImGui::EndPopup();
		}
	}
	void SceneViewPanel::OpenSavePopup()
	{
		m_openSaveAsPopup = true;
		m_sceneNameInput = "";
	}
	void SceneViewPanel::SaveScene(const Engine::GUID & a_guid)
	{
		// 現在のシーンを取得
		auto* _pScene = Engine::Scene::SceneManager::Instance().GetCurrentTopScene();
		if (!_pScene)
		{
			ENGINE_LOG("シーンのセーブに失敗しました");
			return;
		}

		// ファイルパスを取得
		auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		if (_path.empty())
		{
			ENGINE_LOG("シーンのセーブに失敗しました");
			return;
		}

		auto _fileDir = FileUtility::GetDirFromPath(_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(_path);
		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "scene");
		_pScene->Archive(_ar);
	}
}