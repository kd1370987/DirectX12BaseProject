#pragma once
namespace Engine::ECS
{
	// 当たり判定の結果を受け取るコンポーネント(①方式)。
	// 毎フレーム先頭で other=INVALID にクリアし、検出システムが埋める。
	// 反応システム(OnHit)は other != INVALID を見て処理する。
	struct CollisionEvent
	{
		Entity            other  = Limits::INVALID_ENTITY;	// 当たった相手(INVALID=未ヒット)
		DirectX::XMFLOAT3 hitPos = { 0.0f, 0.0f, 0.0f };		// 当たった位置(エフェクト発生点)
		DirectX::XMFLOAT3 hitDir = { 0.0f, 0.0f, 0.0f };		// 法線 or 相手方向
	};
}

template<>
struct Engine::ECS::ComponentTraits<Engine::ECS::CollisionEvent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		// ランタイム専用(毎フレーム再生成)なので保存しない
	}

	static void Edit(CompEditContext& a_context)
	{
		Engine::ECS::CollisionEvent& _comp =
			Engine::Editor::GetValue<Engine::ECS::CollisionEvent>(a_context.pData);

		if (_comp.other == Engine::ECS::Limits::INVALID_ENTITY)
		{
			ImGui::Text("No hit");
		}
		else
		{
			ImGui::Text("Hit Entity : %d", (int)_comp.other);
			ImGui::Text("Hit Pos    : %.2f, %.2f, %.2f", _comp.hitPos.x, _comp.hitPos.y, _comp.hitPos.z);
		}
	}
};
