#include "EntityInspector.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Scene/BaseScene/BaseScene.h"
#include "../../../../Scene/SceneManager/SceneManager.h"

#include "../../../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Helper/EditorHelper.h"

#include "../../../../../Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../../Application/Components/Persistence/NameComponent.h"
#include "../../../../../Application/Components/Hierarchy/HierarchyComponent.h"
#include "../../../../../Application/Components/Persistence/GUIDComponent.h"

#include "../../../../../Application/Components/Resource/AnimatorComponent.h"
#include "../../../../../Application/Components/Resource/NodePoseComponent.h"
#include "../../../../../Application/Components/Resource/SkeletonPoseComponent.h"

namespace Engine::Editor::Inspector
{
	// コンポーネントの追加
	void AddComponent(EditorContext& a_editContext, Engine::ECS::World* a_pWorld)
	{
		// 指定して追加
		// 編集対象はプライマリ選択(選択リストの先頭)
		const ECS::Entity _entity = a_editContext.GetPrimaryEntity();

		if (ImGui::BeginCombo("Add Component", "Select..."))
		{
			// 数が増えると探せなくなるので名前で絞り込めるようにする
			const std::string& _search = EditorHelper::DrawSearchBox();

			const ECS::Signature& _sig = a_pWorld->GetSignature(_entity);
			for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
			{
				// 所持していたら表示しない
				if (_sig.test(_typeID)) continue;

				if (!EditorHelper::IsMatchSearch(_search, _meta.name)) continue;

				// メタ情報から名前表示
				if (ImGui::Selectable(_meta.name.c_str()))
				{
					// コンポーネントの追加
					a_pWorld->AddComponent(_typeID, _entity);
				}
			}

			ImGui::EndCombo();
		}

		// 動きごとに追加
		// アニメーションするエンティティの場合
		if (ImGui::Button("AnimationEntity"))
		{
			ECS::ChangeEntityCmd _cmd = {};
			_cmd.entity = _entity;
			_cmd.toSig = a_pWorld->GetSignature(_entity);
			_cmd.toSig.set(a_pWorld->GetCompTypeID<AnimatorComponent>());
			_cmd.toSig.set(a_pWorld->GetCompTypeID<NodePoseComponent>());
			_cmd.toSig.set(a_pWorld->GetCompTypeID<SkeletonPoseComponent>());
			_cmd.toSig.set(a_pWorld->GetCompTypeID<PostDeserializeTag>());
			if (_cmd.toSig.test(a_pWorld->GetCompTypeID<ActiveTag>()))
			{
				_cmd.toSig.reset(a_pWorld->GetCompTypeID<ActiveTag>());
			}
			a_pWorld->AddChangeSigCommand(_cmd);
		}
	}

	// コンポーネントの削除
	void SubmitCommponent(EditorContext& a_editContext, Engine::ECS::World* a_pWorld, ECS::ComponentTypeID a_typeID)
	{
		ImGui::Separator();

		// 削除系の色分けは EditorHelper に寄せてある(色をここで持たない)
		if (Engine::Editor::EditorHelper::DeleteButton("RemoveComponnet"))
		{
			a_pWorld->SubmitComponent(a_typeID, a_editContext.GetPrimaryEntity());
		}
	}

	//======================================================================================
	// エンティティのプレハブ化
	//======================================================================================

	// プレハブアセットを置くルートフォルダ
	static const std::string PREFAB_ROOT_DIR = "Asset/Prefab/";

	// プレハブ名からアセットのベースパス(拡張子なし)を作る
	std::string MakePrefabBasePath(const std::string& a_name)
	{
		return PREFAB_ROOT_DIR + a_name + "/" + a_name;
	}

	// その名前がすでに使われているか
	bool IsUsedPrefabName(const std::string& a_name)
	{
		const std::string _basePath = MakePrefabBasePath(a_name);

		// アセットデータベースに登録済み
		if (Resource::AssetDatabase::Instance().GetGUIDFromFilePath(_basePath) != Engine::DefaultGUID)
		{
			return true;
		}

		// 未登録でもファイルが残っているケースを拾っておく
		std::error_code _ec;
		if (std::filesystem::exists(_basePath + ".ojprfb", _ec)) return true;
		if (std::filesystem::exists(_basePath + ".obprfb", _ec)) return true;

		return false;
	}

	// ファイル名に使えない文字を落とす
	// ('.' はベースパス比較(拡張子除去)が狂うので同じく落とす)
	std::string SanitizePrefabName(const std::string& a_name)
	{
		std::string _res = {};
		for (const char& _c : a_name)
		{
			if (std::strchr("\\/:*?\"<>|.", _c) != nullptr) continue;
			_res += _c;
		}

		// 前後の空白を削る
		const size_t _begin = _res.find_first_not_of(" \t");
		if (_begin == std::string::npos) return "";
		const size_t _end = _res.find_last_not_of(" \t");

		return _res.substr(_begin, _end - _begin + 1);
	}

	// 重複しないプレハブ名を作る : 被っていたら末尾に _01, _02... と加算していく
	std::string MakeUniquePrefabName(const std::string& a_name)
	{
		const std::string _baseName = SanitizePrefabName(a_name);
		if (_baseName.empty()) return "";
		if (!IsUsedPrefabName(_baseName)) return _baseName;

		for (UINT _i = 1; _i < 1000; ++_i)
		{
			char _suffix[8] = {};
			std::snprintf(_suffix, sizeof(_suffix), "_%02u", _i);

			std::string _candidate = _baseName + _suffix;
			if (!IsUsedPrefabName(_candidate)) return _candidate;
		}

		// 空きが見つからなかった
		return "";
	}

	// システムフェーズ用のタグか : 実体化時に付け直されるのでプレハブには保存しない
	bool IsSystemPhaseTag(ECS::World* a_pWorld, ECS::ComponentTypeID a_typeID)
	{
		return
			a_typeID == a_pWorld->GetCompTypeID<PostDeserializeTag>() ||
			a_typeID == a_pWorld->GetCompTypeID<AwakeTag>() ||
			a_typeID == a_pWorld->GetCompTypeID<StartTag>() ||
			a_typeID == a_pWorld->GetCompTypeID<ActiveTag>() ||
			a_typeID == a_pWorld->GetCompTypeID<ReleaseTag>();
	}

	// エンティティの現在の状態をプレハブアセットとして保存する
	// 子エンティティは含まない(プレハブは1エンティティのテンプレート)
	void CreatePrefabFromEntity(ECS::World* a_pWorld, const ECS::Entity& a_entity, const std::string& a_name)
	{
		if (!a_pWorld || a_entity == ECS::Limits::INVALID_ENTITY) return;

		// 名前の重複を解決する
		const std::string _prefabName = MakeUniquePrefabName(a_name);
		if (_prefabName.empty())
		{
			ENGINE_LOG("プレハブ名を決定できませんでした : %s", a_name.c_str());
			return;
		}

		// ---- 所持コンポーネントをまるごとコピーする ----
		Resource::Prefab _prefab;

		const ECS::Signature _sig = a_pWorld->GetSignature(a_entity);
		for (size_t _i = 0; _i < _sig.size(); ++_i)
		{
			if (!_sig.test(_i)) continue;

			auto _compTypeID = static_cast<ECS::ComponentTypeID>(_i);
			if (IsSystemPhaseTag(a_pWorld, _compTypeID)) continue;

			const uint8_t* _pSrc = a_pWorld->NRefData(a_entity, _compTypeID);
			if (!_pSrc) continue;

			_prefab.AddComponentData(a_pWorld, _compTypeID, _pSrc);
		}

		// ---- 実体固有の値はテンプレートに持ち込まない ----
		// GUIDは実体化時に GUIDFixupSystem が振り直すので空にしておく
		if (uint8_t* _pGUIDData = _prefab.RefData(a_pWorld->GetCompTypeID<GUIDComponent>()))
		{
			GUIDComponent _guidComp = {};
			std::memcpy(_pGUIDData, &_guidComp, sizeof(_guidComp));
		}
		// 親はシーン上のエンティティを指しているので、ルート扱いへ戻す
		if (uint8_t* _pHierarchyData = _prefab.RefData(a_pWorld->GetCompTypeID<HierarchyComponent>()))
		{
			HierarchyComponent _hierarchyComp = {};
			std::memcpy(_pHierarchyData, &_hierarchyComp, sizeof(_hierarchyComp));
		}

		// ---- アセットとして登録して保存する ----
		const std::string _basePath = MakePrefabBasePath(_prefabName);

		// メタ情報を登録して GUID を発行
		auto _guid = Resource::AssetDatabase::Instance().AddMetaData(_basePath, "Prefab");

		_prefab.Save(a_pWorld, _basePath);

		// 作ったそばから使えるようにプールへ登録
		Resource::ResourceManager::Instance().AddResourceAndGUID(std::move(_prefab), _guid);

		ENGINE_LOG("プレハブを作成しました : %s", _basePath.c_str());
	}

	// プレハブ化ボタン + 名前入力ポップアップ
	void CreatePrefabButton(EditorContext& a_editContext, ECS::World* a_pWorld)
	{
		// 入力中のプレハブ名
		static char _nameCach[256] = "";

		if (Engine::Editor::EditorHelper::CreateButton("Create Prefab"))
		{
			// 入力の初期値はエンティティ名にしておく
			std::string _defaultName = "NewPrefab";

			const ECS::Entity _entity = a_editContext.GetPrimaryEntity();
			if (a_pWorld->HasComponent<NameComponent>(_entity))
			{
				const char* _pName = a_pWorld->RefData<NameComponent>(_entity)->name;
				if (_pName && _pName[0] != '\0') _defaultName = _pName;
			}

			std::snprintf(_nameCach, sizeof(_nameCach), "%s", _defaultName.c_str());

			ImGui::OpenPopup("CreatePrefabPopup");
		}

		if (ImGui::BeginPopupModal("CreatePrefabPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Prefab Name");
			ImGui::InputText("##PrefabName", _nameCach, sizeof(_nameCach));

			// 実際に保存される名前(被っていたら _01 が加算されたもの)を出しておく
			const std::string _inputName = _nameCach;
			const std::string _saveName = MakeUniquePrefabName(_inputName);

			if (_saveName.empty())
			{
				ImGui::TextDisabled("Input prefab name...");
			}
			else
			{
				ImGui::TextDisabled("Save to : %s", MakePrefabBasePath(_saveName).c_str());
			}

			ImGui::BeginDisabled(_saveName.empty());
			if (ImGui::Button("Save"))
			{
				CreatePrefabFromEntity(a_pWorld, a_editContext.GetPrimaryEntity(), _inputName);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void EntityInspector(EditorContext& a_editContext)
	{
		// ワールド取得
		Engine::ECS::World* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit()) return;

		// 削除は選択中のエンティティすべてが対象
		const int _selectedCount = static_cast<int>(a_editContext.selectedEntities.size());
		const std::string _removeLabel = (_selectedCount > 1)
			? ("RemoveEntity (" + std::to_string(_selectedCount) + ")")
			: std::string("RemoveEntity");

		if (Engine::Editor::EditorHelper::DeleteButton(_removeLabel.c_str()))
		{
			// 解放予約だけしておく。実際に消えるのは次の BeginFrame で、
			// その前に Release フェーズが走るので、借りているもの
			// (サウンドのボイス・ポーズ行列など)を返してから消える。
			// この場でチャンクは動かないため、選択リストをそのまま舐めてよい。
			for (const Engine::ECS::Entity& _removeEntity : a_editContext.selectedEntities)
			{
				if (_removeEntity == Engine::ECS::Limits::INVALID_ENTITY) continue;
				_pWorld->AddReleaseEntity(_removeEntity);
			}
			a_editContext.ClearEntitySelection();
		}

		const Engine::ECS::Entity _entity = a_editContext.GetPrimaryEntity();
		if (_entity == Engine::ECS::Limits::INVALID_ENTITY)
		{
			return;
		}

		// 選択中のエンティティをプレハブとして保存する(対象はプライマリ選択の1体)
		CreatePrefabButton(a_editContext, _pWorld);

		ImGui::Text("Entity ID : %llu", _entity);

		// 複数選択中は、コンポーネント編集の対象が先頭1体だけであることを明示しておく
		// (移動と削除は選択中すべてに効く)
		if (_selectedCount > 1)
		{
			ImGui::TextDisabled("(%d selected / editing the first)", _selectedCount);
		}

		// エンティティが持っているコンポーネントを羅列する
		const Engine::ECS::EntityLocation& _location = _pWorld->GetLocation(_entity);
		Engine::ECS::Signature _sig = _pWorld->GetSignature(_entity);

		ECS::CompEditContext _compEditContext = {};
		_compEditContext.pWorld = _pWorld;

		for (size_t _typeID = 0; _typeID < _sig.size(); ++_typeID)
		{
			// 持っているコンポーネントのみ表示
			if (_sig.test(_typeID))
			{
				// ツリーノード表示
				auto& _metaData = _pWorld->GetComponentMetaData(static_cast<Engine::ECS::ComponentTypeID>(_typeID));

				if (ImGui::TreeNodeEx(_metaData.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					// コンポーネントごとの特殊エディター処理を入れる
					auto _func = _pWorld->GetCompFunc(_typeID).edit;
					_compEditContext.entity = _entity;
					_compEditContext.pData = _pWorld->NRefData(_entity, _typeID);
					if (_func)
					{
						_func(_compEditContext);
					}

					// コンポーネントを削除するボタン
					SubmitCommponent(a_editContext,_pWorld, _typeID);

					ImGui::TreePop();
				}

			}
		}

		// エンティティに対してコンポーネントを増やす
		AddComponent(a_editContext,_pWorld);
	}
}

