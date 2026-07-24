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

		// アセット/シーンのセーブ・ロードを一元的にログ出力する。
		// シーンも各アセットもこの Archive を通るため、ここで出すことで
		// 「ログが出るものと出ないもの」のばらつきを無くす。
		ENGINE_LOG("[Archive] %s : %s (.%s)",
			(a_mode == Mode::Save) ? "セーブ" : "ロード",
			a_fileName.c_str(),
			a_ext.c_str());

		switch (a_mode)
		{
		case Engine::Persistence::Archive::Mode::Save:
			// 親ディレクトリの作成
			std::filesystem::create_directories(m_fileDir);

			if (a_format == ArchiveFormat::Auto || a_format == ArchiveFormat::Binary)
			{
				m_ofs.open(m_binPath, std::ios::binary);
				if (!m_ofs.is_open())
				{
					Editor::MainEditor::Instance().ErrorLog("Not open archive : %s", m_binPath.c_str());
				}
			}

			if (a_format == ArchiveFormat::Auto || a_format == ArchiveFormat::Json)
			{
				m_json = nlohmann::json::object(); // JSONモードを初期化
			}
			break;

		case Engine::Persistence::Archive::Mode::Load:
		{
			bool _loadJson = false;
			bool _loadBin = false;

			if (a_format == ArchiveFormat::Json) _loadJson = true;
			else if (a_format == ArchiveFormat::Binary) _loadBin = true;
			else
			{
				auto _buildMode = MainEngine::Instance().GetEngineConfig().GetInitConfig().buildMode;
				if (_buildMode == Engine::EBuildConfiguration::Development) _loadJson = true;
				else if (_buildMode == Engine::EBuildConfiguration::Shipping) _loadBin = true;
			}

			if (_loadJson)
			{
				std::ifstream _ifs(m_jsonPath);
				if (_ifs.is_open())
				{
					_ifs >> m_json;
				}
				else
				{
					Editor::MainEditor::Instance().ErrorLog("Not Faund Json : %s", m_jsonPath.c_str());
				}
			}
			else if (_loadBin)
			{
				m_ifs.open(m_binPath, std::ios::binary);
				if (!m_ifs.is_open())
				{
					Editor::MainEditor::Instance().ErrorLog("Not Found Binary : %s", m_binPath.c_str());
				}
			}
			break;
		}
		default:
			break;
		}
	}

	Archive::~Archive()
	{
		if (m_ofs.is_open()) m_ofs.close();
		if (m_ifs.is_open()) m_ifs.close();

		// JSONの書き出し
#ifdef _DEBUG
		if (IsSaving() && !m_json.is_null())
		{
			std::ofstream _ofs(m_jsonPath);
			_ofs << m_json.dump(4); // インデント付きで綺麗に出力
		}
#endif
	}
	void Archive::StringField(const std::string& a_name, std::string& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// Json処理
			if (!m_json.is_null()) CurrentNode()[a_name] = a_data;
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::WriteString(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// Json処理
			if (!m_json.is_null() && CurrentNode().contains(a_name))
			{
				a_data = CurrentNode()[a_name].get<std::string>();
			}
			// binary処理
			if (m_ifs.is_open()) a_data = BinaryHelper::ReadString(m_ifs);
		}
	}
	// =========================================================================
	// 階層・スコープ管理の実装
	// =========================================================================
	bool Archive::BeginGroup(const std::string& a_name)
	{
		bool _success = false;
		// セーブ時
		if (IsSaving())
		{
			// json処理
			if (!m_json.is_null())
			{
				CurrentNode()[a_name] = nlohmann::json::object(); // {} を作成
				m_jsonNodeStack.push(&CurrentNode()[a_name]);     // 潜る
				_success = true;
			}
			// binary処理
			if (m_ofs.is_open()) _success = true; // バイナリはそのまま進む
		}
		// ロード時
		else
		{
			// json処理
			if (!m_json.is_null())
			{
				if (CurrentNode().contains(a_name) && CurrentNode()[a_name].is_object())
				{
					m_jsonNodeStack.push(&CurrentNode()[a_name]);
					_success = true;
				}
			}
			// binary処理
			else if (m_ifs.is_open()) _success = true;
		}
		return _success;
	}
	void Archive::EndGroup()
	{
		// 階層を1つ上がる
		if (!m_jsonNodeStack.empty()) m_jsonNodeStack.pop();
	}
	bool Archive::BeginArray(const std::string& a_name, size_t& a_size)
	{
		bool _success = false;
		// セーブ時
		if (IsSaving())
		{
			// json処理
			if (!m_json.is_null())
			{
				if (CurrentNode().is_object()) // 安全対策
				{
					CurrentNode()[a_name] = nlohmann::json::array(); // [] を作成
					m_jsonNodeStack.push(&CurrentNode()[a_name]);
					_success = true;
				}
			}
			// binary処理
			if (m_ofs.is_open())
			{
				BinaryHelper::Write(m_ofs, a_size); // バイナリは要素数を書き込む
				_success = true;
			}
		}
		// ロード時
		else
		{
			// json処理
			if (!m_json.is_null())
			{
				// 安全対策: 現在のノードがObject({})であることを確認してからアクセス
				if (CurrentNode().is_object() && CurrentNode().contains(a_name) && CurrentNode()[a_name].is_array())
				{
					// ★順番が命！★
					a_size = CurrentNode()[a_name].size(); // 先にサイズを取得する！
					m_jsonNodeStack.push(&CurrentNode()[a_name]); // その後でスタックに潜る！
					_success = true;
				}
			}
			// binary処理
			else if (m_ifs.is_open())
			{
				BinaryHelper::Read(m_ifs, a_size); // バイナリから要素数を読み込む
				_success = true;
			}
		}
		return _success;
	}
	void Archive::EndArray()
	{
		if (!m_jsonNodeStack.empty()) m_jsonNodeStack.pop();
	}
	bool Archive::BeginObject(size_t a_index)
	{
		bool _success = false;
		if (IsSaving())
		{
			if (!m_json.is_null())
			{
				// 配列の中にオブジェクト {} を追加し、そこに潜る
				CurrentNode().push_back(nlohmann::json::object());
				m_jsonNodeStack.push(&CurrentNode().back());
				_success = true;
			}
			if (m_ofs.is_open()) _success = true;
		}
		else
		{
			if (!m_json.is_null())
			{
				// 現在のノードが配列であり、インデックスが範囲内なら潜る
				if (CurrentNode().is_array() && a_index < CurrentNode().size())
				{
					m_jsonNodeStack.push(&CurrentNode()[a_index]);
					_success = true;
				}
			}
			else if (m_ifs.is_open()) _success = true;
		}
		return _success;
	}
	void Archive::EndObject()
	{
		if (!m_jsonNodeStack.empty()) m_jsonNodeStack.pop();
	}
	void Archive::GUIDField(const std::string & a_name, Engine::GUID & a_guid)
	{
		// セーブ時
		if (IsSaving())
		{
			// Json処理
			if (!m_json.is_null()) CurrentNode()[a_name] = a_guid.String();
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_guid.value);
		}
		// ロード時
		else
		{
			// Json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				a_guid.FromString(CurrentNode()[a_name].get<std::string>());
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_guid.value);
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