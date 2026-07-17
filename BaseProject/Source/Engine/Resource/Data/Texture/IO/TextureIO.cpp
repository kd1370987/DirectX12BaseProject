#include "TextureIO.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Resource
{
	Handle<Texture> TextureIO::Create(const TextureCreateDesc& a_initData)
	{
		// テクスチャ作成
		Texture _tex;
		_tex.Create(a_initData);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_tex));

		ENGINE_LOG("テクスチャが作成されました");

		return _handle;
	}

	Texture TextureIO::LoadFromFile(const std::string& a_path)
	{
		Texture _tex = {};
		_tex.Import(a_path);
		return _tex;
	}
	Texture TextureIO::CreateColorTexture(const DXSM::Color& a_color)
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
		_tex.Create(_name, a_color);

		return _tex;
	}
	Engine::GUID TextureIO::GetColorGUID(const DXSM::Color& a_color)
	{
		if (a_color == TexColor::WHITE)  return Engine::GUID(WHITE_TEXTURE_GUIDSTR);
		if (a_color == TexColor::BLACK)  return Engine::GUID(BLACK_TEXTURE_GUIDSTR);
		if (a_color == TexColor::NORMAL) return Engine::GUID(NORMAL_TEXTURE_GUIDSTR);
		if (a_color == TexColor::ORM)    return Engine::GUID(ORM_TEXTURE_GUIDSTR);

		// 未知の色の場合は適当なハッシュをGUIDにするなどの処理
		return Engine::GUID();
	}

	Handle<Texture> TextureIO::LoadTexture(const Engine::GUID& a_guid, const DXSM::Color& a_defaultColor)
	{
		// AssetDatabaseに存在する有効なGUIDなら、統合ロード処理へ投げる
		if (AssetDatabase::Instance().IsValid(a_guid))
		{
			return ResourceManager::Instance().Load<Texture>(a_guid);
		}

		// ---- 無効なGUIDフォールバック処理 ----
		// 色に対応する専用のGUIDを取得
		Engine::GUID _colorGuid = GetColorGUID(a_defaultColor);

		// すでに同じ色のテクスチャが作られていないかキャッシュをチェック
		Handle<Texture> _handle = ResourceManager::Instance().GetCache<Texture>(_colorGuid);

		if (_handle == Handle<Texture>())
		{
			// まだ誰もこの色のテクスチャを作っていなければ、実体を生成
			Texture _newTex = CreateColorTexture(a_defaultColor);

			// 生成した実体を、色専用のGUIDと一緒にResourceManagerに登録する
			_handle = ResourceManager::Instance().AddResourceAndGUID(std::move(_newTex), _colorGuid);
		}

		return _handle;
	}
}