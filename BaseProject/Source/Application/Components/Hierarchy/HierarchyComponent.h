#pragma once

// 復元時はGUIDからの復元

struct HierarchyComponent
{
	// 親
	Engine::ECS::Entity parentID = Engine::ECS::Limits::INVALID_ENTITY;		// ランタイム用

	// 先頭のエンティティ
	Engine::ECS::Entity firstChildID = Engine::ECS::Limits::INVALID_ENTITY;

	// 自分より前のエンティティ
	Engine::ECS::Entity prevSiblingID = Engine::ECS::Limits::INVALID_ENTITY;

	// 自分の次に来るエンティティ
	Engine::ECS::Entity nextSiblingID = Engine::ECS::Limits::INVALID_ENTITY;

	// 先頭から自分が何階層目なのか
	// ソート時につかう
	UINT depth = 0;
};