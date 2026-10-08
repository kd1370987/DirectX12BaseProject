#pragma once
namespace Engine::Graphics
{
	// 環境データ
	//
	// シーン(Engine::Scene::SceneAmbient)が持ち、毎フレーム SceneView へ流し込む。
	// 平行光はここではなく LightManager が持つ。
	// 影とGIがレイを飛ばす先と、ディファードが足す光を1か所にまとめるため。
	//
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/AmbientData.hlsli)と
	//    1バイトもズレないよう、16バイト(float4)境界ごとに区切って並べること。
	//    HLSL の定数バッファは float3 が16バイト境界をまたぐと次の境界へ押し出される。
	struct alignas(256) AmbientData
	{
		// 環境光
		Math::Vector3 ambientColorScale = {0,0,0};
		float pad0;
		//------------------------------------------------------------------------------
		// 高さフォグ
		// heightFogHeight を境に、denseDown で指定した側へ heightFogMaxRange 進むまでを
		// 0%→100% で線形グラデーションする。マックスレンジより先は 100%(フォグ色一色)。
		//------------------------------------------------------------------------------
		Math::Vector3 heightFogColor = { 0.5f, 0.6f, 0.7f };	// フォグの色
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
		Math::Vector3 distanceFogColor = { 0.5f, 0.6f, 0.7f };	// フォグの色
		float distanceFogMaxRange = 200.0f;							// 100% になる距離

		float distanceFogStart  = 30.0f;	// フォグが出始める距離
		int   distanceFogEnable = 0;		// 0 なら計算ごとスキップ
		Math::Vector2 pad4;
	};
}
