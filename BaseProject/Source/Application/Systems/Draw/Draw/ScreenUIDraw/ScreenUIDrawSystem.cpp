#include "ScreenUIDrawSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/UIComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

void ScreenUIDrawSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEach<WorldMatrixComponent, UIComponent>(
		[&a_world, a_dt]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			WorldMatrixComponent* a_matArray,
			UIComponent* a_uiArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				WorldMatrixComponent& _matComp = a_matArray[_i];
				UIComponent& _uiComp = a_uiArray[_i];

				// 描画アイテム
				Engine::Graphics::DrawItem2D _item = {};
				_item.worldMat = _matComp.worldMat;
				_item.srvHandleRange = Engine::Resource::TextureManager::Instance().GetTexture(_uiComp.texHandle).GetSRV();
				_item.colorScale = _uiComp.color;

				Engine::Graphics::RenderContext::Instance().AddItem(RenderQueueType2D::ScreenUI, _item);
			}
		});
}
