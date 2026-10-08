#pragma once
namespace Engine::Graphics
{
	//------------------------------------------------------------------------------------------
	// シーンのボリュメトリックフォグ
	//
	// 値はシーン(Engine::Scene::SceneAmbient)の持ち物で、毎フレーム SceneView へ流し込まれる。
	// パス(SceneVolumetricFogPass / SceneFogCompositePass)は受け取って送るだけ。
	// シーンのフォグとグラウンドダストで CB を分けてある。
	//------------------------------------------------------------------------------------------
	// シーン全体に一様に漂うフォグの調整値
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/SceneFogData.hlsli)と並びを合わせること
	struct SceneFogCB
	{
		Math::Vector3 fogColor = { 0.6f, 0.7f, 0.8f };	// フォグの色(届いた光に掛ける)
		float density = 0.002f;		// 濃さ(1m あたり)。0 ならシーンのフォグは掛からない

		float maxDistance = 1000.0f;	// 空(何も描かれていない画素)へ向けて積分する距離(m)
		float anisotropy = 0.3f;		// 平行光を散らす向きの偏り(-1..1)。正で光源の方向を見たときに明るい
		float lightScale = 1.0f;		// 媒質に届く光(環境光 + 平行光)に掛ける倍率
		float pad0 = 0.0f;
	};

	// 地面から一定の高さまで漂うチリ(グラウンドダスト)の調整値
	// time はパスが毎フレーム上書きする。
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/GroundDustData.hlsli)と並びを合わせること
	struct GroundDustCB
	{
		Math::Vector3 dustColor = { 0.8f, 0.75f, 0.65f };	// チリの色
		float density = 0.2f;		// 濃さ(1m あたり)。0 ならチリは出ない

		float height = 2.0f;		// チリが立つ高さ(地面から。この高さで濃さが 0 になる)
		float noiseScale = 0.1f;	// ノイズのワールド座標に掛ける倍率(大きいほど細かい)
		float time = 0.0f;			// パスが回り始めてからの経過時間(秒)。ノイズを流すのに使う
		float stepSize = 0.5f;		// チリの層の中をレイマーチする1歩の長さ(m)
	};

	// フォグをメインカラーへ重ねるときの調整値(SceneFogCompositePass が送る)
	// ※ HLSL 側(SceneFogCompositeCS の SceneFogCompositeData)と並びを合わせること
	struct SceneFogCompositeCB
	{
		float intensity = 1.0f;		// フォグの濃さに掛ける倍率
		int   enable = 1;			// 0 なら重ねずにそのまま通す
		float pad0[2] = {};
	};
}
