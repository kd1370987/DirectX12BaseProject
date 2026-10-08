#pragma once
namespace Engine::Graphics
{
	//------------------------------------------------------------------------------------------
	// グラウンドフィールド
	//
	// 地面に広がる波紋などの衝撃。アプリ側が SceneView::AddGroundImpulse で毎フレーム積み、
	// GraphicsEngine がフレームぶんの構造体バッファへ詰めて GroundFieldPass へ渡す。
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/GroundFieldData.hlsli)と並びを合わせること
	//------------------------------------------------------------------------------------------
	// グラウンドフィールドに伝える衝撃の最大数
	inline constexpr uint32_t MAX_GROUND_IMPULSES = 64;

	// グラウンドフィールドに伝える衝撃
	struct GroundImpulse
	{
		Math::Vector3 pos;
		float radius;

		float strength;
		float speed;
		float width;
		float lifetime;

		float age;			// 衝撃を出してからの経過時間(秒)。積む側が毎フレーム進める
		float pad0[3];
	};
}
