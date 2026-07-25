#include "SceneViewPanel.h"
#include "../../../MainEngine.h"
#include "../../../Graphics/GraphicEngine.h"
#include "../../../Graphics/RenderGraph/RenderGraph.h"
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../../Scene/BaseScene/BaseScene.h"
#include "../../../Scene/SceneManager/SceneManager.h"

#include "../../../ECS/World/World.h"
#include "../../EditorCamera/EditorCamera.h"
#include "../../../../Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../Application/Components/Transform/WorldMatrixComponent.h"

// CollisionWorld は使わない。エディターでは当たり判定の有無に関わらず、
// 描画メッシュのAABBに対して直接レイ判定してエンティティを選択する。
#include "../../../Collision/CollisionCommon.h"
#include "../../../Collision/NarrowPhase/TestAABB/TestAABB.h"

#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Resource/Data/Model/Model.h"
#include "../../../Resource/Data/Mesh/Mesh.h"
#include "../../../../Application/Components/Resource/ModelComponent.h"

#include "../../../Option/OptionManager.h"

namespace Engine::Editor
{
	void Engine::Editor::SceneViewPanel::OnDrawImGui(EditorContext& a_editContext)
	{
		// シーンファイルメニューバー
		SceneFileMenu(a_editContext);

		// ワールド取得
		Engine::ECS::World* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit()) return;

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
		const auto* _pTex = _pRG->GetTmepTexture("AfterTAAColor");
		if (!_pTex)
		{
			ImGui::End();
			return;
		}

		// テクスチャの描画 : 実際に描画した範囲も取得
		auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
		// 表示アスペクトは実解像度(カメラ/アンプロジェクトが使う windowWidth/Height)に合わせる。
		// ここがずれるとスクリーン→ゲーム座標のスケールが X/Y で食い違い、ピッキングが横方向にずれる。
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		ImVec2 _actualRenderSize = Helper::DrawSRVView(
			_gpuHandle,
			static_cast<float>(_winOp.windowWidth),
			static_cast<float>(_winOp.windowHeight));
		ImVec2 _imageMin = ImGui::GetItemRectMin();   // 画像の実際の左上(スクリーン絶対座標)

		// フリーカメラへホバー状態を渡す。
		// 右クリックの開始位置がこのパネル内の時だけカメラ操作を始めるための判定。
		if (auto* _pEditorCam = MainEditor::Instance().RefEditorCamera())
		{
			_pEditorCam->SetViewportHovered(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows));
		}

		// シーンビュー上でエンティティをクリックした際に選択する
		SelectEntityForMouse(a_editContext,_pWorld, _imageMin,_actualRenderSize);

		// ギズモ描画
		GuizmoDraw(_imageMin, _actualRenderSize, a_editContext.entity, _pWorld);
	}
	void SceneViewPanel::SelectEntityForMouse(EditorContext& a_editContext, Engine::ECS::World* a_pWorld, const ImVec2& a_pos, const ImVec2& a_rect)
	{
		// カメラ・ワールドがなければ機能しない
		if (!a_editContext.pEditorCamera) return;
		if (!a_pWorld) return;

		// このパネル上にカーソルがあり、左クリックした瞬間だけ判定する
		// (毎フレーム判定すると選択が固定されて解除できなくなるため)
		if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) return;
		if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

		// ギズモ操作中/ギズモ上のクリックは移動操作なので選択には使わない
		if (ImGuizmo::IsUsing() || ImGuizmo::IsOver()) return;

		// スクリーン情報取得
		const auto& _windowOp = Option::OptionManager::GetInstance().GetWindowOption();

		// 画像左上を基準にしたローカルマウス座標
		ImVec2 _mousePos = ImGui::GetMousePos();
		DXSM::Vector2 _localMouse = {};
		_localMouse.x = _mousePos.x - a_pos.x;
		_localMouse.y = _mousePos.y - a_pos.y;

		// 表示サイズ → レンダーターゲット解像度へスケール
		if (a_rect.x <= 0.0f || a_rect.y <= 0.0f) return;
		DXSM::Vector2 _gameMouse = {};
		_gameMouse.x = _localMouse.x * (static_cast<float>(_windowOp.windowWidth)  / a_rect.x);
		_gameMouse.y = _localMouse.y * (static_cast<float>(_windowOp.windowHeight) / a_rect.y);

		// スクリーン座標からワールド空間のレイを作成
		Collision::RayInfo _ray = a_editContext.pEditorCamera->ScreenPointToRay(_gameMouse);

		// CollisionWorld を使わず、描画エンティティを直接ピッキングする
		Engine::ECS::Entity _picked = PickEntityByRay(a_pWorld, _ray);
		if (_picked != Engine::ECS::Limits::INVALID_ENTITY)
		{
			a_editContext.entity = _picked;
		}
	}
	Engine::ECS::Entity SceneViewPanel::PickEntityByRay(Engine::ECS::World* a_pWorld, const Engine::Collision::RayInfo& a_ray)
	{
		Engine::ECS::Entity _picked = Engine::ECS::Limits::INVALID_ENTITY;
		if (!a_pWorld) return _picked;

		// レイ方向を正規化しておく(AABB判定は単位ベクトルを前提にしている)
		Collision::RayInfo _ray = a_ray;
		{
			DXSM::Vector3 _dir(_ray.direction);
			if (_dir.LengthSquared() < 1e-12f) return _picked;
			_dir.Normalize();
			_ray.direction = _dir;
		}

		// これまでに見つかった最も手前のヒット距離
		float _closest = _ray.maxDistance;

		// ModelComponent と WorldMatrixComponent を持つ全エンティティを走査し、
		// AABBで枝刈りしたうえで描画メッシュの三角形とレイを厳密判定する。
		// CollisionWorld を介さないので、当たり判定を持たないエンティティも選択できる。
		a_pWorld->ForEach<ModelComponent, WorldMatrixComponent>(
			[&](Engine::ECS::ArchetypeChunk* a_pChunk, uint32_t a_count,
				ModelComponent* a_models, WorldMatrixComponent* a_worlds)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					const auto* _pModel = Resource::ResourceManager::Instance().Get(a_models[_i].handle);
					if (!_pModel) continue;

					DirectX::XMMATRIX _instWorld = DirectX::XMLoadFloat4x4(&a_worlds[_i].worldMat);

					// 描画メッシュノードごとに判定
					for (int _nodeIdx : _pModel->GetDrawNodeVec())
					{
						const Engine::Resource::Node& _node = _pModel->GetOriginalNodeVec()[_nodeIdx];
						DirectX::XMMATRIX _nodeGlobal = DirectX::XMLoadFloat4x4(&_node.worldTransform);
						DirectX::XMMATRIX _meshWorld = DirectX::XMMatrixMultiply(_nodeGlobal, _instWorld);

						for (int _meshIdx : _node.meshIndices)
						{
							const auto& _meshHandle = _pModel->GetMeshHandles()[_meshIdx];
							const auto* _pMesh = Resource::ResourceManager::Instance().Get(_meshHandle);
							if (!_pMesh) continue;

							// --- ブロードフェーズ ---
							// まずワールドAABBで大まかに枝刈りし、無関係なメッシュの三角形走査を省く。
							DirectX::BoundingBox _worldBox;
							_pMesh->GetMetaData().aabb.Transform(_worldBox, _meshWorld);
							float _boxDist = 0.0f;
							if (!Collision::NarrowPhase::TestAABB(_ray, _worldBox, _boxDist)) continue;
							if (_boxDist > _closest) continue;	// 既に手前で当たっていれば不要

							// --- ナローフェーズ ---
							// 描画メッシュの三角形とレイを厳密判定する(当たり判定メッシュは使わない)。
							// 頂点をワールドへ変換し、ワールド空間のレイでそのまま交差を取る。
							const auto& _verts = _pMesh->GetVertexVec();
							const auto& _faces = _pMesh->GetFaceVec();
							const size_t _vertCount = _verts.size();

							DirectX::XMVECTOR _ro = DirectX::XMLoadFloat3(&_ray.origin);
							DirectX::XMVECTOR _rd = DirectX::XMLoadFloat3(&_ray.direction);	// 正規化済み

							for (const auto& _f : _faces)
							{
								// 不正インデックス保護
								if (_f.idx[0] >= _vertCount || _f.idx[1] >= _vertCount || _f.idx[2] >= _vertCount) continue;

								DirectX::XMVECTOR _p0 = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&_verts[_f.idx[0]].pos), _meshWorld);
								DirectX::XMVECTOR _p1 = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&_verts[_f.idx[1]].pos), _meshWorld);
								DirectX::XMVECTOR _p2 = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&_verts[_f.idx[2]].pos), _meshWorld);

								float _t = 0.0f;
								if (DirectX::TriangleTests::Intersects(_ro, _rd, _p0, _p1, _p2, _t))
								{
									// レイ方向は単位ベクトルなので _t はワールド距離
									if (_t >= 0.0f && _t < _closest)
									{
										_closest = _t;
										_picked = a_pChunk->entityData[_i];
									}
								}
							}
						}
					}
				}
			}
		);

		return _picked;
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
					ENGINE_LOG("新規シーンを作成する処理はまだありません。");
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

			// エディター用フリーカメラの設定
			if (ImGui::BeginMenu("Camera"))
			{
				if (a_editContext.pEditorCamera)
				{
					a_editContext.pEditorCamera->DrawEditUI();
				}
				else
				{
					ImGui::TextDisabled("EditorCamera is null");
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

		// どのシーンを保存するかをログ出力する
		ENGINE_LOG("[Scene] セーブ : %s", _path.c_str());

		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "scene");
		_pScene->Archive(_ar);
	}
}