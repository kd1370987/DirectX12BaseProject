#include "TextureView.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Resource/Loader/Texture/TextureLoader.h"
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../../Option/OptionManager.h"

#include "../../ImGui/ImGuiHelper/ImGuiHelper.h"

namespace Engine::Editor
{
	void TextureInspecter(const Engine::GUID& a_guid)
	{
		// ウィンドウサイズ取得
		auto _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		float _winWidth = _winOp.windowWidth;
		float _winHeight = _winOp.windowHegiht;

		// テクスチャハンドル取得
		auto _handle = Resource::TextureLoader::GetHandle(a_guid);
		auto _fileName = Resource::AssetDatabase::Instance().GetFileNameFromGUID(a_guid);

		// 名前と、GUID表示
		ImGui::Text("%s", _fileName.c_str());
		ImGui::Text("%s", a_guid.String().c_str());

		ImGui::Separator();

		// テクスチャハンドルがデフォルト値でない場合
		// 読み込めている
		if (_handle != Handle<Resource::Texture>())
		{
			// テクスチャ表示
			auto _pTex = Engine::Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pTex) return;
			auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
			Helper::DrawSRVView(_gpuHandle,_winWidth,_winHeight);
		}
		// 読み込めていない
		else
		{
			// ロード
			if (ImGui::Button("Load"))
			{
				//Resource::TextureLoader::Load(a_guid);
				Resource::ResourceManager::Instance().Load<Resource::Texture>(a_guid);
			}
		}
	}
}