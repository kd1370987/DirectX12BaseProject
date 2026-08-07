#pragma once
#include "../IPanel.h"

namespace Engine
{
	namespace ECS
	{
		class World;
	}
	namespace Collision
	{
		struct RayInfo;
	}
}


namespace Engine::Editor
{
	class SceneViewPanel : public IPanel
	{
	public:
		~SceneViewPanel() override = default;

		const char* GetName() const override { return "SceneViewPanel"; };
		void OnDrawImGui(EditorContext& a_editContext) override;
		ImGuiWindowFlags GetFlags() const override { return ImGuiWindowFlags_MenuBar; }
	private:

		// シーンビュー上でのエンティティ選択
		void SelectEntityForMouse(EditorContext& a_editContext, Engine::ECS::World* a_pWorld, const ImVec2& a_pos, const ImVec2& a_rect);

		// レイと描画メッシュのAABBでエンティティをピッキングする(CollisionWorldは使わない)。
		// 当たり判定を持たないエンティティも選択できるよう、描画メッシュから直接判定する。
		Engine::ECS::Entity PickEntityByRay(Engine::ECS::World* a_pWorld, const Engine::Collision::RayInfo& a_ray);

		// ギズモ
		// ギズモ本体はプライマリ選択に出し、動かした分は選択中の全エンティティへ適用する。
		void GuizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect, EditorContext& a_editContext, Engine::ECS::World* a_pWorld);

		// ワールド空間の移動量をエンティティへ加算する。
		// 親を持つ場合は LocalTransform が親基準なので、移動量も親空間へ変換してから足す。
		void TranslateEntity(Engine::ECS::World* a_pWorld, const ECS::Entity& a_entity, DirectX::FXMVECTOR a_worldDelta);

		// 祖先に選択中のエンティティがいるか。
		// 親と子を同時に選んでいると、親の移動が子へ伝播したうえに子自身も動いて二重に移動してしまうので、
		// その判定に使う。
		bool IsAncestorSelected(Engine::ECS::World* a_pWorld, const EditorContext& a_editContext, const ECS::Entity& a_entity);

		// 選択エンティティの親のワールド行列を取得する。
		// 親がいない/親のワールドがまだ無い場合は単位行列を入れて false を返す。
		bool TryGetParentWorldMatrix(Engine::ECS::World* a_pWorld, const ECS::Entity& a_entity, DirectX::XMMATRIX& a_outParentMat);

		// ギズモに載せられない情報(各種オフセット位置・パーティクルの発生方向など)を
		// シーンビュー画像の上へ直接描くHUD。
		void DrawEntityHUD(const ImVec2& a_pos, const ImVec2& a_rect, const ECS::Entity& a_entity, Engine::ECS::World* a_pWorld);

		// ECS外オブジェクト用のギズモ(BaseObject::DrawGizmoへ委譲)
		void GameObjectGizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect, EditorContext& a_editContext);

		// メニューバー
		void SceneFileMenu(EditorContext& a_editContext);

		// シーンファイル操作
		void LoadScenePopup();
		void SaveScenePopup();

		void OpenSavePopup();
		void SaveScene(const Engine::GUID& a_guid);

	private:

		// シーンファイル操作系
		bool m_isLoadScenePopup = false;
		bool m_openLoadPopup = false;
		bool m_openSaveAsPopup = false;
		std::string m_sceneNameInput = "";

		// 操作用変数
		bool m_isSaveShortcut = false;
		bool m_canOverwrite = false;
		bool m_doOverwrite = false;

		Engine::GUID m_currentSceneGUID = Engine::DefaultGUID;
	};
}