#pragma once

namespace Engine::Raytracing
{
	// ヒープ設定を用意
	const D3D12_HEAP_PROPERTIES cUploadHeapProps = {	// アップロード
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};
	const D3D12_HEAP_PROPERTIES cDefaultHeapProps = {	// デフォルト
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	// レイトレースの再帰呼び出しの最大
	const int MAX_TRACE_RECURSION_DEPTH = 2;	// TraceRayを呼び出せる最大数

	// レイ用シェーダーのカテゴリ
	// ヒットグループ
	enum EHitGroup
	{
		Undef = -1,

		PBRCameraRay,	// PBRマテリアルにカメラレイが衝突するときのヒットグループ
		PBRShadowRay,	// PBRマテリアルにシャドウレイが衝突するときのヒットグループ

		HitGroupNum				// ヒットグループ数
	};
	// ローカルルートシグネチャ
	enum class ELocalRootSignature
	{
		Empty,				// 空
		RayGen,				// レイ生成シェーダー用
		PBRMaterialHit		// PBRマテリアルにヒットしたとき
	};

	// シェーダーの種類
	enum EShader
	{
		RayGeneration,		// カメラレイを生成するシェーダー
		Miss,				// カメラレイがどこにもぶつからなかったときに呼ばれるシェーダー
		PBRChs,				// もっとも近いポリゴンとカメラレイが交差したときに呼ばれるシェーダー
		ShadowChs,			// もっとも近いポリゴンとシャドウレイが交差したときに呼ばれるシェーダー
		ShadowMiss,			// シャドウレイがどこにもぶつからなかったときに呼ばれるシェーダー
		ShaderNum					// シェーダーの総数
	};

	// シェーダーのカテゴリ
	enum class EShaderCategory
	{
		RayGenerator,		// レイを生成するシェーダー
		Miss,				// レイが当たらなかったときに走るシェーダー
		ClosestHit			// もっとっも近いポリゴンとレイが交差したときに呼ばれるシェーダー
	};

	// レイ用シェーダー
	struct ShaderData
	{
		const wchar_t* entryName;			// エントリーポイント名
		ELocalRootSignature rootsigType;	// 使用するローカルルートシグネチャ
		EShaderCategory category;			// シェーダーのカテゴリ
		EHitGroup hitGroup;					// ヒットグループ(カテゴリが Miss, RayGen の場合無視される値)
	};
	const ShaderData cShaderDatas[] = {
		// エントリー名　		使用ローカルルートシグネチャ　		カテゴリー　					ヒットグループ
		{L"RayGen",				ELocalRootSignature::RayGen,		EShaderCategory::RayGenerator,	EHitGroup::Undef},
		{L"Miss",				ELocalRootSignature::Empty,			EShaderCategory::Miss,			EHitGroup::Undef},
		{L"Chs",				ELocalRootSignature::PBRMaterialHit,EShaderCategory::ClosestHit,	EHitGroup::PBRCameraRay},
		{L"ShadowChs",			ELocalRootSignature::PBRMaterialHit,EShaderCategory::ClosestHit,	EHitGroup::PBRShadowRay},
		{L"ShadowMiss",			ELocalRootSignature::Empty,			EShaderCategory::Miss,			EHitGroup::Undef}
	};
	static_assert(ARRAYSIZE(cShaderDatas) == EShader::ShaderNum, "シェーダーデータの数とシェーダー数が会いません");

	// ヒットグループ
	struct HitGroup
	{
		const wchar_t* name;				// ヒットグループの名前
		const wchar_t* chsHitShaderName;	// 最も近いポリゴンにヒットしたときに呼ばれるシェーダー
		const wchar_t* anyHitShaderName;	// 透明物などの処理用（不透明ならnullptr）
	};
	const HitGroup cHitGroups[] = {
		{L"HitGroup",		cShaderDatas[EShader::PBRChs].entryName,	nullptr},
		{L"ShaderHitGroup",	cShaderDatas[EShader::ShadowChs].entryName,	nullptr}
	};

	// シェーダーテーブルに登録されているSRVの1要素
	// この列挙子の並びがtレジスタ番号になる。
	// シェーダーテーブルには各インスタンスごとにシェーダーリソースのディスクリプタが登録されている
}