#pragma once
namespace App::Editor
{
	class CompEditHelper
	{
	public:
		/// <summary>
		/// 親エンティティのモデルからノードを取得
		/// </summary>
		/// <param name="a_editContext">コンテキスト</param>
		/// <param name="a_nodeNameHash">ノード名のハッシュ値</param>
		/// <param name="a_nodeIndex">ノードのインデックス</param>
		static void SelectParentModelNode(
			Engine::ECS::CompEditContext& a_editContext, 
			UINT& a_nodeNameHash,
			UINT& a_nodeIndex
		);

		/// <summary>
		/// 自身のモデルからノードを取得
		/// </summary>
		/// <param name="a_editContext">コンテキスト</param>
		/// <param name="a_nodeNameHash">ノード名のハッシュ値</param>
		/// <param name="a_nodeIndex">ノードのインデックス</param>
		static void SelectSelfModelNode(
			Engine::ECS::CompEditContext& a_editContext,
			UINT& a_nodeNameHash,
			UINT& a_nodeIndex
		);

		/// <summary>
		/// モデルからノードを取得
		/// </summary>
		/// <param name="a_editContext">コンテキスト</param>
		/// <param name="a_modelHandle">取得先モデルハンドル</param>
		/// <param name="a_nodeNameHash">ノード名のハッシュ値</param>
		/// <param name="a_nodeIndex">ノードのインデックス</param>
		static void SelectModelNode(
			Engine::ECS::CompEditContext& a_editContext,
			const Engine::Handle<Engine::Resource::Model>& a_modelHandle,
			UINT& a_nodeNameHash,
			UINT& a_nodeIndex
		);
	};
}