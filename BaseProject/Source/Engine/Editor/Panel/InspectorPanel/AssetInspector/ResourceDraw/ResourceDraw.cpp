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
#include "AudioBehaviorEdit/AudioBehaviorEdit.h"
#include "EffectAssetEdit/EffectAssetEdit.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/RenderingPipeline/IO/RenderingPipelineAssetIO.h"

#include "../../../../../Resource/Data/Model/IO/ModelConverter/ModelConverter.h"

// プレハブ編集用(ECSのエンティティインスペクタと同じ構成で描く)
#include "Engine/ECS/World/World.h"
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

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
					Resource::ResourceManager::Instance().LoadImmediate<TResource>(a_guid);
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

		ParticleEdit(_pParticles, &a_editContext);
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
	// オーディオビヘイビア
	//-----------------------------------------------------------------------------------------
	void AudioBehaviorDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pBehavior = ResolveAsset<Resource::AudioBehavior>(_guid);
		if (!_pBehavior) { return; }

		AudioBehaviorEdit(a_editContext, _pBehavior);
	}

	//-----------------------------------------------------------------------------------------
	// エフェクト
	//-----------------------------------------------------------------------------------------
	void EffectAssetDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pEffect = ResolveAsset<Resource::EffectAsset>(_guid);
		if (!_pEffect) { return; }

		EffectAssetEdit(_guid, _pEffect, true, &a_editContext);
	}

	//-----------------------------------------------------------------------------------------
	// レンダリングパイプライン
	// ノードエディタはアセット自身が持っているので、ここは呼び出しとセーブだけ
	//-----------------------------------------------------------------------------------------
	void RenderingPipelineDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		auto* _pPipeline = ResolveAsset<Graphics::Pipeline::RenderingPipelineAsset>(_guid);
		if (!_pPipeline) { return; }

		// ロード直後はレジストリが入っていないことがあるので、ここで必ず通しておく
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		_pPipeline->SetMetaRegistry(_pGE ? _pGE->RefPassMetaRegistry() : nullptr);

		if (ImGui::Button("Save"))
		{
			_pPipeline->Save(a_editContext.pAssetProp->filePath);
		}

		// 画面が出ているかどうか。
		// 組めていないと画面は真っ黒になるので、ここで分かるようにしておく
		if (_pGE)
		{
			ImGui::SameLine();
			if (_pGE->IsPipelinePresentActive())	ImGui::TextDisabled("| 画面 : 出力中");
			else								ImGui::TextDisabled("| 画面 : 出ていません");
		}
		ImGui::Separator();

		_pPipeline->DrawEditor();
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

		//------------------------------------------------------------------
		// シグネチャのコピー / ペースト
		//
		// 別のプレハブへコンポーネント構成をそのまま持っていくためのもの。
		// 初期値のバイト列ごと運ぶので、貼り付け先は編集済みの値まで引き継ぐ。
		// 貼り付けはメモリ上のプレハブを書き換えるだけなので、
		// 確定させるには Save を押すこと。
		//------------------------------------------------------------------
		auto& _clipboard = a_editContext.prefabClipboard;

		ImGui::SameLine();
		if (ImGui::Button("Copy Signature"))
		{
			_clipboard.signature = _pPrefab->GetSignature();
			_clipboard.dataMap = _pPrefab->GetDataMap();
			_clipboard.isValid = true;

			ENGINE_LOG("Copy Prefab Signature : %d components", static_cast<int>(_clipboard.GetCount()));
		}

		// コピー前は貼り付けられない
		ImGui::SameLine();
		ImGui::BeginDisabled(!_clipboard.isValid);
		if (ImGui::Button("Paste Signature"))
		{
			_pPrefab->PasteSignatureAndData(_clipboard.signature, _clipboard.dataMap);
			ENGINE_LOG("Paste Prefab Signature : %d components", static_cast<int>(_clipboard.GetCount()));
		}
		ImGui::EndDisabled();

		// クリップボードの中身を出しておく : 何を貼るのか押す前に分かるようにする
		if (_clipboard.isValid)
		{
			ImGui::TextDisabled("Clipboard : %d components", static_cast<int>(_clipboard.GetCount()));
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				for (size_t _typeID = 0; _typeID < _clipboard.signature.size(); ++_typeID)
				{
					if (!_clipboard.signature.test(_typeID)) continue;

					const auto& _meta = _pWorld->GetComponentMetaData(static_cast<ECS::ComponentTypeID>(_typeID));
					ImGui::Text("%s", _meta.name.c_str());
				}
				ImGui::EndTooltip();
			}
		}
		else
		{
			ImGui::TextDisabled("Clipboard : empty");
		}

		ImGui::Separator();

		//------------------------------------------------------------------
		// 一緒に覚えている子エンティティ
		//------------------------------------------------------------------
		// ここで編集はできない(このインスペクタはルートの構成を触る場所)。
		// 実体化すると、この一覧ぶんが親子リンク付きで一緒に生成される。
		//------------------------------------------------------------------
		const auto& _childVec = _pPrefab->GetChildren();
		if (_childVec.empty())
		{
			ImGui::TextDisabled("Children : none");
		}
		else
		{
			ImGui::Text("Children : %d", static_cast<int>(_childVec.size()));

			if (ImGui::TreeNode("Children List"))
			{
				for (size_t _i = 0; _i < _childVec.size(); ++_i)
				{
					const auto& _child = _childVec[_i];

					// 親の位置(-1 はルート直下)
					const std::string _parentLabel = (_child.parentIndex < 0)
						? std::string("Root")
						: ("Child " + std::to_string(_child.parentIndex));

					// コンポーネント数だけ出しておけば、空でないことは分かる
					ImGui::BulletText("Child %d : parent = %s (%d components)",
						static_cast<int>(_i),
						_parentLabel.c_str(),
						static_cast<int>(_child.sig.count()));
				}
				ImGui::TreePop();
			}
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

				if (Engine::Editor::EditorHelper::DeleteButton("RemoveComponent"))
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
			// 数が増えると探せなくなるので名前で絞り込めるようにする
			const std::string& _search = EditorHelper::DrawSearchBox();

			for (auto& [_compTypeID, _meta] : _pWorld->GetAllComponentMetaData())
			{
				// すでに持っていたら出さない
				if (_sig.test(_compTypeID)) continue;

				if (!EditorHelper::IsMatchSearch(_search, _meta.name)) continue;

				// 登録名が同じコンポーネントがあってもImGuiのIDがぶつからないようにする
				// (Selectable のIDはラベル文字列から作られるため)
				ImGui::PushID(static_cast<int>(_compTypeID));

				if (ImGui::Selectable(_meta.name.c_str()))
				{
					_pPrefab->AddComponentDefault(_pWorld, _compTypeID);
				}

				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
	}
}
