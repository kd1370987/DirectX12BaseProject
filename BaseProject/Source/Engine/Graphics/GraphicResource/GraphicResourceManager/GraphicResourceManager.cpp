#include "GraphicResourceManager.h"

#include "../Loader/TextureLoader/TextureLoader.h"

void GraphicResourceManager::Init()
{

	m_texStorage.Init(100);
}

void GraphicResourceManager::Release()
{
	m_texStorage.Clear();
}

bool GraphicResourceManager::GetTexture(Engine::Resource::ID& a_outID, const std::string& a_dir, const std::string& a_key, TextureUse a_use)
{
	ImGuiContex::Instance().AddLog("GetTexture: %s\n", (a_dir + a_key).c_str());

	if (a_key == "")
	{
		switch (a_use)
		{
		case TextureUse::Albedo:
			a_outID = m_texStorage.Add(a_dir + "Albedo", TextureLoad::White());
			return false;
			break;
		case TextureUse::MetallicRoughness:
			a_outID = m_texStorage.Add(a_dir + "MetaRoug", TextureLoad::ORM());
			return false;
		case TextureUse::Emissive:
			a_outID = m_texStorage.Add(a_dir + "Emissive", TextureLoad::Black());
			return false;
		case TextureUse::Normal:
			a_outID =  m_texStorage.Add(a_dir + "Normal", TextureLoad::NormalWhite());
			return false;

		default:
			break;
		}
	}

	std::string _fullPath = a_dir + a_key;

	if (!m_texStorage.Has(_fullPath))
	{
		TextureS _tex = {};
		if (!TextureLoad::Load(_fullPath, _tex))
		{
			switch (a_use)
			{
			case TextureUse::Albedo:
				_tex = TextureLoad::White();
				break;
			case TextureUse::MetallicRoughness:
				_tex = TextureLoad::ORM();
				break;
			case TextureUse::Emissive:
				_tex = TextureLoad::Black();
				break;
			case TextureUse::Normal:
				_tex = TextureLoad::NormalWhite();
				break;
			default:
				break;
			}
		}
		a_outID = m_texStorage.Add(_fullPath, _tex);
		return true;
	}
	else
	{
		a_outID = m_texStorage.GetID(_fullPath);
		return true;
	}
}

const TextureS* GraphicResourceManager::NGetTexture(const uint32_t& a_texID)
{
	return m_texStorage.Get(a_texID);
}

const std::string& GraphicResourceManager::GetTexturePath(const uint32_t& a_texID)
{
	return m_texStorage.GetString(a_texID);
}

