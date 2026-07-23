#include "CameraStartSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Camera/ProjMatComponent.h"
#include "../../../../Components/Camera/CameraParamComponent.h"

#include "../../../../Components/Tag/SystemPhaseTag/StartTag.h"

#include "../../../../../Engine/MainEngine.h"
#include "../../../../../Engine/Graphics/RenderContext/RenderContext.h"

#include "../../../../../Engine/Option/OptionManager.h"

void CameraStartSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<CameraParamComponent,ProjMatComponent>(
		Engine::ECS::ESystemType::Start,
		"CameraStartSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
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
				const auto& _winOp = Engine::Option::OptionManager::GetInstance().GetWindowOption();
				_camParamComp.aspectRatio = (float)_winOp.windowWidth / (float)_winOp.windowHegiht;

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
