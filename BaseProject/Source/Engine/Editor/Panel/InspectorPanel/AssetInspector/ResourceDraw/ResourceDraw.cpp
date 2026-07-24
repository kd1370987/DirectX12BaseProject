#include "ResourceDraw.h"

#include "ModelEdit/ModelEdit.h"
#include "MeshEdit/MeshEdit.h"
#include "MaterialEdit/MaterialEdit.h"
#include "AnimationEdit/AnimationEdit.h"
#include "TextureEdit/TextureEdit.h"
#include "ShaderEdit/ShaderEdit.h"
#include "ShadingModelTableEdit/ShadingModelTableEdit.h"
#include "ParticleEdit/ParticleEdit.h"
#include "AnimatorEdit/AnimatorEdit.h"
#include "ActionStateMachineEdit/ActionStateMachineEdit.h"

#include "../../../../../Resource/Data/Model/IO/ModelConverter/ModelConverter.h"

// プレハブ編集用(ECSのエンティティインスペクタと同じ構成で描く)
#include "Engine/ECS/World/World.h"
#include "Engine/Scene/SceneManager/SceneManager.h"

namespace Engine::Editor::Inspector
{
	namespace
	{
		//-----------------------------------------------------------------------------------------
		// アセットの解決
		// ロード済みなら実体を返し、未ロードならロードボタンを出してnullptrを返す
		//-----------------------------------------------------------------------------------------
		template<typename TResource>
		TResource* ResolveAsset(const Engine::GUID& a_guid)
		{
			if (!Resource::ResourceManager::Instance().Has<TResource>(a_guid))
			{
				ImGui::Text("No loaded file");
				if (ImGui::Button("Load"))
				{
					Resource::ResourceManager::Instance().Load<TResource>(a_guid);
				}
				return nullptr;
			}

			auto _handle = Resource::ResourceManager::Instance().GetCache<TResource>(a_guid);
			auto* _pResource = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pResource)
			{
				ImGui::Text("Not found asset");
				return nullptr;
			}

			return _pResource;
		}
	}

	//-----------------------------------------------------------------------------------------
	// モデル
	//-----------------------------------------------------------------------------------------
	void ModelDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pModel = ResolveAsset<Resource::Model>(_guid);
		if (!_pModel) { return; }

		// バイナリへの変換
		if (ImGui::Button("Convert"))
		{
			auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			Resource::Converter::ModelConverter::ConvertModelDataToBinary(_filePath);
			ENGINE_LOG("モデルのconvert処理が完了 : %s", _filePath.c_str());
		}

		ImGui::Separator();

		ModelEdit(a_editContext, _pModel);
	}

	//-----------------------------------------------------------------------------------------
	// テクスチャ
	//-----------------------------------------------------------------------------------------
	void TextureDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		// 名前と、GUID表示
		auto _fileName = Resource::AssetDatabase::Instance().GetFileNameFromGUID(_guid);
		ImGui::Text("%s", _fileName.c_str());
		ImGui::Text("%s", _guid.String().c_str());

		ImGui::Separator();

		auto* _pTexture = ResolveAsset<Resource::Texture>(_guid);
		if (!_pTexture) { return; }

		TextureEdit(a_editContext, _pTexture);
	}

	//-----------------------------------------------------------------------------------------
	// アニメーター(アニメ用ステートマシン)
	//-----------------------------------------------------------------------------------------
	void AnimatorDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pAnimator = ResolveAsset<Resource::AnimatorAsset>(_guid);
		if (!_pAnimator) { return; }

		// ノードエディタ側がハンドルを必要とするため取得しておく
		auto _handle = Resource::ResourceManager::Instance().GetCache<Resource::AnimatorAsset>(_guid);

		AnimatorEdit(a_editContext, _pAnimator, _handle);
	}

	//-----------------------------------------------------------------------------------------
	// ゲームプレイ用ステートマシン
	//-----------------------------------------------------------------------------------------
	void ActionStateMachineDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pAsset = ResolveAsset<Resource::ActionStateMachineAsset>(_guid);
		if (!_pAsset) { return; }

		auto _handle = Resource::ResourceManager::Instance().GetCache<Resource::ActionStateMachineAsset>(_guid);

		ActionStateMachineEdit(a_editContext, _pAsset, _handle);
	}

	//-----------------------------------------------------------------------------------------
	// パーティクル
	//-----------------------------------------------------------------------------------------
	void ParticleDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pParticles = ResolveAsset<Resource::ParticlesAsset>(_guid);
		if (!_pParticles) { return; }

		ParticleEdit(a_editContext, _pParticles);
	}

	//-----------------------------------------------------------------------------------------
	// マテリアル
	//-----------------------------------------------------------------------------------------
	void MaterialDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pMaterial = ResolveAsset<Resource::Material>(_guid);
		if (!_pMaterial) { return; }

		MaterialEdit(a_editContext, _pMaterial);
	}

	//-----------------------------------------------------------------------------------------
	// メッシュ
	//-----------------------------------------------------------------------------------------
	void MeshDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pMesh = ResolveAsset<Resource::Mesh>(_guid);
		if (!_pMesh) { return; }

		MeshEdit(a_editContext, _pMesh);
	}

	//-----------------------------------------------------------------------------------------
	// アニメーション
	//-----------------------------------------------------------------------------------------
	void AnimationDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pAnimation = ResolveAsset<Resource::AnimationData>(_guid);
		if (!_pAnimation) { return; }

		AnimationEdit(a_editContext, _pAnimation);
	}

	//-----------------------------------------------------------------------------------------
	// シェーダー
	//-----------------------------------------------------------------------------------------
	void ShaderDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pShader = ResolveAsset<Resource::Shader>(_guid);
		if (!_pShader) { return; }

		ShaderEdit(a_editContext, _pShader);
	}

	//-----------------------------------------------------------------------------------------
	// シェーディングモデルテーブル
	//-----------------------------------------------------------------------------------------
	void ShadingModelTableDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pTable = ResolveAsset<Resource::ShadingModelTable>(_guid);
		if (!_pTable) { return; }

		ShadingModelTableEdit(a_editContext, _pTable);
	}

	//-----------------------------------------------------------------------------------------
	// プレハブ
	// ECS のエンティティインスペクタと同じ構成で、コンポーネントを追加・削除・編集する
	//-----------------------------------------------------------------------------------------
	void PrefabDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pPrefab = ResolveAsset<Resource::Prefab>(_guid);
		if (!_pPrefab) { return; }

		// コンポーネントのメタ情報・編集関数を引くために World が必要
		ECS::World* _pWorld = Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit())
		{
			ImGui::Text("No active World.");
			ImGui::Text("Open a scene to edit prefab components.");
			return;
		}

		// ---- 保存 ----
		if (ImGui::Button("Save"))
		{
			auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			_pPrefab->Save(_pWorld, _path);
			ENGINE_LOG("Save Prefab : %s", _path.c_str());
		}
		ImGui::Separator();

		// ---- 所持コンポーネントの羅列・編集 ----
		const ECS::Signature& _sig = _pPrefab->GetSignature();

		ECS::CompEditContext _compEditContext = {};
		_compEditContext.pWorld = _pWorld;
		_compEditContext.entity = ECS::Limits::INVALID_ENTITY;	// プレハブは実体を持たない

		// 反復中に消すと崩れるので削除は予約する
		ECS::ComponentTypeID _removeTypeID = ECS::Limits::INVALID_COMPONENTTYPEID;

		for (size_t _typeID = 0; _typeID < _sig.size(); ++_typeID)
		{
			if (!_sig.test(_typeID)) continue;

			auto _compTypeID = static_cast<ECS::ComponentTypeID>(_typeID);
			const auto& _metaData = _pWorld->GetComponentMetaData(_compTypeID);

			if (ImGui::TreeNodeEx(_metaData.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				// コンポーネントごとの編集UI(エンティティインスペクタと同じ edit 関数を使う)
				_compEditContext.pData = _pPrefab->RefData(_compTypeID);
				auto _func = _pWorld->GetCompFunc(_compTypeID).edit;
				if (_func && _compEditContext.pData)
				{
					_func(_compEditContext);
				}

				if (ImGui::Button("RemoveComponent"))
				{
					_removeTypeID = _compTypeID;
				}

				ImGui::TreePop();
			}
		}

		if (_removeTypeID != ECS::Limits::INVALID_COMPONENTTYPEID)
		{
			_pPrefab->RemoveComponent(_removeTypeID);
		}

		// ---- コンポーネントの追加 ----
		if (ImGui::BeginCombo("Add Component", "Select..."))
		{
			for (auto& [_compTypeID, _meta] : _pWorld->GetAllComponentMetaData())
			{
				// すでに持っていたら出さない
				if (_sig.test(_compTypeID)) continue;

				if (ImGui::Selectable(_meta.name.c_str()))
				{
					_pPrefab->AddComponentDefault(_pWorld, _compTypeID);
				}
			}
			ImGui::EndCombo();
		}
	}
}
