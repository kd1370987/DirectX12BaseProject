#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

//==========================================================================================
// AttachmentSlotLinkSystem
//
// AttachmentSlotsComponent の各スロットが保存している GUID から、
// ランタイムの Entity(id) を解決する。
// (HierarchyComponent の parentGUID -> parentID 解決と同じ流儀)
//==========================================================================================
class AttachmentSlotLinkSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
