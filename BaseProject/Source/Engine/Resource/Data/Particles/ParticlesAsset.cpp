#include "ParticlesAsset.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	void Engine::Resource::ParticlesAsset::Create(const std::string& a_name, const Engine::GUID& a_guid)
	{
		m_name = a_name;
		m_guid = a_guid;
	}
	void ParticlesAsset::Release()
	{
		m_name = "";
		m_guid = {};
		m_texGUID = {};
	}
	void ParticlesAsset::Save(const std::string & a_filePath)
	{
		// アーカイブ作成
		auto _fileDir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		Persistence::Archive _archi(Persistence::Archive::Mode::Save,_fileDir,_fileName,"ptic");

		// 保存
		_archi.Field("m_name",m_name);
		_archi.Field("m_guid",m_guid);
		_archi.Field("m_texGUID", m_texGUID);
		_archi.Field("m_initialSpeedMin", m_initialSpeedMin);
		_archi.Field("m_initialSpeedMax", m_initialSpeedMax);
		_archi.Field("m_gravityPow", m_gravityPow);
		_archi.Field("m_ligeTimeMin",m_lifeTimeMin);
		_archi.Field("m_ligeTimeMax",m_lifeTimeMax);
		_archi.Field("m_capacity", m_capacity);
		_archi.Field("m_emissionRate", m_emissionRate);

		// ※ 追加は末尾に。バイナリは順次読みなので途中に挿すと既存データが全部ずれる
		_archi.Field("m_orientation", m_orientation);
		_archi.Field("m_stretch", m_stretch);
	}
	void ParticlesAsset::Load(const std::string & a_fileDir, const std::string & a_fileName)
	{
		// アーカイブ作成
		Persistence::Archive _archi(Persistence::Archive::Mode::Load, a_fileDir, a_fileName, "ptic");

		// 読み込み
		_archi.Field("m_name", m_name);
		_archi.Field("m_guid", m_guid);
		_archi.Field("m_texGUID", m_texGUID);
		_archi.Field("m_initialSpeedMin", m_initialSpeedMin);
		_archi.Field("m_initialSpeedMax", m_initialSpeedMax);
		_archi.Field("m_gravityPow", m_gravityPow);
		_archi.Field("m_ligeTimeMin", m_lifeTimeMin);
		_archi.Field("m_ligeTimeMax", m_lifeTimeMax);
		_archi.Field("m_capacity", m_capacity);
		_archi.Field("m_emissionRate", m_emissionRate);

		// ※ 追加は末尾に。バイナリは順次読みなので途中に挿すと既存データが全部ずれる
		_archi.Field("m_orientation", m_orientation);
		_archi.Field("m_stretch", m_stretch);

		// キャパシティが0だとリソース生成ができないので最低値を入れておく
		if (m_capacity == 0)
		{
			m_capacity = 1000;
		}

		// 伸ばし倍率が0だと板が潰れて見えなくなるので最低値を入れておく
		if (m_stretch <= 0.0f)
		{
			m_stretch = 1.0f;
		}

		// テクスチャのハンドル取得
		m_texHandle = ResourceManager::Instance().LoadImmediate<Texture>(m_texGUID);
	}
	void ParticlesAsset::Load(const std::string& a_filePath)
	{
		// アーカイブ作成
		auto _fileDir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		Persistence::Archive _archi(Persistence::Archive::Mode::Load, _fileDir, _fileName, "ptic");

		// 読み込み
		_archi.Field("m_name", m_name);
		_archi.Field("m_guid", m_guid);
		_archi.Field("m_texGUID", m_texGUID);
		_archi.Field("m_initialSpeedMin", m_initialSpeedMin);
		_archi.Field("m_initialSpeedMax", m_initialSpeedMax);
		_archi.Field("m_gravityPow", m_gravityPow);
		_archi.Field("m_ligeTimeMin", m_lifeTimeMin);
		_archi.Field("m_ligeTimeMax", m_lifeTimeMax);
		_archi.Field("m_capacity", m_capacity);
		_archi.Field("m_emissionRate", m_emissionRate);

		// ※ 追加は末尾に。バイナリは順次読みなので途中に挿すと既存データが全部ずれる
		_archi.Field("m_orientation", m_orientation);
		_archi.Field("m_stretch", m_stretch);

		// キャパシティが0だとリソース生成ができないので最低値を入れておく
		if (m_capacity == 0)
		{
			m_capacity = 1000;
		}

		// 伸ばし倍率が0だと板が潰れて見えなくなるので最低値を入れておく
		if (m_stretch <= 0.0f)
		{
			m_stretch = 1.0f;
		}

		// テクスチャのハンドル取得
		m_texHandle = ResourceManager::Instance().LoadImmediate<Texture>(m_texGUID);
	}
}