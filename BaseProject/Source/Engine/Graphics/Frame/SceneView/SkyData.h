#pragma once
namespace Engine::Graphics
{
	//----------------------------------------------------------------------------------
	// スカイの設定
	//
	// シーン(Engine::Scene::SceneAmbient)が持ち、毎フレーム SceneView へ流し込む。
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
}
