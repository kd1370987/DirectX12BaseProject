#pragma once

#include "../../Components/Camera/ProjMatComponent.h"

struct CameraParamComponent
{
	float fovY			= 60.0f;        // 垂直視野角(単位: 度)
	float aspectRatio	= 16.0f / 9.0f; // アスペクト比
	float nearZ			= 0.1f;			// ニアクリップ距離
	float farZ			= 1000.0f;	    // ファークリップ距離

	// 実行時の画角の上乗せ(度)。実際の画角は fovY + fovBoost。
	// スピードに応じた広角化(TPSSystem)のように毎フレーム動かす値をここに入れる。
	// fovY を直接書き換えると「作者が決めた基準の画角」が失われるので分けてある。
	// 派生値なので保存しない。
	float fovBoost		= 0.0f;

	// 画角などが変わったので射影行列を作り直す必要がある
	bool isDirty = false;

	// 実効画角(度)。0 や 180 以上は射影行列が破綻するので必ず内側へ丸める
	float GetFovY() const { return std::clamp(fovY + fovBoost, 1.0f, 179.0f); }
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

		// プレハブのインスペクタは実体を持たない(entity が無効値)。
		// RefData は生存エンティティ前提で添え字を引くので、先に弾く
		if (!a_context.pWorld || a_context.entity == Engine::ECS::Limits::INVALID_ENTITY) return;

		auto* _pProjMatComp = a_context.pWorld->RefData<ProjMatComponent>(a_context.entity);
		if (!_pProjMatComp) return;

		if (_isEdit)
		{
			_pProjMatComp->projMat = Math::Matrix::CreatePerspectiveFieldOfView(
				DirectX::XMConvertToRadians(_comp.GetFovY()),
				_comp.aspectRatio,
				_comp.nearZ,
				_comp.farZ
			);
		};
	}
};