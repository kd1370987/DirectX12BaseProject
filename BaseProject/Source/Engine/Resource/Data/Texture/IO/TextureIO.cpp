#include "TextureIO.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"

#include "../../../Common/ScopedResourceBuild.h"

namespace Engine::Resource
{
	Handle<Texture> TextureIO::Create(const TextureCreateDesc& a_initData, const ResourceBuildContext* a_pContext)
	{
		// ビューの置き場はコンテキストから引く : 渡されていなければその場で開く
		ResourceBuildScope _scope(a_pContext);

		// テクスチャ作成
		Texture _tex;
		_tex.Create(_scope.GetContext().pHeapManager, a_initData);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_tex));

		ENGINE_LOG("テクスチャが作成されました");

		return _handle;
	}

	Texture TextureIO::LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext)
	{
		ResourceBuildScope _scope(a_pContext);

		Texture _tex = {};
		_tex.Import(_scope.GetContext(), a_path);
		return _tex;
	}
	Texture TextureIO::CreateColorTexture(const Math::Color& a_color, const ResourceBuildContext& a_ctx)
	{
		std::string _name = "ColorTex_";
		// カラーチェック
		if (a_color == TexColor::WHITE)
		{
			_name += "WHITE";
		}
		else if (a_color == TexColor::BLACK)
		{
			_name += "BLACK";
		}
		else if (a_color == TexColor::NORMAL)
		{
			_name += "NORMAL";
		}
		else if (a_color == TexColor::ORM)
		{
			_name += "ORM";

		}

		// テクスチャ作成
		Texture _tex;
		_tex.Create(a_ctx, _name, a_color);

		return _tex;
	}
	Engine::GUID TextureIO::GetColorGUID(const Math::Color& a_color)
	{
		if (a_color == TexColor::WHITE)  return Engine::GUID(WHITE_TEXTURE_GUIDSTR);
		if (a_color == TexColor::BLACK)  return Engine::GUID(BLACK_TEXTURE_GUIDSTR);
		if (a_color == TexColor::NORMAL) return Engine::GUID(NORMAL_TEXTURE_GUIDSTR);
		if (a_color == TexColor::ORM)    return Engine::GUID(ORM_TEXTURE_GUIDSTR);

		// 未知の色の場合は適当なハッシュをGUIDにするなどの処理
		return Engine::GUID();
	}

	Handle<Texture> TextureIO::LoadTexture(
		const Engine::GUID& a_guid,
		const Math::Color& a_defaultColor,
		const ResourceBuildContext* a_pContext
	)
	{
		// 参照するマネージャーはコンテキストから引く
		auto& _assetDb = (a_pContext && a_pContext->pAssetDatabase) ? *a_pContext->pAssetDatabase : AssetDatabase::Instance();
		auto& _resMgr = (a_pContext && a_pContext->pResourceManager) ? *a_pContext->pResourceManager : ResourceManager::Instance();

		// AssetDatabaseに存在する有効なGUIDなら、統合ロード処理へ投げる
		if (_assetDb.IsValid(a_guid))
		{
			return _resMgr.LoadImmediate<Texture>(a_guid, a_pContext);
		}

		// ---- 無効なGUIDフォールバック処理 ----
		// 色に対応する専用のGUIDを取得
		Engine::GUID _colorGuid = GetColorGUID(a_defaultColor);

		// すでに同じ色のテクスチャが作られていないかキャッシュをチェック
		Handle<Texture> _handle = _resMgr.GetCache<Texture>(_colorGuid);

		if (_handle == Handle<Texture>())
		{
			// まだ誰もこの色のテクスチャを作っていなければ、実体を生成
			ResourceBuildScope _scope(a_pContext);
			Texture _newTex = CreateColorTexture(a_defaultColor, _scope.GetContext());

			// 生成した実体を、色専用のGUIDと一緒にResourceManagerに登録する
			_handle = _resMgr.AddResourceAndGUID(std::move(_newTex), _colorGuid);
		}

		return _handle;
	}
}