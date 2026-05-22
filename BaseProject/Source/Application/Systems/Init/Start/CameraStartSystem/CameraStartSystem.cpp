#include "CameraStartSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Camera/ProjMatComponent.h"
#include "../../../../Components/Camera/CameraParamComponent.h"

#include "../../../../Components/Tag/SystemPhaseTag/StartTag.h"

void CameraStartSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<CameraParamComponent,ProjMatComponent>(
		Engine::ECS::ESystemType::Start,
		"CameraStartSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			StartTag* a_startTag,
			CameraParamComponent* a_camParamArray,
			ProjMatComponent* a_projMatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				CameraParamComponent& _camParamComp = a_camParamArray[_i];
				ProjMatComponent& _projMatComp = a_projMatArray[_i];

				// カメラパラメーターの初期化
				_camParamComp.aspectRatio = (float)1280 / (float)720;
				_camParamComp.fovY = 60.0f;
				_camParamComp.nearZ = 0.1f;
				_camParamComp.farZ = 1000.0f;

				// プロジェクション行列の作成
				DirectX::XMMATRIX _lhMat = DirectX::XMMatrixPerspectiveFovLH(
					DirectX::XMConvertToRadians(_camParamComp.fovY),
					_camParamComp.aspectRatio,
					_camParamComp.nearZ,
					_camParamComp.farZ
				);
				DirectX::XMStoreFloat4x4(&_projMatComp.projMat, _lhMat);
			}
		}
	);
}
