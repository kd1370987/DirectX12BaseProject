#include "ResourceDraw.h"

#include "ModelEdit.h"
#include "MeshEdit.h"
#include "MaterialEdit.h"
#include "AnimationEdit.h"
#include "TextureEdit.h"
#include "ShaderEdit.h"
#include "ShadingModelTableEdit.h"
#include "ParticleEdit.h"
#include "AnimatorEdit.h"
#include "ActionStateMachineEdit.h"

#include "../../../../../Resource/Data/Model/IO/ModelConverter/ModelConverter.h"

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
}
