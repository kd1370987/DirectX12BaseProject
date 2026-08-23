#include "SceneViewPanel.h"
#include "../../../MainEngine.h"
#include "../../../Graphics/GraphicEngine.h"
#include "../../../Graphics/RenderGraph/RenderGraph.h"
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../../Scene/BaseScene/BaseScene.h"
#include "../../../Scene/SceneManager/SceneManager.h"
#include "../../../GameObject/GameObjectManager/GameObjectManager.h"

#include "../../../ECS/World/World.h"
#include "../../EditorCamera/EditorCamera.h"
#include "../../Helper/EditorHelper.h"
#include "../../../../Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../Application/Components/Transform/WorldMatrixComponent.h"
#include "../../../../Application/Components/Hierarchy/HierarchyComponent.h"
#include "../../../../Application/Components/Persistence/GUIDComponent.h"

// HUD表示に使うコンポーネント群(オフセット・パーティクルの発生方向など)
#include "../../../../Application/Components/Hierarchy/FollowAnimationNodeComponent.h"
#include "../../../../Application/Components/Resource/ParticlesComponent.h"
#include "../../../../Application/Components/Camera/CameraFocusTargetComponent.h"
#include "../../../../Application/Components/Character/LookAngleComponent.h"
#include "../../../../Application/Components/Camera/TPSOffsetComponent.h"
#include "../../../../Application/Components/Camera/FollowTargetComponent.h"

// CollisionWorld は使わない。エディターでは当たり判定の有無に関わらず、
// 描画メッシュのAABBに対して直接レイ判定してエンティティを選択する。
#include "../../../Collision/CollisionCommon.h"
#include "../../../Collision/NarrowPhase/TestAABB/TestAABB.h"

#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Resource/Data/Model/Model.h"
#include "../../../Resource/Data/Mesh/Mesh.h"
#include "../../../../Application/Components/Resource/ModelComponent.h"

#include "../../../Option/OptionManager.h"

#include "../../../GameObject/BaseObject/BaseObject.h"

namespace
{
	//======================================================================================
	// シーンビュー画像の上へ、ワールド座標の点・線・矢印を直接描くためのヘルパー。
	//
	// ImGuizmo に渡しているのと同じ矩形(画像の左上と表示サイズ)を基準にするので、
	// ギズモと表示位置がずれない。ImDrawList へ描くだけなので描画パスには一切触らない。
	//======================================================================================
	struct HudPainter
	{
		DirectX::XMMATRIX	viewProj	= DirectX::XMMatrixIdentity();
		ImVec2				origin		= {};		// 画像左上(スクリーン絶対座標)
		ImVec2				size		= {};		// 画像の表示サイズ
		ImDrawList*			pDrawList	= nullptr;

		//----------------------------------------------------------------------------------
		// ワールド座標 → シーンビュー画像上のスクリーン座標。
		// カメラの後ろ(w<=0)にある点は描けないので false を返す。
		//----------------------------------------------------------------------------------
		bool ToScreen(const DXSM::Vector3& a_world, ImVec2& a_out) const
		{
			// w で割る必要があるので TransformCoord ではなく Transform を使う
			DirectX::XMVECTOR _clip = DirectX::XMVector3Transform(Math::DX::Load(a_world), viewProj);

			const float _w = DirectX::XMVectorGetW(_clip);
			if (_w <= 1e-4f) return false;

			const float _ndcX = DirectX::XMVectorGetX(_clip) / _w;
			const float _ndcY = DirectX::XMVectorGetY(_clip) / _w;

			a_out.x = origin.x + (_ndcX * 0.5f + 0.5f) * size.x;
			a_out.y = origin.y + (1.0f - (_ndcY * 0.5f + 0.5f)) * size.y;
			return true;
		}

		//----------------------------------------------------------------------------------
		// 位置を示す丸+十字のマーカー。ラベルを渡すと右下に文字を添える。
		//----------------------------------------------------------------------------------
		void Marker(const DXSM::Vector3& a_world, ImU32 a_col, const char* a_label, float a_radius = 5.0f) const
		{
			ImVec2 _sp;
			if (!ToScreen(a_world, _sp)) return;

			pDrawList->AddCircle(_sp, a_radius, a_col, 12, 1.5f);
			pDrawList->AddLine(ImVec2(_sp.x - a_radius * 2.0f, _sp.y), ImVec2(_sp.x + a_radius * 2.0f, _sp.y), a_col, 1.0f);
			pDrawList->AddLine(ImVec2(_sp.x, _sp.y - a_radius * 2.0f), ImVec2(_sp.x, _sp.y + a_radius * 2.0f), a_col, 1.0f);

			if (a_label && a_label[0] != '\0')
			{
				// 背景色の上でも読めるよう、黒を1pxずらして敷いてから本体を描く
				const ImVec2 _textPos(_sp.x + a_radius * 2.0f + 2.0f, _sp.y + 2.0f);
				pDrawList->AddText(ImVec2(_textPos.x + 1.0f, _textPos.y + 1.0f), IM_COL32(0, 0, 0, 200), a_label);
				pDrawList->AddText(_textPos, a_col, a_label);
			}
		}

		//----------------------------------------------------------------------------------
		// ワールド空間の線分。どちらかの端がカメラ後ろなら描かない。
		//----------------------------------------------------------------------------------
		void Line(const DXSM::Vector3& a_from, const DXSM::Vector3& a_to, ImU32 a_col, float a_thickness = 1.5f) const
		{
			ImVec2 _a, _b;
			if (!ToScreen(a_from, _a)) return;
			if (!ToScreen(a_to, _b)) return;
			pDrawList->AddLine(_a, _b, a_col, a_thickness);
		}

		//----------------------------------------------------------------------------------
		// 方向を示す矢印。矢じりはスクリーン空間で作るので、向きに関わらず同じ大きさで見える。
		//----------------------------------------------------------------------------------
		void Arrow(const DXSM::Vector3& a_from, const DXSM::Vector3& a_dir, float a_length, ImU32 a_col, const char* a_label) const
		{
			const DXSM::Vector3 _tip = a_from + a_dir * a_length;

			ImVec2 _a, _b;
			if (!ToScreen(a_from, _a)) return;
			if (!ToScreen(_tip, _b)) return;

			pDrawList->AddLine(_a, _b, a_col, 2.0f);

			// スクリーン上での向きから矢じりを作る
			ImVec2 _v(_b.x - _a.x, _b.y - _a.y);
			const float _len = std::sqrt(_v.x * _v.x + _v.y * _v.y);
			if (_len > 1e-3f)
			{
				_v.x /= _len;
				_v.y /= _len;
				const ImVec2 _n(-_v.y, _v.x);
				const float _head = 10.0f;
				const ImVec2 _p1(_b.x - _v.x * _head + _n.x * _head * 0.5f, _b.y - _v.y * _head + _n.y * _head * 0.5f);
				const ImVec2 _p2(_b.x - _v.x * _head - _n.x * _head * 0.5f, _b.y - _v.y * _head - _n.y * _head * 0.5f);
				pDrawList->AddTriangleFilled(_b, _p1, _p2, a_col);
			}

			if (a_label && a_label[0] != '\0')
			{
				pDrawList->AddText(ImVec2(_b.x + 7.0f, _b.y + 1.0f), IM_COL32(0, 0, 0, 200), a_label);
				pDrawList->AddText(ImVec2(_b.x + 6.0f, _b.y), a_col, a_label);
			}
		}

		//----------------------------------------------------------------------------------
		// 指定軸を法線とする円。半径のばらつきなどを見せるのに使う。
		//----------------------------------------------------------------------------------
		void Circle(const DXSM::Vector3& a_center, const DXSM::Vector3& a_axis, float a_radius, ImU32 a_col) const
		{
			if (a_radius <= 0.0f) return;

			DXSM::Vector3 _u, _v;
			MakeBasis(a_axis, _u, _v);

			constexpr int _segments = 24;
			ImVec2 _prev;
			bool _hasPrev = false;
			for (int _i = 0; _i <= _segments; ++_i)
			{
				const float _rad = DirectX::XM_2PI * static_cast<float>(_i) / static_cast<float>(_segments);
				const DXSM::Vector3 _p = a_center + (_u * std::cos(_rad) + _v * std::sin(_rad)) * a_radius;

				ImVec2 _sp;
				if (!ToScreen(_p, _sp)) { _hasPrev = false; continue; }
				if (_hasPrev) pDrawList->AddLine(_prev, _sp, a_col, 1.0f);
				_prev = _sp;
				_hasPrev = true;
			}
		}

		//----------------------------------------------------------------------------------
		// 発生方向の拡散角を示すコーン(母線 + 底面の円)。
		//----------------------------------------------------------------------------------
		void Cone(const DXSM::Vector3& a_apex, const DXSM::Vector3& a_dir, float a_length, float a_angleDeg, ImU32 a_col) const
		{
			if (a_angleDeg <= 0.0f || a_length <= 0.0f) return;

			// 90度以上は円錐にならないので描画上の上限を設ける
			const float _angle = DirectX::XMConvertToRadians(std::min(a_angleDeg, 89.0f));
			const float _radius = a_length * std::tan(_angle);
			const DXSM::Vector3 _center = a_apex + a_dir * a_length;

			DXSM::Vector3 _u, _v;
			MakeBasis(a_dir, _u, _v);

			// 母線(4本だけ描いて円錐と分かる程度に留める)
			constexpr int _lines = 4;
			for (int _i = 0; _i < _lines; ++_i)
			{
				const float _rad = DirectX::XM_2PI * static_cast<float>(_i) / static_cast<float>(_lines);
				Line(a_apex, _center + (_u * std::cos(_rad) + _v * std::sin(_rad)) * _radius, a_col, 1.0f);
			}

			Circle(_center, a_dir, _radius, a_col);
		}

	private:

		//----------------------------------------------------------------------------------
		// 軸に直交する正規直交基底を作る。軸と平行にならない参照ベクトルを選ぶ。
		//----------------------------------------------------------------------------------
		static void MakeBasis(const DXSM::Vector3& a_axis, DXSM::Vector3& a_outU, DXSM::Vector3& a_outV)
		{
			DXSM::Vector3 _axis = a_axis;
			if (_axis.LengthSquared() < 1e-8f) _axis = DXSM::Vector3(0.0f, 0.0f, 1.0f);
			_axis.Normalize();

			const DXSM::Vector3 _ref = (std::fabs(_axis.y) > 0.99f)
				? DXSM::Vector3(1.0f, 0.0f, 0.0f)
				: DXSM::Vector3(0.0f, 1.0f, 0.0f);

			a_outU = _axis.Cross(_ref);
			a_outU.Normalize();
			a_outV = _axis.Cross(a_outU);
			a_outV.Normalize();
		}
	};

	// HUDの色分け(用途ごとに色を固定しておくと画面上で見分けやすい)
	constexpr ImU32 HUD_COL_PARENT		= IM_COL32(120, 200, 255, 255);	// 親子関係
	constexpr ImU32 HUD_COL_OFFSET		= IM_COL32(255, 200, 90, 255);	// 各種オフセット
	constexpr ImU32 HUD_COL_PARTICLE	= IM_COL32(120, 255, 160, 255);	// パーティクル発生源
	constexpr ImU32 HUD_COL_CAMERA		= IM_COL32(255, 130, 200, 255);	// カメラ関連
}

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
		const auto* _pTex = _pRG->GetTmepTexture("FinalColor");
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
		ImVec2 _actualRenderSize = EditorHelper::DrawSRVView(
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

		// エンティティコピー(Ctrl+C / Ctrl+V)
		CopyEntities(a_editContext, _pWorld);

		// ギズモ描画
		// ゲームオブジェクト(Gameモード)を選択中ならそちらのギズモを、
		// そうでなければ従来通りECSエンティティのギズモを出す。
		if (a_editContext.eInspectorType == EInspectorType::Game && a_editContext.pGameObject)
		{
			GameObjectGizmoDraw(_imageMin, _actualRenderSize, a_editContext);
		}
		else
		{
			// ギズモはプライマリ選択(選択リストの先頭)に出し、
			// 動かした分は選択中の全エンティティへ適用する
			GuizmoDraw(_imageMin, _actualRenderSize, a_editContext, _pWorld);
		}
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
			// LCtrl押下中は追加選択(再クリックで解除)、押していなければ単体選択に切り替わる
			a_editContext.SelectEntity(_picked, a_editContext.m_isSelecting);
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

					DirectX::XMMATRIX _instWorld = Math::DX::Load(a_worlds[_i].worldMat);

					// 描画メッシュノードごとに判定
					for (int _nodeIdx : _pModel->GetDrawNodeVec())
					{
						const Engine::Resource::Node& _node = _pModel->GetOriginalNodeVec()[_nodeIdx];
						DirectX::XMMATRIX _nodeGlobal = Math::DX::Load(_node.worldTransform);
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

							DirectX::XMVECTOR _ro = Math::DX::Load(_ray.origin);
							DirectX::XMVECTOR _rd = Math::DX::Load(_ray.direction);	// 正規化済み

							for (const auto& _f : _faces)
							{
								// 不正インデックス保護
								if (_f.idx[0] >= _vertCount || _f.idx[1] >= _vertCount || _f.idx[2] >= _vertCount) continue;

								DirectX::XMVECTOR _p0 = DirectX::XMVector3TransformCoord(Math::DX::Load(_verts[_f.idx[0]].pos), _meshWorld);
								DirectX::XMVECTOR _p1 = DirectX::XMVector3TransformCoord(Math::DX::Load(_verts[_f.idx[1]].pos), _meshWorld);
								DirectX::XMVECTOR _p2 = DirectX::XMVector3TransformCoord(Math::DX::Load(_verts[_f.idx[2]].pos), _meshWorld);

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
	void SceneViewPanel::GuizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect, EditorContext& a_editContext, Engine::ECS::World* a_pWorld)
	{
		// 現在のウィンドウの描画リストにギズモを追加するよう指示
		ImGuizmo::SetDrawlist();

		// ギズモの操作・描画領域を、テクスチャの領域にぴったり合わせる
		ImGuizmo::SetRect(a_pos.x, a_pos.y, a_rect.x, a_rect.y);

		if (!a_pWorld) return;

		// ギズモを出す基準はプライマリ選択(選択リストの先頭)
		const ECS::Entity _primaryEntity = a_editContext.GetPrimaryEntity();
		if (_primaryEntity == ECS::Limits::INVALID_ENTITY) return;

		// オフセットやパーティクル方向のHUDは、
		// トランスフォームを持たないエンティティでも出したいのでギズモより先に描く。
		DrawEntityHUD(a_pos, a_rect, _primaryEntity, a_pWorld);

		// トランスフォームを持っているかチェック
		if (!a_pWorld->HasComponent<LocalTransformComponent>(_primaryEntity)) return;

		// 取得
		auto* _pTrsComp = a_pWorld->RefData<LocalTransformComponent>(_primaryEntity);
		if (!_pTrsComp) return;

		auto* _pWorldComp = a_pWorld->RefData<WorldMatrixComponent>(_primaryEntity);
		if (!_pWorldComp) return;

		// 現在のカメラ行列を取得
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;
		const auto& _camData = _pGE->GetCPUCameraData();

		// Pos/Quat/Scale から 4x4ワールド行列を合成
		DirectX::XMVECTOR _vScale = Math::DX::Load(_pTrsComp->scale);
		DirectX::XMVECTOR _vQuat = Math::DX::Load(_pTrsComp->quat);
		DirectX::XMVECTOR _vPos = Math::DX::Load(_pTrsComp->pos);

		// アフィン変換行列を作成(親を持つ場合、これは親基準のローカル行列)
		DirectX::XMMATRIX _mLocal = DirectX::XMMatrixAffineTransformation(_vScale, DirectX::XMVectorZero(), _vQuat, _vPos);

		// 親のワールド行列を取得する。
		// LocalTransform は親基準なので、そのままギズモへ渡すと親が動いている分だけ
		// ギズモが実際の見た目の位置からずれる。CommitHierarchyWorldMatrixSystem と
		// 同じ world = local * parentWorld で合成してから渡す。
		DirectX::XMMATRIX _mParent = DirectX::XMMatrixIdentity();
		const bool _hasParent = TryGetParentWorldMatrix(a_pWorld, _primaryEntity, _mParent);

		DirectX::XMMATRIX _mWorld = _hasParent ? (_mLocal * _mParent) : _mLocal;

		DirectX::XMFLOAT4X4 _worldFloat4x4;
		_worldFloat4x4 = Math::DX::StoreMatrix(_mWorld);
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
		if (ImGuizmo::IsUsing())
		{
			DirectX::XMMATRIX _updatedWorld = Math::DX::Load(_worldFloat4x4);

			// 移動のみのギズモなので、ワールド空間での平行移動量だけを取り出す。
			// 行列を分解して書き戻す方式だと複数エンティティへ同じ操作を配れないため、
			// 「移動量」に落としてから選択中の各エンティティへ適用する。
			DirectX::XMVECTOR _worldDelta = DirectX::XMVectorSubtract(_updatedWorld.r[3], _mWorld.r[3]);

			// 動いていないフレームは何もしない(isDirtyを立て続けないため)
			if (DirectX::XMVector3NearEqual(_worldDelta, DirectX::XMVectorZero(), DirectX::XMVectorReplicate(1e-8f)))
			{
				return;
			}

			// 選択中の全エンティティへ同じ移動量を適用する
			for (const ECS::Entity& _entity : a_editContext.selectedEntities)
			{
				if (_entity == ECS::Limits::INVALID_ENTITY) continue;

				// 祖先も一緒に選ばれている場合、親の移動が子へ伝播するので自分では動かさない。
				// (動かすと親の分と自分の分で二重に移動してしまう)
				if (IsAncestorSelected(a_pWorld, a_editContext, _entity)) continue;

				TranslateEntity(a_pWorld, _entity, _worldDelta);
			}
		}
	}

	void SceneViewPanel::TranslateEntity(Engine::ECS::World* a_pWorld, const ECS::Entity& a_entity, DirectX::FXMVECTOR a_worldDelta)
	{
		if (!a_pWorld) return;
		if (!a_pWorld->HasComponent<LocalTransformComponent>(a_entity)) return;

		auto* _pTrsComp = a_pWorld->RefData<LocalTransformComponent>(a_entity);
		if (!_pTrsComp) return;

		// LocalTransform は親基準なので、ワールドの移動量を親空間へ変換してから足す
		DirectX::XMVECTOR _localDelta = a_worldDelta;

		DirectX::XMMATRIX _mParent = DirectX::XMMatrixIdentity();
		if (TryGetParentWorldMatrix(a_pWorld, a_entity, _mParent))
		{
			DirectX::XMVECTOR _det;
			DirectX::XMMATRIX _invParent = DirectX::XMMatrixInverse(&_det, _mParent);

			// 親のスケールが0などで逆行列が作れない場合、
			// そのまま書き戻すとローカルがNaNで壊れるので動かさない。
			if (DirectX::XMVector4Equal(_det, DirectX::XMVectorZero())) return;

			// 位置ではなく変位なので TransformNormal(平行移動成分を無視)を使う
			_localDelta = DirectX::XMVector3TransformNormal(a_worldDelta, _invParent);
		}

		DirectX::XMVECTOR _pos = DirectX::XMVectorAdd(Math::DX::Load(_pTrsComp->pos), _localDelta);
		_pTrsComp->pos = Math::DX::StoreVector3(_pos);
		_pTrsComp->isDirty = true;

		// ワールド行列も同フレーム中に追従させる。
		// ここを更新しないと、次のフレームで行列が再計算されるまでギズモとHUDの位置が1フレーム遅れる。
		if (a_pWorld->HasComponent<WorldMatrixComponent>(a_entity))
		{
			if (auto* _pWorldComp = a_pWorld->RefData<WorldMatrixComponent>(a_entity))
			{
				DirectX::XMMATRIX _mWorld = Math::DX::Load(_pWorldComp->worldMat);
				_mWorld.r[3] = DirectX::XMVectorAdd(_mWorld.r[3], a_worldDelta);
				_pWorldComp->worldMat = Math::DX::StoreMatrix(_mWorld);
			}
		}
	}

	bool SceneViewPanel::IsAncestorSelected(Engine::ECS::World* a_pWorld, const EditorContext& a_editContext, const ECS::Entity& a_entity)
	{
		if (!a_pWorld) return false;

		// 親子関係が万が一循環していても止まらなくならないよう、たどる深さに上限を設ける
		constexpr int _maxDepth = 64;

		ECS::Entity _current = a_entity;
		for (int _i = 0; _i < _maxDepth; ++_i)
		{
			if (!a_pWorld->HasComponent<HierarchyComponent>(_current)) return false;

			auto* _pHierarchy = a_pWorld->RefData<HierarchyComponent>(_current);
			if (!_pHierarchy) return false;

			_current = _pHierarchy->parentID;
			if (_current == ECS::Limits::INVALID_ENTITY) return false;

			if (a_editContext.IsSelectedEntity(_current)) return true;
		}
		return false;
	}
	bool SceneViewPanel::TryGetParentWorldMatrix(Engine::ECS::World* a_pWorld, const ECS::Entity& a_entity, DirectX::XMMATRIX& a_outParentMat)
	{
		a_outParentMat = DirectX::XMMatrixIdentity();

		if (!a_pWorld) return false;
		if (a_entity == ECS::Limits::INVALID_ENTITY) return false;
		if (!a_pWorld->HasComponent<HierarchyComponent>(a_entity)) return false;

		auto* _pHierarchy = a_pWorld->RefData<HierarchyComponent>(a_entity);
		if (!_pHierarchy) return false;

		// 親はランタイム側のIDで引く(GUIDはPostDeserializeで解決済み)
		const ECS::Entity _parent = _pHierarchy->parentID;
		if (_parent == ECS::Limits::INVALID_ENTITY) return false;
		if (!a_pWorld->HasComponent<WorldMatrixComponent>(_parent)) return false;

		auto* _pParentWorldComp = a_pWorld->RefData<WorldMatrixComponent>(_parent);
		if (!_pParentWorldComp) return false;

		a_outParentMat = Math::DX::Load(_pParentWorldComp->worldMat);
		return true;
	}
	void SceneViewPanel::DrawEntityHUD(const ImVec2& a_pos, const ImVec2& a_rect, const ECS::Entity& a_entity, Engine::ECS::World* a_pWorld)
	{
		if (!a_pWorld) return;
		if (a_entity == ECS::Limits::INVALID_ENTITY) return;
		if (a_rect.x <= 0.0f || a_rect.y <= 0.0f) return;

		// 現在のカメラ行列を取得
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;
		const auto& _camData = _pGE->GetCPUCameraData();

		HudPainter _hud;
		_hud.viewProj =
			Math::DX::Load(_camData.viewMat) *
			Math::DX::Load(_camData.projMat);
		_hud.origin = a_pos;
		_hud.size = a_rect;
		_hud.pDrawList = ImGui::GetWindowDrawList();
		if (!_hud.pDrawList) return;

		//==================================================================================
		// 基準になるワールド行列
		// ワールド行列を持たないエンティティでもオフセットは見せたいので、
		// その場合は LocalTransform から組む(親を持たないものと同じ扱いになる)。
		//==================================================================================
		DXSM::Matrix _world = DXSM::Matrix::Identity;
		if (a_pWorld->HasComponent<WorldMatrixComponent>(a_entity))
		{
			if (auto* _pWorldComp = a_pWorld->RefData<WorldMatrixComponent>(a_entity))
			{
				_world = DXSM::Matrix(_pWorldComp->worldMat);
			}
		}
		else if (a_pWorld->HasComponent<LocalTransformComponent>(a_entity))
		{
			if (auto* _pTrsComp = a_pWorld->RefData<LocalTransformComponent>(a_entity))
			{
				_world = DXSM::Matrix(DirectX::XMMatrixAffineTransformation(
					Math::DX::Load(_pTrsComp->scale),
					DirectX::XMVectorZero(),
					Math::DX::Load(_pTrsComp->quat),
					Math::DX::Load(_pTrsComp->pos)));
			}
		}
		const DXSM::Vector3 _originPos = _world.Translation();

		//==================================================================================
		// 親子関係
		// 親の原点と線で結び、どこを基準にしたローカル座標なのかを一目で分かるようにする。
		//==================================================================================
		DirectX::XMMATRIX _mParent = DirectX::XMMatrixIdentity();
		const bool _hasParent = TryGetParentWorldMatrix(a_pWorld, a_entity, _mParent);
		if (_hasParent)
		{
			const DXSM::Vector3 _parentPos = DXSM::Matrix(_mParent).Translation();
			_hud.Line(_parentPos, _originPos, HUD_COL_PARENT, 1.5f);
			_hud.Marker(_parentPos, HUD_COL_PARENT, "Parent", 4.0f);
		}

		//==================================================================================
		// アニメーションノード追従のオフセット
		// FollowAnimationNodeSystem は local = offsetMat * nodeMat を書き込むので、
		// 逆に world へ offsetMat の逆行列を掛ければ追従元のノード位置が求まる。
		// (ノードポーズ配列を引かなくてよいので、エディター側だけで完結する)
		//==================================================================================
		if (a_pWorld->HasComponent<FollowAnimationNodeComponent>(a_entity))
		{
			if (auto* _pFollowNode = a_pWorld->RefData<FollowAnimationNodeComponent>(a_entity))
			{
				const DirectX::XMMATRIX _offsetMat =
					DirectX::XMMatrixRotationQuaternion(Math::DX::Load(_pFollowNode->offsetRotation)) *
					DirectX::XMMatrixScalingFromVector(Math::DX::Load(_pFollowNode->offsetScale)) *
					DirectX::XMMatrixTranslationFromVector(Math::DX::Load(_pFollowNode->offsetPosition));

				// offsetScale が 0 だと逆行列を作れないので、その時はノード位置を出さない
				DirectX::XMVECTOR _det;
				const DirectX::XMMATRIX _invOffset = DirectX::XMMatrixInverse(&_det, _offsetMat);
				if (!DirectX::XMVector4Equal(_det, DirectX::XMVectorZero()))
				{
					const DirectX::XMMATRIX _nodeWorld = _invOffset * Math::DX::Load(_world);
					const DXSM::Vector3 _nodePos = DXSM::Matrix(_nodeWorld).Translation();
					_hud.Marker(_nodePos, HUD_COL_OFFSET, "Node", 4.0f);
					_hud.Line(_nodePos, _originPos, HUD_COL_OFFSET, 1.5f);
					_hud.Marker(_originPos, HUD_COL_OFFSET, "NodeOffset", 5.0f);
				}
			}
		}

		//==================================================================================
		// パーティクルの発生源と発生方向
		// EmitParticleSystem と同じ式で位置・方向を求めるので、
		// ここで見えているものがそのまま実際の発生位置・方向になる。
		//==================================================================================
		if (a_pWorld->HasComponent<ParticlesComponent>(a_entity))
		{
			if (auto* _pParticles = a_pWorld->RefData<ParticlesComponent>(a_entity))
			{
				DXSM::Vector3 _emitPos;
				DXSM::Vector3 _emitDir;

				switch (_pParticles->emitSpace)
				{
				case EEmitSpace::WorldMatrix:
					_emitPos = _originPos;
					_emitDir = DXSM::Vector3(_world._31, _world._32, _world._33);
					break;

				case EEmitSpace::LocalOffset:
					_emitPos = DXSM::Vector3::Transform(DXSM::Vector3(_pParticles->posOffset), _world);
					_emitDir = DXSM::Vector3::TransformNormal(DXSM::Vector3(_pParticles->emitDir), _world);
					break;

				case EEmitSpace::FixedWorld:
				default:
					_emitPos = DXSM::Vector3(_pParticles->worldPos);
					_emitDir = DXSM::Vector3(_pParticles->emitDir);
					break;
				}

				if (_emitDir.LengthSquared() > 1e-8f) _emitDir.Normalize();
				else                                  _emitDir = DXSM::Vector3(0.0f, 0.0f, 1.0f);

				// 矢印の長さは見やすさ優先の固定値。拡散コーンもこの長さを基準に描く。
				constexpr float _arrowLength = 1.5f;
				const ImU32 _weakCol = IM_COL32(120, 255, 160, 140);

				// オブジェクト本体からどれだけずれた位置で出るのかを線で見せる
				if (_pParticles->emitSpace != EEmitSpace::WorldMatrix)
				{
					_hud.Line(_originPos, _emitPos, HUD_COL_OFFSET, 1.0f);
				}

				_hud.Marker(_emitPos, HUD_COL_PARTICLE, "Emit", 5.0f);
				_hud.Arrow(_emitPos, _emitDir, _arrowLength, HUD_COL_PARTICLE, "EmitDir");

				// 発生位置のばらつき(半径)と方向のばらつき(角度)
				_hud.Circle(_emitPos, _emitDir, _pParticles->positionRadius, _weakCol);
				_hud.Cone(_emitPos, _emitDir, _arrowLength, _pParticles->directionAngle, _weakCol);
			}
		}

		//==================================================================================
		// カメラの注視点オフセット
		// TPSSystem は offsetPos をカメラ空間(オービット基準)として扱い、
		// 注視点 = ターゲット座標 + オービットで回したオフセット で求める。
		// (左手系なので +X が画面右、+Y が画面上、+Z が視線の奥)
		//
		// オービットは LookAngleComponent の Yaw/Pitch へ追従するので、
		// ここでも同じ角度から回転を組む。機体の姿勢(quat)で回すと、
		// 胴体が視線と別方向を向いている間だけ表示がずれる。
		// LookAngle を持たないエンティティはワールド軸で出す(回転なし)。
		//==================================================================================
		if (a_pWorld->HasComponent<CameraFocusTargetComponent>(a_entity))
		{
			if (auto* _pFocus = a_pWorld->RefData<CameraFocusTargetComponent>(a_entity))
			{
				DXSM::Quaternion _orbit = DXSM::Quaternion::Identity;
				if (a_pWorld->HasComponent<LookAngleComponent>(a_entity))
				{
					if (auto* _pLookAng = a_pWorld->RefData<LookAngleComponent>(a_entity))
					{
						// 角度は度で保持されている。Vector3 オーバーロードは軸が
						// 入れ替わるのでスカラー版を明示的に使う(TPSSystem と同じ式)。
						_orbit = DXSM::Quaternion::CreateFromYawPitchRoll(
							DirectX::XMConvertToRadians(_pLookAng->Yaw),
							DirectX::XMConvertToRadians(-_pLookAng->Pitch),
							0.0f);
						_orbit.Normalize();
					}
				}

				const DXSM::Vector3 _focusPos =
					_originPos + DXSM::Vector3::Transform(DXSM::Vector3(_pFocus->offsetPos), _orbit);
				_hud.Line(_originPos, _focusPos, HUD_COL_CAMERA, 1.0f);
				_hud.Marker(_focusPos, HUD_COL_CAMERA, "FocusOffset", 5.0f);
			}
		}

		//==================================================================================
		// TPSカメラのオフセット
		// y はピボットの高さ、z は引きの距離として使われる(TPSSystem)。
		// 追従ターゲットが引ければピボット位置を、引けなければ自分基準のオフセットを出す。
		//==================================================================================
		if (a_pWorld->HasComponent<TPSOffsetComponent>(a_entity))
		{
			if (auto* _pOffset = a_pWorld->RefData<TPSOffsetComponent>(a_entity))
			{
				ECS::Entity _target = ECS::Limits::INVALID_ENTITY;
				if (a_pWorld->HasComponent<FollowTargetComponent>(a_entity))
				{
					if (auto* _pFollow = a_pWorld->RefData<FollowTargetComponent>(a_entity))
					{
						_target = _pFollow->target;
					}
				}

				const LocalTransformComponent* _pTargetTrs = nullptr;
				if (_target != ECS::Limits::INVALID_ENTITY &&
					a_pWorld->HasComponent<LocalTransformComponent>(_target))
				{
					_pTargetTrs = a_pWorld->RefData<LocalTransformComponent>(_target);
				}

				if (_pTargetTrs)
				{
					// ピボット = ターゲット座標 + 上方向 * y
					const DXSM::Vector3 _pivot =
						DXSM::Vector3(_pTargetTrs->pos) + DXSM::Vector3::Up * _pOffset->y;

					_hud.Marker(_pivot, HUD_COL_CAMERA, "TPS Pivot", 5.0f);
					// ピボットから現在のカメラ位置まで。ここの長さが実際の引き量になる。
					_hud.Line(_pivot, _originPos, HUD_COL_CAMERA, 1.0f);
				}
				else
				{
					const DXSM::Vector3 _offsetPos =
						_originPos + DXSM::Vector3(_pOffset->x, _pOffset->y, _pOffset->z);
					_hud.Line(_originPos, _offsetPos, HUD_COL_CAMERA, 1.0f);
					_hud.Marker(_offsetPos, HUD_COL_CAMERA, "TPS Offset", 5.0f);
				}
			}
		}
	}
	void SceneViewPanel::GameObjectGizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect, EditorContext& a_editContext)
	{
		auto* _pObj = a_editContext.pGameObject;
		if (!_pObj) return;

		// オブジェクト側がシングルトンを触らずに済むよう、
		// マネージャーが配っているものと同じ実行コンテキストを渡す
		auto* _pObjManager = Engine::Scene::SceneManager::Instance().RefGameObjectManager();
		if (!_pObjManager) return;

		// シーン切り替えを跨いだ選択が残っていないか、実体を触る前に確かめる
		if (!_pObjManager->IsManaged(_pObj))
		{
			a_editContext.pGameObject = nullptr;
			return;
		}

		// ギズモ描画先とレクトを、シーンビュー画像に合わせる(エンティティ用と同じ設定)
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(a_pos.x, a_pos.y, a_rect.x, a_rect.y);

		// カメラ行列を取得
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;
		const auto& _camData = _pGE->GetCPUCameraData();

		// オブジェクトへ渡すコンテキストを組む。
		// 具体的な編集方法(3Dギズモ/スクリーン上のハンドル等)は各オブジェクトのDrawGizmoに委ねる。
		GameObject::ObjectGizmoContext _ctx = {};
		_ctx.viewMat = _camData.viewMat;
		_ctx.projMat = _camData.projMat;
		_ctx.viewportPos = { a_pos.x, a_pos.y };
		_ctx.viewportSize = { a_rect.x, a_rect.y };

		_pObj->DrawGizmo(_ctx, _pObjManager->RefObjectContext());
	}
	void SceneViewPanel::SceneFileMenu(EditorContext& a_editContext)
	{
		//------------------------------------------------------------------
		// 開いているシーンを実物から取り直す
		//
		// シーンはエディターからだけでなくゲーム側からも切り替わる
		// (タイトル→ホーム、ホーム→出撃先など)。以前はここで一度覚えたきり
		// だったので、ゲームが切り替えた後もエディターは前のシーンを指したまま
		// になり、Ctrl+S で「別のシーンのファイル」へ上書きしてしまっていた。
		//
		// 実際に積まれているシーンが自分のGUIDを持っているので、毎フレーム
		// そこから取り直す。これで表示も保存先も必ず今のシーンを指す。
		//------------------------------------------------------------------
		auto* _pScene = Engine::Scene::SceneManager::Instance().GetCurrentTopScene();
		if (!_pScene) return;

		if (!(m_currentSceneGUID == _pScene->GetGUID()))
		{
			m_currentSceneGUID = _pScene->GetGUID();

			// 保存先のファイルがあるシーンだけ上書きさせる
			m_canOverwrite = m_currentSceneGUID.IsValid();

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
				if (ImGui::MenuItem("Create new scene..."))
				{
					m_openCreatePopup = true;
					m_sceneNameInput = "";
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

			//--------------------------------------------------------------
			// デバッグプレイの入り切り
			//
			// エディターを出したままゲームを動かすモード。
			// インスペクターで値の動きを見ながら操作を確かめるためのもので、
			// 映るのはゲームのカメラ、入力もゲームへ渡る。
			//
			// 入っている間はマウスがゲーム側の持ち物になる(カーソルは画面中央へ
			// 固定され、エディターはマウスを受け取らない)ので、
			// 抜けるのは Ctrl+P。ボタンの文字にも出しておく。
			//
			// 色は付けない。このエディターでは色付きのボタンは
			// 「押すと増える(緑)/減る(赤)」を表す決まりなので、
			// 表示の切り替えに使うと意味が食い違う
			//--------------------------------------------------------------
			{
				auto& _engine = MainEngine::Instance();
				const bool _isDebugPlay = (_engine.GetMode() == EAppMode::DebugPlay);

				if (ImGui::Button(_isDebugPlay
					? "Debug Play : ON   (Ctrl+P to Editor)"
					: "Debug Play : OFF"))
				{
					_engine.ChangeMode(_isDebugPlay ? EAppMode::Editor : EAppMode::DebugPlay);
				}

				if (!_isDebugPlay && ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(
						"エディターを出したままシーンビューで遊ぶ\n"
						"入るとマウスはゲーム側へ渡るので、抜けるときは Ctrl+P");
				}
			}

			ImGui::EndMenuBar();
		}

		// ポップアップ処理
		LoadScenePopup();
		SaveScenePopup();
		CreateScenePopup();

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

			// 数が増えると探せなくなるので名前で絞り込めるようにする
			const std::string& _search = EditorHelper::DrawSearchBox();

			// 同名のシーンが別フォルダにあり得るので、置き場所を添える対象を先に拾う
			const auto _duplicatedSet = EditorHelper::CollectDuplicatedNames(
				_sceneMetaVec,
				[](const Resource::AssetProperty& a_prop) { return a_prop.fileName; });

			for (size_t _i = 0; _i < _sceneMetaVec.size(); ++_i)
			{
				const auto& _sceneMeta = _sceneMetaVec[_i];

				// 同名が並ぶときはフォルダ名で絞り込めたほうが早いので、パスも検索対象にする
				if (!EditorHelper::IsMatchSearch(_search, _sceneMeta.fileName) &&
					!EditorHelper::IsMatchSearch(_search, _sceneMeta.filePath)) continue;

				// 名前が同じでもImGuiのIDがぶつからないようにする
				// (Selectable のIDはラベル文字列から作られるため)
				ImGui::PushID(static_cast<int>(_i));

				const std::string _label = EditorHelper::MakeUniqueLabel(
					_duplicatedSet, _sceneMeta.fileName,
					Engine::File::GetDirFromPath(_sceneMeta.filePath));

				if (ImGui::Selectable(_label.c_str(), m_currentSceneGUID == _sceneMeta.guid))
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

				ImGui::PopID();
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

					// 名前を付けて保存した先が、このシーンの新しい置き場所になる。
					// シーン本体にも覚えさせておかないと、上のGUID取り直しで
					// 保存前のGUIDへ戻ってしまう
					if (auto* _pCurrentScene = Engine::Scene::SceneManager::Instance().GetCurrentTopScene())
					{
						_pCurrentScene->SetGUID(_guid);
					}

					SaveScene(m_currentSceneGUID);
					ImGui::CloseCurrentPopup(); // 保存後に閉じる
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); } // タイポ修正
			ImGui::EndPopup();
		}
	}
	//======================================================================================
	// 新規シーンの作成
	//--------------------------------------------------------------------------------------
	// 空のシーンをアセットとして書き出してから、そのまま開く。
	//
	// 「名前を付けて保存」は今開いている中身を別名で書き出すものなので、
	// 空から始めたいときには使えない(前のシーンの中身が付いてくる)。
	//======================================================================================
	void SceneViewPanel::CreateScenePopup()
	{
		if (m_openCreatePopup) { ImGui::OpenPopup("Create New Scene"); m_openCreatePopup = false; }
		if (!ImGui::BeginPopupModal("Create New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

		ImGui::Text("Input Filename (.scene) : ");

		const bool _isEnterPressed =
			ImGui::InputText("##newscenename", &m_sceneNameInput, ImGuiInputTextFlags_EnterReturnsTrue);

		ImGui::TextDisabled("Asset/Scenes/<名前>/ へ空のシーンを作って開きます");

		// 名前が無いままは押させない
		ImGui::BeginDisabled(m_sceneNameInput.empty());
		const bool _isCreatePressed = ImGui::Button("Create", ImVec2(120, 0));
		ImGui::EndDisabled();

		if ((_isCreatePressed || _isEnterPressed) && !m_sceneNameInput.empty())
		{
			const Engine::GUID _guid =
				Engine::Scene::SceneManager::Instance().CreateEmptyScene("", m_sceneNameInput);

			if (_guid.IsValid())
			{
				// 作った先をこのまま開く。
				// 中身は空なので、上書き保存の行き先もここになる
				Engine::Scene::SceneManager::Instance().SetNextScene(_guid, Scene::SceneChangeType::Replace);

				m_currentSceneGUID = _guid;
				m_canOverwrite = true;

				ImGui::CloseCurrentPopup();
			}
			// 失敗(同名がある等)のときは閉じない。理由はログへ出ている
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }

		ImGui::EndPopup();
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

		auto _fileDir = Engine::File::GetDirFromPath(_path);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(_path);

		// どのシーンを保存するかをログ出力する
		ENGINE_LOG("[Scene] セーブ : %s", _path.c_str());

		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "scene");
		_pScene->Archive(_ar);
	}
	void SceneViewPanel::CopyEntities(EditorContext& a_editContext, Engine::ECS::World* a_pWorld)
	{
		// コピー : LCtr + C で選択中のエンティティをコピー
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C))
		{
			CopyToBuffer(a_editContext, a_pWorld);
		}

		// ペースト
		// バッファは消さないので、続けて何度でも貼り付けられる
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V))
		{
			PasteFromBuffer(a_pWorld);
		}
	}

	void SceneViewPanel::CopyToBuffer(EditorContext& a_editContext, Engine::ECS::World* a_pWorld)
	{
		m_copyBufferVec.clear();
		if (!a_pWorld) return;

		// 選択中のエンティティとその子孫が対象
		const std::vector<ECS::Entity> _targets = CollectCopyTargets(a_pWorld, a_editContext);
		if (_targets.empty()) return;

		m_copyBufferVec.reserve(_targets.size());

		for (const ECS::Entity& _entity : _targets)
		{
			EntityCopyData _copyData = {};
			_copyData.sig = a_pWorld->GetSignature(_entity);

			// 全コンポーネントのバイト列をそのまま控える。
			// コンポーネントは登録時に trivially copyable を強制しているので memcpy で複製できる。
			for (size_t _typeID = 0; _typeID < _copyData.sig.size(); ++_typeID)
			{
				if (!_copyData.sig.test(_typeID)) continue;

				const auto _compTypeID = static_cast<ECS::ComponentTypeID>(_typeID);

				const uint8_t* _pSrc = a_pWorld->NRefData(_entity, _compTypeID);
				if (!_pSrc) continue;

				const size_t _size = a_pWorld->GetComponentMetaData(_compTypeID).compSize;
				if (_size == 0) continue;

				auto& _buffer = _copyData.dataMap[_compTypeID];
				_buffer.resize(_size);
				std::memcpy(_buffer.data(), _pSrc, _size);
			}

			// 貼り付け時に親子関係を貼り直すため、コピー元のGUIDを覚えておく
			if (a_pWorld->HasComponent<GUIDComponent>(_entity))
			{
				if (const auto* _pGUIDComp = a_pWorld->RefData<GUIDComponent>(_entity))
				{
					_copyData.srcGUID = _pGUIDComp->guid;
				}
			}

			m_copyBufferVec.push_back(std::move(_copyData));
		}

		ENGINE_LOG("[Scene] エンティティをコピーしました : %d 体", static_cast<int>(m_copyBufferVec.size()));
	}

	void SceneViewPanel::PasteFromBuffer(Engine::ECS::World* a_pWorld)
	{
		if (!a_pWorld) return;
		if (m_copyBufferVec.empty()) return;

		// 貼り付けるたびに新しいGUIDを配る。
		// 親子関係は parentGUID で繋がっているので、
		// 「コピー元のGUID → 今回作るGUID」の対応表を先に作ってから付け替える必要がある。
		std::unordered_map<Engine::GUID, Engine::GUID> _guidMap = {};
		for (const EntityCopyData& _copyData : m_copyBufferVec)
		{
			if (!_copyData.srcGUID.IsValid()) continue;

			Engine::GUID _newGUID = {};
			_newGUID.Create();
			_guidMap.emplace(_copyData.srcGUID, _newGUID);
		}

		const auto _guidTypeID			= a_pWorld->GetCompTypeID<GUIDComponent>();
		const auto _hierarchyTypeID		= a_pWorld->GetCompTypeID<HierarchyComponent>();
		const auto _localTransformTypeID = a_pWorld->GetCompTypeID<LocalTransformComponent>();

		const auto _postDeserializeTypeID	= a_pWorld->GetCompTypeID<PostDeserializeTag>();
		const auto _awakeTypeID				= a_pWorld->GetCompTypeID<AwakeTag>();
		const auto _startTypeID				= a_pWorld->GetCompTypeID<StartTag>();
		const auto _activeTypeID			= a_pWorld->GetCompTypeID<ActiveTag>();

		for (const EntityCopyData& _copyData : m_copyBufferVec)
		{
			// バイト列はこの後書き換えるので、バッファ本体ではなくコピーを触る
			// (バッファを残しておくことで何度でも貼り付けられる)
			ECS::Signature _sig = _copyData.sig;
			auto _dataMap = _copyData.dataMap;

			// GUIDは一意でなければならないので、配り直したものへ差し替える
			auto _guidIt = _dataMap.find(_guidTypeID);
			if (_guidIt != _dataMap.end() && _guidIt->second.size() >= sizeof(GUIDComponent))
			{
				auto* _pGUIDComp = reinterpret_cast<GUIDComponent*>(_guidIt->second.data());

				auto _found = _guidMap.find(_pGUIDComp->guid);
				if (_found != _guidMap.end()) _pGUIDComp->guid = _found->second;
			}

			// 親子関係の貼り直し
			auto _hierarchyIt = _dataMap.find(_hierarchyTypeID);
			if (_hierarchyIt != _dataMap.end() && _hierarchyIt->second.size() >= sizeof(HierarchyComponent))
			{
				auto* _pHierarchyComp = reinterpret_cast<HierarchyComponent*>(_hierarchyIt->second.data());

				// 親も一緒にコピーされているなら、新しく作られる親の方へ繋ぎ替える。
				// コピー範囲の外にいる親はそのままなので、コピー元の兄弟として生成される。
				auto _found = _guidMap.find(_pHierarchyComp->parentGUID);
				if (_found != _guidMap.end()) _pHierarchyComp->parentGUID = _found->second;

				// ランタイム用のIDは PostDeserialize 以降で貼り直されるので、ここでは無効化しておく
				_pHierarchyComp->parentID = ECS::Limits::INVALID_ENTITY;
			}

			// 親が変わっている可能性があるので、ワールド行列は必ず作り直させる
			auto _transformIt = _dataMap.find(_localTransformTypeID);
			if (_transformIt != _dataMap.end() && _transformIt->second.size() >= sizeof(LocalTransformComponent))
			{
				reinterpret_cast<LocalTransformComponent*>(_transformIt->second.data())->isDirty = true;
			}

			// 生成直後に初期化フェーズを通す。
			// PostDeserialize → Awake → Start → Active と遷移する間に
			// GUID解決・親子リンク・リソース確保などが走るので、
			// コピー元が持っていたフェーズタグは落としておく。
			_sig.set(_postDeserializeTypeID);
			_sig.reset(_awakeTypeID);
			_sig.reset(_startTypeID);
			_sig.reset(_activeTypeID);

			a_pWorld->AddEntityWithData(_sig, std::move(_dataMap));
		}

		ENGINE_LOG("[Scene] エンティティを貼り付けました : %d 体", static_cast<int>(m_copyBufferVec.size()));
	}

	std::vector<Engine::ECS::Entity> SceneViewPanel::CollectCopyTargets(Engine::ECS::World* a_pWorld, const EditorContext& a_editContext)
	{
		std::vector<ECS::Entity> _targets = {};
		if (!a_pWorld) return _targets;

		// 「親 → 子リスト」の対応表を一度だけ作る。
		// 子をたどるたびに全エンティティを舐め直さないため。
		std::unordered_map<ECS::Entity, std::vector<ECS::Entity>> _childMap = {};
		a_pWorld->ForEach<HierarchyComponent>(
			[&_childMap]
			(
				ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				HierarchyComponent* a_hierarchyArray
				)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					const ECS::Entity _parent = a_hierarchyArray[_i].parentID;
					if (_parent == ECS::Limits::INVALID_ENTITY) continue;

					_childMap[_parent].push_back(a_pChunk->entityData[_i]);
				}
			}
		);

		// 選択中のエンティティを起点に子孫をたどって集める
		std::vector<ECS::Entity> _stack = {};
		for (const ECS::Entity& _entity : a_editContext.selectedEntities)
		{
			if (_entity == ECS::Limits::INVALID_ENTITY) continue;
			_stack.push_back(_entity);
		}

		std::unordered_set<ECS::Entity> _visited = {};
		while (!_stack.empty())
		{
			const ECS::Entity _entity = _stack.back();
			_stack.pop_back();

			// 親と子を同時に選んでいた場合などの重複はここで弾かれる。
			// 循環参照があっても訪問済みで止まるので無限ループにはならない。
			if (!_visited.insert(_entity).second) continue;

			_targets.push_back(_entity);

			auto _it = _childMap.find(_entity);
			if (_it == _childMap.end()) continue;

			for (const ECS::Entity& _child : _it->second)
			{
				_stack.push_back(_child);
			}
		}

		return _targets;
	}
}