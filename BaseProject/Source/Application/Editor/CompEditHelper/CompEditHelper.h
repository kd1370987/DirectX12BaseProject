#pragma once
namespace App::Editor
{
	class CompEditHelper
	{
	public:
		/// <summary>
		/// モデルからノードを取得
		/// </summary>
		/// <param name="a_editContext">コンテキスト</param>
		/// <param name="a_nodeNameHash">ノード名のハッシュ値</param>
		/// <param name="a_nodeIndex">ノードのインデックス</param>
		static void SelectModelNode(
			Engine::ECS::CompEditContext& a_editContext, 
			UINT& a_nodeNameHash,
			UINT& a_nodeIndex
		);
	};
}