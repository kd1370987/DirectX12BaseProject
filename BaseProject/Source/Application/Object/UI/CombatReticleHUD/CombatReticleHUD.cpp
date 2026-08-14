#include "CombatReticleHUD.h"

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
		// GUID未設定(新規追加)ならデフォルトのテスト用テクスチャを引く。
		// シーン読み込み時は Archive で復元済みのGUIDを尊重する。
		if (!m_texGUID.IsValid())
		{
			m_texGUID = Engine::Resource::AssetDatabase::Instance().GetGUIDFromFilePath(RETICLE_TEXTURE_PATH);
		}
		if (!m_texGUID.IsValid()) return;
		m_texRef = Engine::Resource::ResourceManager::Instance().LoadImmediate<Engine::Resource::Texture>(m_texGUID);
	}

	void CombatReticleHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
	}

}
