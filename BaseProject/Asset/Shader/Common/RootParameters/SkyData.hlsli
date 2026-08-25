// スカイの設定。シーンの SceneAmbientObject が持つ値を流し込む。
//
// スカイドームのメッシュは置かず、各ピクセルの視線を仮想ドーム
// (中心の高さ = horizonHeight / 半径 = radius)へ飛ばして交点の方向でテクスチャを引く。
// カメラが horizonHeight から離れるほど地平線が動き、radius が小さいほどその動きは強い。
//
// ※ CPU 側 Engine::Graphics::SkyData と並びを合わせること
#ifndef ROOTPARAM_SKY_DATA_HLSLI
#define ROOTPARAM_SKY_DATA_HLSLI

struct SkyData
{
	float exposure;
	float horizonHeight;	// 地平線の高さ(ワールドY) = 仮想ドームの中心
	float radius;
	float rotationDeg;		// 方位の回転(度)

	// 空はメッシュを持たないため深度が far(1.0)のまま残る。
	// 距離だけで判定すると「一番遠いもの」として最大の奥ボケが掛かり空だけが滲むので、
	// 既定では掛けない。判定するのは CoCShader で、ここは値を運ぶだけ
	int   isSkyDof;
	float dofScale;			// 掛けるときのボケ量倍率(1.0 で他の遠景と同じ)
	float2 pad;
};

#endif
