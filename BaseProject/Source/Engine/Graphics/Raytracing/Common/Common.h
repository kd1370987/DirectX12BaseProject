#pragma once

namespace Engine::Raytracing
{
	// レイトレワールドに載せられるインスタンス数の上限。
	// TLAS・インスタンス/マテリアルのバッファ・シェーダーテーブルの大きさはすべてこれで決まる。
	// どれか1つだけ違う数にすると、多いほうのデータが少ないほうのバッファをはみ出すので、
	// 数を直接書かず必ずこれを使うこと
	inline constexpr UINT kMaxInstanceNum = 1000;

	// レイ用シェーダーのカテゴリ
	enum ShaderCategory
	{
		RayGenerator,		// レイを生成するシェーダー
		Miss,				// レイが当たらなかったときに走るシェーダー
		ClosestHit,			// もっとも近いポリゴンとレイが交差したときに呼ばれるシェーダー
		AnyHit
	};

	//ローカルルートシグネチャ
	enum LocalRootSignature
	{
		Empty,				//空のローカルルートシグネチャ。
		RayGen,				//レイ生成シェーダー用のローカルルートシグネチャ。
		PBRMaterialHit,		//PBRマテリアルにヒットしたときのローカルルートシグネチャ。
	};

	// レイ用シェーダーのデータ
	struct RayShaderData
	{
		const wchar_t* entryName;		// エントリーポイント名
		LocalRootSignature rootsigType;	// ローカルルートシグネチャの種類
		ShaderCategory category;		// シェーダーのカテゴリ
	};

	// ヒットグループ
	struct HitGroup
	{
		const wchar_t* name;			// ヒットグループの名前
		const wchar_t* closestHit;		// 最も近いポリゴンにヒットしたときに呼ばれるシェーダー
		const wchar_t* anyHitShader;	// それ以外
	};
}