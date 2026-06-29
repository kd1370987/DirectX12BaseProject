#pragma once

#include "../../../Engine/MainEngine.h"
#include "../../../Engine/Graphics/GraphicEngine.h"


struct LocalTransformComponent
{
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 quat = { 0.0f, 0.0f, 0.0f,1.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	mutable bool isDirty = true;
};

template<>
struct Engine::ECS::ComponentTraits<LocalTransformComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		LocalTransformComponent& _comp = Engine::Editor::GetValue<LocalTransformComponent>(a_pData);
		a_ar.Field("pos",_comp.pos);
		a_ar.Field("quat",_comp.quat);
		a_ar.Field("scale",_comp.scale);

		_comp.isDirty = true;
	}
	static void Edit(void* a_pData) 
	{
		LocalTransformComponent& _comp = Engine::Editor::GetValue<LocalTransformComponent>(a_pData);
		bool _isEdit = false;
		_isEdit |= ImGui::DragFloat3("Position", &_comp.pos.x);
		_isEdit |= ImGui::DragFloat4("Quaternion", &_comp.quat.x);
		_isEdit |= ImGui::DragFloat3("Scale", &_comp.scale.x);
		_comp.isDirty |= _isEdit;

		// 現在のカメラ行列を取得
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;
		const auto& _camData = _pGE->GetCPUCameraData();

		// Pos/Quat/Scale から 4x4ワールド行列を合成
		DirectX::XMVECTOR _vScale = DirectX::XMLoadFloat3(&_comp.scale);
		DirectX::XMVECTOR _vQuat = DirectX::XMLoadFloat4(&_comp.quat);
		DirectX::XMVECTOR _vPos = DirectX::XMLoadFloat3(&_comp.pos);

		// アフィン変換行列を作成
		DirectX::XMMATRIX _mWorld = DirectX::XMMatrixAffineTransformation(_vScale, DirectX::XMVectorZero(), _vQuat, _vPos);

		DirectX::XMFLOAT4X4 worldFloat4x4;
		DirectX::XMStoreFloat4x4(&worldFloat4x4, _mWorld);

		// マニピュレーターの操作（戻り値で行列が更新されたか判定可能）
		bool isUsing = ImGuizmo::Manipulate(
			&_camData.viewMat._11,
			&_camData.projMat._11,
			ImGuizmo::OPERATION::TRANSLATE, // 移動モード
			ImGuizmo::MODE::WORLD,          // ワールド座標系
			&worldFloat4x4._11              // 操作したい行列（結果もここに入る）
		);

		// 行列が更新されたら、ECSのトランスフォームを同期する
		if (isUsing) {
			DirectX::XMMATRIX updatedWorld = DirectX::XMLoadFloat4x4(&worldFloat4x4);

			DirectX::XMVECTOR outScale, outRotQuat, outTrans;

			// 行列を分解
			DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, updatedWorld);

			DirectX::XMStoreFloat3(&_comp.pos, outTrans);
			DirectX::XMStoreFloat4(&_comp.quat, outRotQuat);
			DirectX::XMStoreFloat3(&_comp.scale, outScale);

			_comp.isDirty = true;
		}
	}
};