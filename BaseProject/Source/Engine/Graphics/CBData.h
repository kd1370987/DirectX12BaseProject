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
		DirectX::XMFLOAT4X4 nonJitteredInvViewProj;	// ジッターなしビュープロジェクション行列

		// 1フレーム前のデータ
		DirectX::XMFLOAT4X4 prevView;
		DirectX::XMFLOAT4X4 prevProj;
		DirectX::XMFLOAT4X4 prevViewProj;


		DirectX::XMFLOAT4 pos = { 0.0f,0.0f,0.0f,0.0f };	// カメラのワールド座標
		DirectX::XMFLOAT2 jitterOffset = {};
		DirectX::XMFLOAT2 prevJitterOffset = {};

		DXSM::Vector4 frustumPlanes[6] = {};

		/// <summary>
		/// フラスタムの平面を求める : すでに構造体内にデータが入っている前提での処理
		/// </summary>
		void ExtractFrustumPlanes()
		{
			auto& _m = nonJitteredViewProj;
			// 各平面の抽出 : x , y , z は法線ベクトル , w は原点からの距離
			// [0] 左平面
			frustumPlanes[0] = DXSM::Vector4(_m._14 + _m._11, _m._24 + _m._21, _m._34 + _m._31, _m._44 + _m._41);
			// [1] 右平面
			frustumPlanes[1] = DXSM::Vector4(_m._14 - _m._11, _m._24 - _m._21, _m._34 - _m._31, _m._44 - _m._41);
			// [2] 下平面
			frustumPlanes[2] = DXSM::Vector4(_m._14 + _m._12, _m._24 + _m._22, _m._34 + _m._32, _m._44 + _m._42);
			// [3] 上平面
			frustumPlanes[3] = DXSM::Vector4(_m._14 - _m._12, _m._24 - _m._22, _m._34 - _m._32, _m._44 - _m._42);
			// [4] 近平面
			frustumPlanes[4] = DXSM::Vector4(_m._13, _m._23, _m._33, _m._43);
			// [5] 遠平面
			frustumPlanes[5] = DXSM::Vector4(_m._14 - _m._13, _m._24 - _m._23, _m._34 - _m._33, _m._44 - _m._43);
			// 平面の平均化
			for (int _i = 0; _i < 6; ++_i)
			{
				DirectX::XMVECTOR _plane = frustumPlanes[_i];
				_plane = DirectX::XMPlaneNormalize(_plane);			// 法線ベクトルの長さを正規化 : Wも合わせてスケール
				frustumPlanes[_i] = _plane;
			}
		}
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

	// デバッグライン用データ
	enum class EShapeType : UINT
	{
		Line,
		Box,
		Capsule,
		Sphere
	};
	struct DebugLineData
	{
		DirectX::XMFLOAT4	color;
		DirectX::XMFLOAT4X4 worldMat;
		UINT shapeType;
	};

	// ---- メッシュシェーダー用構造体 ----
	struct MeshInstanceData
	{
		DXSM::Matrix worldMat;			// 現在フレームのワールド行列

		DXSM::Matrix prevWorldMat;		// １フレーム前のワールド行列

		uint32_t materialOffset;			// メッシュが参照するマテリアル
		uint32_t meshletOffset;				// メッシュレットオフセット
		uint32_t vertexOffset;				// 頂点オフセット
		uint32_t uviOffset;					// ユニーク頂点インデックスオフセット

		uint32_t primitiveOffset;			// プリミティブオフセット
		uint32_t animatedVertexStart;		// アニメーション頂点オフセット
		uint32_t isAnimated;				// アニメーションするかどうか
		uint32_t cullStart;					// カリングバッファオフセット
	};
	struct MeshMaterial
	{
		// マテリアルのテクスチャスケール値
		DXSM::Vector4 baseColor;

		DXSM::Vector3 emissive;
		float metallic;

		float roughness;

		DXSM::Vector3 pad;

		// テクスチャのSRVインデックス
		int albedIndex;					// アルベド
		int metaRoughnessIndex;			// メタリックラフネステクスチャ
		int emissiveIndex;
		int normalIndex;
	};
}