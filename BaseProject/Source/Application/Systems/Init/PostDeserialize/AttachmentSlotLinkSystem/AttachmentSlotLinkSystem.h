#pragma once

#include "Application/ECS/ISystem/ISystem.h"

//==========================================================================================
// AttachmentSlotLinkSystem
//
// AttachmentSlotsComponent の各スロットが保存している GUID から、
// ランタイムの Entity(id) を解決する。
// (HierarchyComponent の parentGUID -> parentID 解決と同じ流儀)
//==========================================================================================
class AttachmentSlotLinkSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
