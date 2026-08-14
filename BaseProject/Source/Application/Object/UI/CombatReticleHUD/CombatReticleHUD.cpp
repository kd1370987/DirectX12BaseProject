#include "CombatReticleHUD.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"
#include "Engine/Option/OptionManager.h"	// ウィンドウ解像度(px)取得用

namespace App::Object
{
	namespace
	{
		// テスト用レティクルテクスチャのパス
		constexpr const char* RETICLE_TEXTURE_PATH = "Asset/Texture/Test/uiTest.png";
	}

	void CombatReticleHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		// リソース周りはコンテキストが運んできたサービスを使う
		if (!a_context.pServices) return;
		if (!a_context.pServices->pAssetDatabase || !a_context.pServices->pResourceManager) return;

		// GUID未設定(新規追加)ならデフォルトのテスト用テクスチャを引く。
		// シーン読み込み時は Archive で復元済みのGUIDを尊重する。
		if (!m_texGUID.IsValid())
		{
			m_texGUID = a_context.pServices->pAssetDatabase->GetGUIDFromFilePath(RETICLE_TEXTURE_PATH);
		}
		if (!m_texGUID.IsValid()) return;

		// 実体の到着は待たない。描画側が IsReady を見てスキップする
		m_texRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_texGUID);
	}

	void CombatReticleHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
	}

}
