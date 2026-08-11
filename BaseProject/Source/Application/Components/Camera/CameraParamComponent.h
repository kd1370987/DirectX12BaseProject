#pragma once

#include "../../Components/Camera/ProjMatComponent.h"

struct CameraParamComponent
{
	float fovY			= 60.0f;        // 垂直視野角(単位: 度)
	float aspectRatio	= 16.0f / 9.0f; // アスペクト比
	float nearZ			= 0.1f;			// ニアクリップ距離
	float farZ			= 1000.0f;	    // ファークリップ距離

	bool isDirty = false;
};

template<>
struct Engine::ECS::ComponentTraits<CameraParamComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		CameraParamComponent& _comp = Engine::Editor::GetValue<CameraParamComponent>(a_pData);
		a_ar.Field("fovY",_comp.fovY);
		a_ar.Field("aspectRatio",_comp.aspectRatio);
		a_ar.Field("nearZ",_comp.nearZ);
		a_ar.Field("farZ",_comp.farZ);
	}

	static void Edit(CompEditContext& a_context)
	{
		CameraParamComponent& _comp = Engine::Editor::GetValue<CameraParamComponent>(a_context.pData);

		bool _isEdit = false;

		_isEdit |= ImGui::DragFloat("Fov", &_comp.fovY);
		_isEdit |= ImGui::DragFloat("Aspect", &_comp.aspectRatio);
		_isEdit |= ImGui::DragFloat("NearZ", &_comp.nearZ, 0.01f, 0.1f);
		_isEdit |= ImGui::DragFloat("FarZ", &_comp.farZ,1.0f,1.0f);

		_comp.nearZ = std::max(0.1f, _comp.nearZ);
		_comp.farZ = std::max(1.0f,_comp.farZ);

		auto* _pProjMatComp = a_context.pWorld->RefData<ProjMatComponent>(a_context.entity);
		if (!_pProjMatComp) return;

		if (_isEdit)
		{
			DirectX::XMMATRIX _lhMat = DirectX::XMMatrixPerspectiveFovLH(
				DirectX::XMConvertToRadians(_comp.fovY),
				_comp.aspectRatio,
				_comp.nearZ,
				_comp.farZ
			);
			DirectX::XMStoreFloat4x4(&_pProjMatComp->projMat, _lhMat);
		};
	}
};