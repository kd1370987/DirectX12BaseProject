#include "SceneView.h"

#include "EditorCamera/EditorCamera.h"

#include "../../MainEngine.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../Graphics/GraphicEngine.h"
#include "../../Graphics/RenderGraph/RenderGraph.h"

#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../Application/Components/Transform/LocalTransformComponent.h"
#include "../../../Application/Components/Transform/WorldMatrixComponent.h"

#include "../../ECS/World/World.h"

namespace Engine::Editor
{
	void Engine::Editor::SceneView::Init()
	{
		m_upEditorCamera = std::make_unique<EditorCamera>();
		m_upEditorCamera->Init(1920,1080);
	}
	void SceneView::Update(float a_dt)
	{
		m_upEditorCamera->Update(a_dt);
	}
	const EditorCamera* SceneView::GetEditorCamera()
	{
		if (!m_upEditorCamera) return nullptr;

		return m_upEditorCamera.get();
	}
	void SceneView::Draw(const ECS::Entity& a_currentSelectEntity, Engine::ECS::World* a_pWorld)
	{
		if (ImGui::Begin("SceneView"))
		{
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
			auto _texHandle = _pRG->GetTexHandle("AffterTAAColor");
			const auto* _pTex = Resource::ResourceManager::Instance().Get(_texHandle);
			if(!_pTex)
			{
				ImGui::End();
				return;
			}

			// テクスチャの描画 : 実際に描画した範囲も取得
			auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
			ImVec2 _actualRenderSize = Helper::DrawSRVView(_gpuHandle, static_cast<UINT>(1980), static_cast<UINT>(1080));

			// ギズモ描画
			GuizmoDraw(_windowPos,_actualRenderSize,a_currentSelectEntity,a_pWorld);
		}
		ImGui::End();
	}
	void SceneView::GuizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect, const ECS::Entity& a_currentSelectEntity, Engine::ECS::World* a_pWorld)
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
}
