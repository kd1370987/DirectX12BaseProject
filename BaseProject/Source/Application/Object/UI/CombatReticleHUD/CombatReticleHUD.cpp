#include "CombatReticleHUD.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"

namespace App::Object
{
	namespace
	{
		// テスト用レティクルテクスチャのパス
		constexpr const char* RETICLE_TEXTURE_PATH = "Asset/Texture/Test/uiTest.png";
	}

	void CombatReticleHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		// パスからGUIDを引いてテクスチャを読み込む
		Engine::GUID _guid = Engine::Resource::AssetDatabase::Instance().GetGUIDFromFilePath(RETICLE_TEXTURE_PATH);
		m_reticleTexRef = Engine::Resource::ResourceManager::Instance().Load<Engine::Resource::Texture>(_guid);
	}

	void CombatReticleHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// テスト段階のため更新処理は無し
	}

	void CombatReticleHUD::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;

		// 画面中央にレティクルを描画命令として積む。
		// スクリーン矩形はNDC半径指定(ベースクアッドが-1〜1のため、size分だけ広がる)。
		_pGE->SubmitUI(
			m_reticleTexRef,	// ResourceRef -> Handle への暗黙変換
			m_screenPos,
			m_size,
			Engine::Color::WHITE
		);
	}
}
