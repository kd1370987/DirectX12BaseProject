#include "ScreenUIDrawSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"

#include "Application/Components/Resource/UIComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

void ScreenUIDrawSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, const UIComponent>(
		Engine::ECS::ESystemType::Draw,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const WorldMatrixComponent* a_worldMatArray,
			const UIComponent* a_uiArray
		) 
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _matComp = a_worldMatArray[_i];
				const UIComponent& _uiComp = a_uiArray[_i];

				// 描画アイテム
				Engine::Graphics::DrawItem2D _item = {};
				_item.worldMat = _matComp.worldMat;
				_item.srvHandleRange = Engine::Resource::TextureManager::Instance().GetTexture(_uiComp.texHandle).GetSRV();
				_item.colorScale = _uiComp.color;

				// 描画キューに追加
				auto* _pRCT = Engine::MainEngine::Instance().RefRenderContext();
				_pRCT->AddItem(RenderQueueType2D::ScreenUI, _item);
			}
		}
	);
}