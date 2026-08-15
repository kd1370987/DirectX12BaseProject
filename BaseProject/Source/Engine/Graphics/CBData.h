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
		void ExtractFrustumPlanes(const DXSM::Matrix& viewProj) // ★引数で転置前の行列を受け取る
		{
			const auto& _m = viewProj;
			// 各平面の抽出 : x , y , z は法線ベクトル , w は原点からの距離
			// [0] 左平面
			frustumPlanes[0] = DXSM::Vector4(_m._14 + _m._11, _m._24 + _m._21, _m._34 + _m._31, _m._44 + _m._41);
			// [1] 右平面
			frustumPlanes[1] = DXSM::Vector4(_m._14 - _m._11, _m._24 - _m._21, _m._34 - _m._31, _m._44 - _m._41);
			// [2] 下平面
			frustumPlanes[2] = DXSM::Vector4(_m._14 + _m._12, _m._24 + _m._22, _m._34 + _m._32, _m._44 + _m._42);
			// [3] 上平面
			frustumPlanes[3] = DXSM::Vector4(_m._14 - _m._12, _m._24 - _m._22, _m._34 - _m._32, _m._44 - _m._42);
			// [4] 近平面 (DirectXはZが0～1なので _13 等になる)
			frustumPlanes[4] = DXSM::Vector4(_m._13, _m._23, _m._33, _m._43);
			// [5] 遠平面
			frustumPlanes[5] = DXSM::Vector4(_m._14 - _m._13, _m._24 - _m._23, _m._34 - _m._33, _m._44 - _m._43);

			// 平面の平均化（正規化）
			for (int _i = 0; _i < 6; ++_i)
			{
				DirectX::XMVECTOR _plane = frustumPlanes[_i];
				_plane = DirectX::XMPlaneNormalize(_plane); // 法線ベクトルの長さを正規化 : Wも合わせてスケール
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
	//
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/AmbientData.hlsli)と
	//    1バイトもズレないよう、16バイト(float4)境界ごとに区切って並べること。
	//    HLSL の定数バッファは float3 が16バイト境界をまたぐと次の境界へ押し出される。
	struct alignas(256) AmbientData
	{
		// 環境光
		DirectX::XMFLOAT3 ambientColorScale = {0,0,0};
		float pad0;
		// ディレクショナルライト
		DirectX::XMFLOAT3 dlDir = {0,0,0};
		float pad1;
		DirectX::XMFLOAT3 dlColor = {0,0,0};
		float pad2;

		//------------------------------------------------------------------------------
		// 高さフォグ
		// heightFogHeight を境に、denseDown で指定した側へ heightFogMaxRange 進むまでを
		// 0%→100% で線形グラデーションする。マックスレンジより先は 100%(フォグ色一色)。
		//------------------------------------------------------------------------------
		DirectX::XMFLOAT3 heightFogColor = { 0.5f, 0.6f, 0.7f };	// フォグの色
		float heightFogMaxRange = 30.0f;							// 100% になるまでの距離(基準高さから)

		float heightFogHeight    = 0.0f;	// フォグが出始める高さ(ワールドY)
		int   heightFogEnable    = 0;		// 0 なら計算ごとスキップ
		int   heightFogDenseDown = 1;		// 1 = 下へ行くほど濃い / 0 = 上へ行くほど濃い
		float pad3;

		//------------------------------------------------------------------------------
		// 距離フォグ
		// distanceFogStart から distanceFogMaxRange までを 0%→100% で線形グラデーション
		// する。どちらもカメラからの深度。マックスレンジより奥は 100%。
		//------------------------------------------------------------------------------
		DirectX::XMFLOAT3 distanceFogColor = { 0.5f, 0.6f, 0.7f };	// フォグの色
		float distanceFogMaxRange = 200.0f;							// 100% になる距離

		float distanceFogStart  = 30.0f;	// フォグが出始める距離
		int   distanceFogEnable = 0;		// 0 なら計算ごとスキップ
		DirectX::XMFLOAT2 pad4;
	};

	// 被写界深度(DoF)の調整値
	// アクティブカメラの FocusParamComponent を CamSetShaderSystem が詰め、
	// CoCパスとDoFパスの両方へ送る。
	// ※ HLSL 側(Asset/Shader/Common/CB/CBDoFOption.hlsli)と並びを合わせること
	struct DoFOptionCB
	{
		float focusDistance;	// ピントが合う距離(カメラからの深度)
		float focusRange;		// ピントが合う幅
		float nearRange;		// 手前側が最大ボケになるまでの距離
		float farRange;			// 奥側が最大ボケになるまでの距離

		float maxBlurRadius;	// 最大ボケ半径(ピクセル)
		int   enable;			// 0 ならボカさずそのまま通す
		float pad0;
		float pad1;
	};

	// 川瀬式ブルームの調整値
	// OptionManager の BloomOption を、抽出パスと合成パスの両方が詰めて送る。
	// ※ HLSL 側(Asset/Shader/Common/CB/CBBloomOption.hlsli)と並びを合わせること
	struct BloomOptionCB
	{
		float threshold;	// 高輝度として抽出し始める輝度
		float softKnee;		// しきい値付近をなめらかにつなぐ幅の割合(0でハードカット)
		float intensity;	// 合成時のブルームの強さ
		int   enable;		// 0 ならブルームを掛けない
	};

	// ガウシアンブラーパスの設定値
	// 入力の解像度とボケ幅はパスごとに違うので、登録時に決めた値を毎フレーム送る。
	// ※ HLSL 側(Asset/Shader/Compute/PostEffect/Blur/GaussianBlurShader.hlsl)と並びを合わせること
	struct GaussianBlurCB
	{
		DirectX::XMFLOAT2 srcTexelSize;	// 入力テクスチャの1テクセルぶんのUV(= 1 / 入力解像度)
		float sigma;					// ガウス分布の標準偏差(入力テクセル単位)
		int   tapRadius;				// 片側のタップ数(0でブラーなし)
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

	// UIデータ
	// StructuredBuffer<UIData> と1バイトもズレないよう、16バイト(float4)境界を意識して並べる。
	// HLSLの構造化バッファは float2 が16バイト境界をまたぐ位置に来ると次の境界へ押し出される。
	// 各行がちょうど float4 に収まる順序にしておけばパディングのズレが起きない。
	// (row0: pos+axisX / row1: axisY+uvOffset / row2: color / row3: layer+texIndex+pad)
	//
	// 回転・アスペクト補正・ピボットはCPU(SubmitUI)側でピクセル空間で計算し、
	// クアッド頂点(-1..1)を線形変換する基底(axisX/axisY)とNDC中心座標(pos)として渡す。
	// シェーダーは pos + axisX*q.x + axisY*q.y を計算するだけでよい。
	struct UIData
	{
		DXSM::Vector2 pos;			// クアッド中心のNDC座標(平行移動成分)
		DXSM::Vector2 axisX;		// クアッドx方向の基底(NDC, 回転・アスペクト込み)

		DXSM::Vector2 axisY;		// クアッドy方向の基底(NDC, 回転・アスペクト込み)
		DXSM::Vector2 uvOffset;		// UVをずらす際のオフセット

		DXSM::Vector4 color;		// 色調補正

		float layer;				// Z順
		UINT texIndex;				// SRVインデックス
		DXSM::Vector2 _pad;			// 16バイトアライメント用
	};

	// サブメッシュ単位データ
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/SubsetData.hlsli)と並びを合わせること
	struct SubSetData
	{
		// テクスチャスケール
		DirectX::XMFLOAT4 baseColorScale = {};
		DirectX::XMFLOAT3 emissiveColorScale = {};
		float metallic = 0.0f;
		float roughness = 0.0f;

		// マテリアルとは独立した自己発光(ModelComponent の 発光色 × 発光強度)。
		// emissiveColorScale はエミッシブテクスチャに掛ける倍率なので、テクスチャを
		// 持たないモデルは何倍しても光らない。こちらは加算なので単体で光らせられる。
		DirectX::XMFLOAT3 emissiveAdd = {};
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

		uint32_t meshletCount;				// メッシュレットカウント
		DXSM::Vector3 pad;
	};
	struct MeshMaterial
	{
		// マテリアルのテクスチャスケール値
		DXSM::Vector4 baseColor;

		DXSM::Vector3 emissive;
		float metallic;

		float roughness;

		// マテリアルとは独立した自己発光(ModelComponent の 発光色 × 発光強度)。
		// emissive はエミッシブテクスチャに掛ける倍率なので、テクスチャを持たない
		// モデルは何倍しても光らない。こちらは加算なので単体で光らせられる。
		DXSM::Vector3 emissiveAdd;

		// テクスチャのSRVインデックス
		int albedoIndex;					// アルベド
		int metaRoughnessIndex;			// メタリックラフネステクスチャ
		int emissiveIndex;
		int normalIndex;
	};
}