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

	// 環境データ
	//
	// 平行光はここではなく LightManager が持つ。
	// 影とGIがレイを飛ばす先と、ディファードが足す光を1か所にまとめるため。
	//
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/AmbientData.hlsli)と
	//    1バイトもズレないよう、16バイト(float4)境界ごとに区切って並べること。
	//    HLSL の定数バッファは float3 が16バイト境界をまたぐと次の境界へ押し出される。
	struct alignas(256) AmbientData
	{
		// 環境光
		DirectX::XMFLOAT3 ambientColorScale = {0,0,0};
		float pad0;
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

	//----------------------------------------------------------------------------------
	// スカイの設定
	//
	// シーンに置いた SceneAmbientObject が持ち、毎フレームここへ流し込む。
	// スカイドームのメッシュは置かず、画面の各ピクセルが見ている方向から
	// 直接スカイテクスチャを引くので、ドームの形はこの2つの値で決まる。
	//   horizonHeight : ドームの中心の高さ(ワールドY)。ここが地平線になる
	//   radius        : ドームの半径。小さいほどカメラの上下で地平線が強く動く
	//
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/SkyData.hlsli)と並びを合わせること
	//----------------------------------------------------------------------------------
	struct SkyData
	{
		float exposure      = 1.0f;		// スカイの色に掛ける露出倍率
		float horizonHeight = 0.0f;		// 地平線の高さ(ワールドY) = 仮想ドームの中心の高さ
		float radius        = 500.0f;	// 仮想ドームの半径
		float rotationDeg   = 0.0f;		// 方位の回転(度)

		// 空に被写界深度を掛けるか。
		// 空は深度が far のまま残るので、既定では掛けない(掛けると空だけべったり滲む)。
		// 判定と適用は CoCShader 側
		int   isSkyDof      = 0;		// 0 なら空だけボケない
		float dofScale      = 1.0f;		// 掛けるときのボケ量の倍率
		float pad0          = 0.0f;
		float pad1          = 0.0f;
	};

	// 被写界深度(DoF)の調整値
	// アクティブカメラの FocusParamComponent を CamSetShaderSystem が詰め、
	// CoCパスとDoFパスの両方へ送る。
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/DoFOptionData.hlsli)と並びを合わせること
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

	// ラジアルブラーの調整値
	// アクティブカメラの RadialBlurComponent を CamSetShaderSystem が詰め、
	// RadialBlurPass へ送る。
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/RadialBlurOptionData.hlsli)と並びを合わせること
	struct RadialBlurOptionCB
	{
		DirectX::XMFLOAT2 blurCenter;	// ブラーの中心(UV : 画面左上が0、右下が1)
		float strength;					// 引きずる長さ(UV単位。中心からの距離に比例して伸びる)
		int   sampleCount;				// サンプル数

		float radius;					// ここまで(中心からのUV距離)はボカさない
		float falloff;					// radius から先の効きの立ち上がり
		int   enable;					// 0 ならボカさずそのまま通す
		float pad0;
	};

	// 川瀬式ブルームの調整値
	// OptionManager の BloomOption を、抽出パスと合成パスの両方が詰めて送る。
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/BloomOptionData.hlsli)と並びを合わせること
	struct BloomOptionCB
	{
		float threshold;	// 高輝度として抽出し始める輝度
		float softKnee;		// しきい値付近をなめらかにつなぐ幅の割合(0でハードカット)
		float intensity;	// 合成時のブルームの強さ
		int   enable;		// 0 ならブルームを掛けない
	};

	// ガウシアンブラーパスの設定値
	// 入力の解像度とボケ幅はパスごとに違うので、登録時に決めた値を毎フレーム送る。
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/GaussianBlurSetting.hlsli)と並びを合わせること
	struct GaussianBlurCB
	{
		DirectX::XMFLOAT2 srcTexelSize;	// 入力テクスチャの1テクセルぶんのUV(= 1 / 入力解像度)
		float sigma;					// ガウス分布の標準偏差(入力テクセル単位)
		int   tapRadius;				// 片側のタップ数(0でブラーなし)
	};

	// UIデータ
	// StructuredBuffer<UIData> と1バイトもズレないよう、16バイト(float4)境界を意識して並べる。
	// HLSLの構造化バッファは float2 が16バイト境界をまたぐ位置に来ると次の境界へ押し出される。
	// 各行がちょうど float4 に収まる順序にしておけばパディングのズレが起きない。
	// (row0: pos+axisX / row1: axisY+uvOffset / row2: color / row3: layer+texIndex+uvScale)
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

		// 重なり順 : 大きいほど手前。
		// UIパスは深度を持たないので、これを見てCPU側が積んだ順を並べ替える
		float layer;
		UINT texIndex;				// SRVインデックス

		// UVに掛ける倍率。1つのテクスチャに並べた絵を切り出すために使う
		// (数字の 0〜9 を横に並べたものから1文字だけ出す、など)。
		// uv * uvScale + uvOffset の順で効く。既定は等倍。
		// row3 の余りに入れているので、構造体の大きさは変わらない
		DXSM::Vector2 uvScale = { 1.0f, 1.0f };
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