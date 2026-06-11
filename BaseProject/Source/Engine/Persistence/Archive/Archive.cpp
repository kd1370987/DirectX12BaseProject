#include "Archive.h"

#include "../../MainEngine.h"

namespace Engine::Persistence
{
	Archive::Archive(Mode a_mode, const std::string& a_fileDir, const std::string& a_fileName, const std::string& a_ext, ArchiveFormat a_format)
	{
		m_fileDir = a_fileDir;
		m_mode = a_mode;
		m_binPath = a_fileDir + "/" + a_fileName + ".ob" + a_ext;
		m_jsonPath = a_fileDir + "/" + a_fileName + ".oj" + a_ext;

		switch (a_mode)
		{
		case Engine::Persistence::Archive::Mode::Save:
			// 親ディレクトリの作成
			std::filesystem::create_directories(m_fileDir);

			// Save時は、Autoなら両方（今まで通り）、指定があればそのフォーマットだけ準備する
			if (a_format == ArchiveFormat::Auto || a_format == ArchiveFormat::Binary)
			{
				m_ofs.open(m_binPath, std::ios::binary);
				if (!m_ofs.is_open())
				{
					Editor::MainEditor::Instance().ErrorLog(
						"アーカイブが開けませんでした : %s", m_binPath.c_str());
				}
			}

			if (a_format == ArchiveFormat::Auto || a_format == ArchiveFormat::Json)
			{
				m_json = nlohmann::json::object();
			}
			break;

		case Engine::Persistence::Archive::Mode::Load:
		{
			// ロードするフォーマットを決定
			bool _loadJson = false;
			bool _loadBin = false;

			if (a_format == ArchiveFormat::Json)
			{
				_loadJson = true; // 手動でJSON指定
			}
			else if (a_format == ArchiveFormat::Binary)
			{
				_loadBin = true; // 手動でバイナリ指定
			}
			else // Autoの場合（今まで通りのビルドモード判定）
			{
				auto _buildMode = MainEngine::Instance().GetEngineConfig().GetInitConfig().buildMode;
				if (_buildMode == Engine::EBuildConfiguration::Development)
				{
					_loadJson = true;
				}
				else if (_buildMode == Engine::EBuildConfiguration::Shipping)
				{
					_loadBin = true;
				}
			}

			// 決定したフォーマットに基づいて読み込み処理
			if (_loadJson)
			{
				std::ifstream _ifs(m_jsonPath);
				if (_ifs.is_open())
				{
					_ifs >> m_json;
				}
				else
				{
					// ※ついでにエラーログの内容を少し親切（m_jsonPathを出力）に直しています
					Editor::MainEditor::Instance().ErrorLog("JSONが見つかりませんでした : %s", m_jsonPath.c_str());
				}
			}
			else if (_loadBin)
			{
				m_ifs.open(m_binPath, std::ios::binary);
				if (!m_ifs.is_open())
				{
					Editor::MainEditor::Instance().ErrorLog("バイナリが開けませんでした : %s", m_binPath.c_str());
				}
			}
			break;
		}
		default:
			break;
		}
	}

	Archive::Archive(Mode a_mode, const std::string& a_filePath, const std::string& a_ext, ArchiveFormat a_format)
	{
		auto _dir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		auto _ext = FileUtility::GetFilePathExtension(a_filePath);
		Archive(a_mode, _dir, _fileName, a_ext, a_format);
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