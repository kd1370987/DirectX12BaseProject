#pragma once
namespace App::InstanceResource
{
	struct HierarchyResource
	{
		int maxDepth = 0;		// ヒエラルキーの最大深度
		bool isDirty = true;	// ツリーが更新されたかどうかのフラグ
	};
}