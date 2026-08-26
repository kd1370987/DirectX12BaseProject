#include "CameraProjUpdateSystem.h"

#include "Application/ECS/World/World.h"

#include "../../../../Components/Camera/CameraParamComponent.h"
#include "../../../../Components/Camera/ProjMatComponent.h"

//==========================================================================================
// CameraProjUpdateSystem
//
// 画角(fovY + fovBoost)が動いたカメラの射影行列を作り直す。
//==========================================================================================
void CameraProjUpdateSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveTask<CameraParamComponent, ProjMatComponent>(
		Engine::ECS::ESystemType::Camera,
		"CameraProjUpdateSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			CameraParamComponent* a_camParamArray,
			ProjMatComponent* a_projMatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				CameraParamComponent& _camParamComp = a_camParamArray[_i];
				ProjMatComponent&     _projMatComp  = a_projMatArray[_i];

				if (!_camParamComp.isDirty) continue;
				_camParamComp.isDirty = false;

				_projMatComp.projMat = Math::Matrix::CreatePerspectiveFieldOfView(
					DirectX::XMConvertToRadians(_camParamComp.GetFovY()),
					_camParamComp.aspectRatio,
					_camParamComp.nearZ,
					_camParamComp.farZ
				);
			}
		}
	);
}
