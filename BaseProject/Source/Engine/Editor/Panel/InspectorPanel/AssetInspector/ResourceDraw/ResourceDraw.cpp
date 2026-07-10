#include "ResourceDraw.h"

#include "../../../../../Option/OptionManager.h"
#include "../../../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// モデル
	//-----------------------------------------------------------------------------------------
	void ModelDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;
		if (Resource::ResourceManager::Instance().Has<Resource::Model>(_guid))
		{
			auto _handle = Resource::ResourceManager::Instance().GetCache<Resource::Model>(_guid);
			auto* _pModel = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pModel)
			{
				ImGui::Text("Not faund model");
				return;
			}
			else
			{
				if (ImGui::Button("Convert"))
				{
					auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
					_pModel->Save(_path);
				}
			}
		}
		else
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::Model>(_guid);
			}
		}
	}

	//-----------------------------------------------------------------------------------------
	// テクスチャ
	//-----------------------------------------------------------------------------------------
	void TextureDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;

		// ウィンドウサイズ取得
		auto _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		float _winWidth = _winOp.windowWidth;
		float _winHeight = _winOp.windowHegiht;

		// テクスチャハンドル取得
		auto _handle = Resource::ResourceManager::Instance().GetCache<Resource::Texture>(_guid);
		auto _fileName = Resource::AssetDatabase::Instance().GetFileNameFromGUID(_guid);

		// 名前と、GUID表示
		ImGui::Text("%s", _fileName.c_str());
		ImGui::Text("%s", _guid.String().c_str());

		ImGui::Separator();

		// テクスチャハンドルがデフォルト値でない場合
		// 読み込めている
		if (_handle != Handle<Resource::Texture>())
		{
			// テクスチャ表示
			auto _pTex = Engine::Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pTex) return;
			auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
			Helper::DrawSRVView(_gpuHandle, _winWidth, _winHeight);
		}
		// 読み込めていない
		else
		{
			// ロード
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::Texture>(_guid);
			}
		}
	}
	//-----------------------------------------------------------------------------------------
	// ステートマシン
	//-----------------------------------------------------------------------------------------
	void StateMachineDraw(EditorContext& a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;
		if (Resource::ResourceManager::Instance().Has<Resource::StateMachineAsset>(_guid))
		{
			auto _handle = Resource::ResourceManager::Instance().Load<Resource::StateMachineAsset>(_guid);
			auto* _pMachin = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pMachin)
			{
				ImGui::Text("Not faund state machin");
				return;
			}
			_pMachin->EditImGui(_handle);
		}
		else
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::StateMachineAsset>(_guid);
			}
		}
	}
	//-----------------------------------------------------------------------------------------
	// パーティクル
	//-----------------------------------------------------------------------------------------
	void ParticleDraw(EditorContext & a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;
		if (Resource::ResourceManager::Instance().Has<Resource::ParticlesAsset>(_guid))
		{
			auto _handle = Resource::ResourceManager::Instance().Load<Resource::ParticlesAsset>(_guid);
			auto* _pData = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pData)
			{
				ImGui::Text("Not faund particle");
				return;
			}
			_pData->EditImGui();
		}
		else
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::ParticlesAsset>(_guid);
			}
		}
	}
	//-----------------------------------------------------------------------------------------
	// マテリアル
	//-----------------------------------------------------------------------------------------
	void MaterialDraw(EditorContext & a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;
		if (Resource::ResourceManager::Instance().Has<Resource::Material>(_guid))
		{
			auto _handle = Resource::ResourceManager::Instance().Load<Resource::Material>(_guid);
			auto* _pData = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pData)
			{
				ImGui::Text("Not faund particle");
				return;
			}
			_pData->Edit(_guid);
		}
		else
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::Material>(_guid);
			}
		}
	}
	//-----------------------------------------------------------------------------------------
	// メッシュ
	//-----------------------------------------------------------------------------------------
	void MeshDraw(EditorContext & a_editContext)
	{}
	//-----------------------------------------------------------------------------------------
	// アニメーション
	//-----------------------------------------------------------------------------------------
	void AnimationDraw(EditorContext & a_editContext)
	{}
	//-----------------------------------------------------------------------------------------
	// シェーダー
	//-----------------------------------------------------------------------------------------
	void ShaderDraw(EditorContext & a_editContext)
	{}
	//-----------------------------------------------------------------------------------------
	// シェーディングモデルテーブル
	//-----------------------------------------------------------------------------------------
	void ShadingModelTableDraw(EditorContext & a_editContext)
	{
		auto _guid = a_editContext.pAssetProp->guid;
		if (Resource::ResourceManager::Instance().Has<Resource::ShadingModelTable>(_guid))
		{
			auto _handle = Resource::ResourceManager::Instance().Load<Resource::ShadingModelTable>(_guid);
			auto* _pData = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pData)
			{
				ImGui::Text("Not faund particle");
				return;
			}
			_pData->Edit(_guid);
		}
		else
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::ShadingModelTable>(_guid);
			}
		}
	}
}