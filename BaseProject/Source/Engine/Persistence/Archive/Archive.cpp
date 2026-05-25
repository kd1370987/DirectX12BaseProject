#include "Archive.h"
namespace Engine::Persistence
{
	Archive::Archive(Mode a_mode, const std::string& a_fileDir, const std::string& a_fileName, const std::string& a_ext)
	{
		m_fileDir = a_fileDir;
		m_mode = a_mode;
		m_binPath = a_fileDir + "/" + a_fileName + ".ob" + a_ext;
		m_jsonPath = a_fileDir + "/" + a_fileName + ".oj" + a_ext;

		switch (a_mode)
		{
		case Engine::Persistence::Archive::Mode::Save:
			// 本番時データ
			m_ofs.open(m_binPath, std::ios::binary);
			if (!m_ofs.is_open())
			{
				Editor::MainEditor::Instance().ErrorLog(
					"アーカイブが開けませんでした : %s", m_binPath.c_str());
			}
			// テスト時データ
			m_json = nlohmann::json::object();
			break;
		case Engine::Persistence::Archive::Mode::Load:
			// テスト時データの読み込み
			#ifdef _DEBUG
			{
				std::ifstream _ifs(m_jsonPath);
				if (_ifs.is_open())
				{
					_ifs >> m_json;
				}
				else
				{
					Editor::MainEditor::Instance().ErrorLog(a_fileDir.c_str());
				}
			}
			#else
			{
				// 本番時データ用
				m_ifs.open(m_binPath, std::ios::binary);
			}
			#endif
			
			break;
		default:
			break;
		}
	}

	Archive::~Archive()
	{
		// ファイルを閉じる
		if (m_ofs.is_open())
		{
			m_ofs.close();
		}
		if (m_ifs.is_open())
		{
			m_ifs.close();
		}

		// JSONの書き出し
#ifdef _DEBUG
		if (IsSaving())
		{
			std::ofstream _ofs(m_jsonPath);
			_ofs << m_json.dump(4);
		}
#endif
	}
	void Archive::StringField(const std::string& a_name, std::string& a_data)
	{
		// 保存処理
		if (IsSaving())
		{
			// テスト時データ
			m_json[a_name] = a_data;
			// 本番時(バイナリ)
			if (m_ofs.is_open())
			{
				BinaryHelper::WriteString(m_ofs, a_data);
			}
		}
		// 読み込み処理
		else
		{
			// テスト時
			if (m_json.contains(a_name))
			{
				a_data = m_json[a_name].get<std::string>();
			}
			// 本番時
			if (m_ifs.is_open())
			{
				a_data = BinaryHelper::ReadString(m_ifs);
			}
		}
	}
	void Archive::GUIDField(const std::string & a_name, Engine::GUID & a_guid)
	{
		// 保存処理
		if (IsSaving())
		{
			// テスト時データ
			m_json[a_name] = a_guid.String();
			// 本番時(バイナリ)
			if (m_ofs.is_open())
			{
				// value == UUID
				BinaryHelper::Write(m_ofs, a_guid.value);
			}
		}
		// 読み込み処理
		else
		{
			// テスト時
			if (m_json.contains(a_name))
			{
				a_guid.FromString(m_json[a_name].get<std::string>());
			}
			// 本番時
			if (m_ifs.is_open())
			{
				BinaryHelper::Read(m_ifs,a_guid.value);
			}
		}
	}
	void Archive::GUIDVectorField(const std::string & a_name, std::vector<Engine::GUID>&a_guids)
	{
		// 保存処理
		if (IsSaving())
		{
			// 要素数の書き込み
			size_t _size = a_guids.size();
			Field(a_name + "_size", _size);

			// 各要素のシリアライズ
			for (size_t _i = 0; _i < _size; ++_i)
			{
				GUIDField(a_name + "[" + std::to_string(_i) + "]", a_guids[_i]);
			}
		}
		// 読み込み
		else
		{
			// 要素数でリサイズ
			size_t _size = 0;
			Field(a_name + "_size", _size);
			a_guids.resize(_size);

			for (size_t _i = 0; _i < _size; ++_i)
			{
				GUIDField(a_name + "[" + std::to_string(_i) + "]", a_guids[_i]);
			}
		}
	}
}