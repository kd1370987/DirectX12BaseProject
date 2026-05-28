#pragma once
namespace Engine::Graphics
{
	// カメラ
	struct CameraData
	{
		DirectX::XMFLOAT3 pos = { 0.0f,0.0f,0.0f };	// カメラのワールド座標
		float pad;

		// 現在フレームのデータ
		DirectX::XMFLOAT4X4 viewMat = {};			// ビュー行列
		DirectX::XMFLOAT4X4 projMat = {};			// 射影行列
		DirectX::XMFLOAT4X4 projInvMat = {};		// 射影逆行列
		DirectX::XMFLOAT4X4 viewInvMat = {};		// ビュー行列

		// 1フレーム前のデータ
		DirectX::XMFLOAT4X4 prevView;
		DirectX::XMFLOAT4X4 prevProj;
		DirectX::XMFLOAT4X4 prevViewProj;
	};

	// バッファインデックス
	struct BufferIndexData
	{
		UINT instanceIndex = 0;
		UINT subsetIndex = 0;

		float pad0;
		float pad1;
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

		float pad0;
		float pad1;
	};

	// サブメッシュ単位データ
	struct SubSetData
	{
		// テクスチャスケール
		DirectX::XMFLOAT4 baseColorScale = {};
		DirectX::XMFLOAT3 emissiveColorScale = {};
		float metallic = 0.0f;
		float roughness = 0.0f;

		float pad0;
		float pad1;
		float pad2;
	};

	// ボーンデータ
	struct BonePallete
	{
		DirectX::XMFLOAT4X4 mat;
	};
}