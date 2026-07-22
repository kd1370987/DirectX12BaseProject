#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

//==========================================================================================
// AttachmentSlotLinkSystem
//
// AttachmentSlotsComponent の各スロットが保存している GUID から、
// ランタイムの Entity(id) を解決する。
// (ExoskeletonAttachmentComponent の parentGUID -> parentID 解決と同じ流儀)
//==========================================================================================
class AttachmentSlotLinkSystem : public Engine::ECS::SystemBase<AttachmentSlotLinkSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostDeserialize;
	void Init(Engine::ECS::World& a_world) override;
};
