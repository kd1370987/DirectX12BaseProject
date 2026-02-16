#pragma once

#include "ModelResource/Material/Material.h"
#include "ModelResource/Animation/Animation.h"
#include "ModelResource/Mesh/Mesh.h"
#include "ModelResource/Node/Node.h"

struct Model
{
	// マテリアル
	std::vector<Material>						materials;

	// メッシュの配列
	std::vector<std::shared_ptr<Mesh>> 			spMeshVec;

	// アニメーション
	std::vector<std::shared_ptr<AnimationData>> spAnimations;

	// ノード
	std::vector<Node>							originalNodes;				// 全ノード配列
	std::vector<int>							rootNodeIndices;			// Rootノード
	std::vector<int>							boneNodeIndices;			// ボーンノード
	std::vector<int>							meshNodeIndices;			// メッシュが存在するノード
	std::vector<int>							collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
	std::vector<int>							drawMeshNodeIndices;		// 描画するノード
};

namespace ModelUtility
{
	constexpr uint32_t MAX_ANIMATIONCLIP = 16;

	/// <summary>
	/// 文字列からアニメーションのクリップIDを取得
	/// </summary>
	/// <param name="a_model">モデル</param>
	/// <param name="a_animeNmae">アニメーション文字列</param>
	/// <returns>クリップID</returns>
	uint32_t GetAnimationClipCount(const Model& a_model,const std::string& a_animeNmae);

	/// <summary>
	/// アニメーション取得
	/// </summary>
	/// <param name="a_model">モデル</param>
	/// <param name="a_clipID">クリップID</param>
	/// <returns>シェアードポインターのアニメーションデータ</returns>
	std::shared_ptr<AnimationData> GetSPAnimation(const Model& a_model,uint32_t a_clipID);
}