#include "CameraProjUpdateSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Camera/CameraParamComponent.h"
#include "../../../../Components/Camera/ProjMatComponent.h"

//==========================================================================================
// CameraProjUpdateSystem
//
// 画角(fovY + fovBoost)が動いたカメラの射影行列を作り直す。
//
// ・射影行列は CameraStartSystem が起動時に一度だけ作る作りだったので、
//   実行中に画角を動かしても絵に反映されなかった。その差分更新だけを担当する。
// ・作り直すのは isDirty が立っているフレームだけ。書く側(TPSSystem など)が
//   「変わった」と判断したときに立てる契約。
// ・CameraParamComponent を書く TPSSystem と同じ Camera 帯。あちらが書き、
//   こちらが読む(RAW)ので実行順は自動で後ろに回る。
//==========================================================================================
void CameraProjUpdateSystem::Init(Engine::ECS::World& a_world)
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
