#pragma once
namespace Engine::Graphics
{
	// カメラ
	struct alignas(256) CameraData
	{
		// 現在フレームのデータ
		// ジッターありデータ
		DirectX::XMFLOAT4X4 viewMat = {};			// ビュー行列
		DirectX::XMFLOAT4X4 projMat = {};			// 射影行列
		DirectX::XMFLOAT4X4 viewInvMat = {};		// ビュー行列
		DirectX::XMFLOAT4X4 projInvMat = {};		// 射影逆行列
		DirectX::XMFLOAT4X4 viewProjMat = {};
		DirectX::XMFLOAT4X4 invViewProjMat = {};

		// モーションベクター用
		DirectX::XMFLOAT4X4 nonJitteredProj;		// ジッターなし投影行列
		DirectX::XMFLOAT4X4 nonJitteredViewProj;	// ジッターなしビュープロジェクション行列

		// 1フレーム前のデータ
		DirectX::XMFLOAT4X4 prevView;
		DirectX::XMFLOAT4X4 prevProj;
		DirectX::XMFLOAT4X4 prevViewProj;


		DirectX::XMFLOAT4 pos = { 0.0f,0.0f,0.0f,0.0f };	// カメラのワールド座標
		DirectX::XMFLOAT2 jitterOffset = {};
		DirectX::XMFLOAT2 prevJitterOffset = {};

	};

	// バッファインデックス
	struct alignas(256) BufferIndexData
	{
		UINT instanceIndex = 0;
		UINT subsetIndex = 0;

		float pad0;
		float pad1;
	};

	// 環境データ
	struct alignas(256) AmbientData
	{
		// 環境光
		DirectX::XMFLOAT3 ammbientColorScale = {0,0,0};
		float pad0;
		// ディレクショナルライト
		DirectX::XMFLOAT3 dlDir = {0,0,0};
		float pad1;
		DirectX::XMFLOAT3 dlColor = {0,0,0};
		float pad2;
	};

	// インスタンスデータ
	struct InstanceData
	{
		// ワールド座標
		DirectX::XMFLOAT4X4 worldMat;		// 現在フレーム
		DirectX::XMFLOAT4X4 prevWorldMat;	// １フレーム前

		// ボーン情報
		int boneStartIndex = 0;
		int boneCount = 0;

		DirectX::XMFLOAT2 pad;
	};

	// サブメッシュ単位データ
	struct SubSetData
	{
		// テクスチャスケール
		DirectX::XMFLOAT4 baseColorScale = {};
		DirectX::XMFLOAT3 emissiveColorScale = {};
		float metallic = 0.0f;
		float roughness = 0.0f;

		DirectX::XMFLOAT3 pad;
	};

	// ボーンデータ
	struct BonePallete
	{
		DirectX::XMFLOAT4X4 mat;
	};
}