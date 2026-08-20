#pragma once

namespace Engine::GameObject
{
	class BaseObject;
}

namespace Engine::Editor
{
	class EditorCamera;
	class Profiler;

	/// <summary>
	/// インスペクターのモード
	/// </summary>
	enum class EInspectorType
	{
		None,
		Entity,
		Asset,
		Game,
	};

	/// <summary>
	/// プレハブのコピー&ペースト用クリップボード。
	///
	/// シグネチャだけでなくコンポーネントの初期値バイト列ごと持つので、
	/// 貼り付け先は編集済みの値まで引き継げる。
	/// コンポーネントのタイプIDはワールド生成時に同じ順で振られるため、
	/// 実行中は別シーンのプレハブへ貼っても同じ型を指す。
	/// (アプリを閉じると消える。ファイルには残らない)
	/// </summary>
	struct PrefabClipboard
	{
		bool isValid = false;
		ECS::Signature signature = {};
		std::unordered_map<ECS::ComponentTypeID, std::vector<uint8_t>> dataMap = {};

		// クリップボードに載っているコンポーネント数
		size_t GetCount() const { return signature.count(); }
	};

	/// <summary>
	/// パネル間でやり取りされるメモ帳
	/// </summary>
	struct EditorContext
	{
		float appWindowWidth = 0;
		float appWindowHeight = 0;

		// 現在のインスペクターモード
		EInspectorType eInspectorType = EInspectorType::None;

		// 選択中のアセット
		Resource::AssetProperty* pAssetProp = nullptr;

		// 選択中のエンティティ
		std::vector<ECS::Entity> selectedEntities = { ECS::Limits::INVALID_ENTITY };	// 一番先頭が単体選択、インスペクター表示
		bool m_isSelecting = false;													// LCtr押下中は複数選択できる

		//==============================================================================
		// エンティティ選択の操作
		//
		// selectedEntities は
		//   ・空にしない(未選択のときは INVALID_ENTITY を1つだけ持つ)
		//   ・先頭がプライマリ選択＝インスペクターに出るもの
		// を不変条件にしている。直接書き換えず必ずこれらを通すこと。
		//==============================================================================

		// インスペクター表示・ギズモ操作の対象になるプライマリ選択
		ECS::Entity GetPrimaryEntity() const
		{
			if (selectedEntities.empty()) return ECS::Limits::INVALID_ENTITY;
			return selectedEntities.front();
		}

		// そのエンティティが選択されているか(ヒエラルキーのハイライト用)
		bool IsSelectedEntity(const ECS::Entity& a_entity) const
		{
			if (a_entity == ECS::Limits::INVALID_ENTITY) return false;
			return std::find(selectedEntities.begin(), selectedEntities.end(), a_entity) != selectedEntities.end();
		}

		// 選択解除。空にはせず INVALID を1つだけ残す
		void ClearEntitySelection()
		{
			selectedEntities.assign(1, ECS::Limits::INVALID_ENTITY);
		}

		// エンティティを選択する
		// a_isAdditive : true(LCtrl押下中)なら追加選択。すでに選択済みなら選択から外す(トグル)
		//                false なら単体選択で置き換える
		void SelectEntity(const ECS::Entity& a_entity, bool a_isAdditive)
		{
			if (a_entity == ECS::Limits::INVALID_ENTITY)
			{
				ClearEntitySelection();
				return;
			}

			// 単体選択 : 中身を入れ替えるだけ
			if (!a_isAdditive)
			{
				selectedEntities.assign(1, a_entity);
				return;
			}

			// 追加選択 : すでに入っていればトグルで外す
			auto _it = std::find(selectedEntities.begin(), selectedEntities.end(), a_entity);
			if (_it != selectedEntities.end())
			{
				selectedEntities.erase(_it);
				if (selectedEntities.empty()) ClearEntitySelection();
				return;
			}

			// 未選択状態のダミー(INVALID)が残っていたら取り除いてから追加する
			std::erase(selectedEntities, ECS::Limits::INVALID_ENTITY);
			selectedEntities.push_back(a_entity);
		}

		// 選択からエンティティを取り除く(エンティティ削除時など)
		void DeselectEntity(const ECS::Entity& a_entity)
		{
			std::erase(selectedEntities, a_entity);
			if (selectedEntities.empty()) ClearEntitySelection();
		}

		// 選択中のECS外オブジェクト(GameObjectManager管理下)
		GameObject::BaseObject* pGameObject = nullptr;

		// プレハブのシグネチャ+初期値のコピー先
		PrefabClipboard prefabClipboard = {};

		// エディターカメラポインタ
		EditorCamera* pEditorCamera = nullptr;

		// プロファイラポインタ : 計測はプロファイラ側が行い、パネルは結果を読むだけ
		Profiler* pProfiler = nullptr;

		//==============================================================================
		// シーンに紐づくものを捨てる
		//
		// 選択中のエンティティとゲームオブジェクトは、そのシーンのワールド・
		// オブジェクトマネージャーが持っているものを指している。
		// シーンが切り替わると実体ごと消えるため、持ち越すと解放済みメモリを触る。
		// アセット選択(pAssetProp)はアセットデータベースの持ち物でシーンを跨いで
		// 生きているので、ここでは捨てない。
		//==============================================================================
		void ClearSceneContext()
		{
			ClearEntitySelection();
			pGameObject = nullptr;

			// アセットを見ていたならその表示は残す
			if (eInspectorType == EInspectorType::Entity || eInspectorType == EInspectorType::Game)
			{
				eInspectorType = EInspectorType::None;
			}
		}
	};
}