#include "Archive.h"

// JSON の実体を触るのはこのファイルだけ。
// Archive.h はプリコンパイル済みヘッダー経由で全翻訳単位に乗るので、
// あちらでは JSONForward.h(前方宣言)しか読まない
#pragma warning(push, 0)
#include <nlohmannJSON/json.hpp>
#pragma warning(pop)

#include "../../MainEngine.h"

namespace Engine::Persistence
{
	Archive::Archive(Mode a_mode, const std::string& a_fileDir, const std::string& a_fileName, const std::string& a_ext, ArchiveFormat a_format)
	{
		// 中身は null のまま。JSON を使うかどうかは下の分岐で決まる
		m_upJson = std::make_unique<nlohmann::json>();

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
					ENGINE_ERROR("Not open archive : %s", m_binPath.c_str());
				}
			}

			if (a_format == ArchiveFormat::Auto || a_format == ArchiveFormat::Json)
			{
				*m_upJson = nlohmann::json::object(); // JSONモードを初期化
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
				// Auto : ビルドモードで決める(下の ShouldLoadJson を参照)
				_loadJson = ShouldLoadJson(a_ext);
				_loadBin  = !_loadJson;
			}

			if (_loadJson)
			{
				std::ifstream _ifs(m_jsonPath);
				if (_ifs.is_open())
				{
					_ifs >> *m_upJson;
				}
				else
				{
					ENGINE_ERROR("Not Faund Json : %s", m_jsonPath.c_str());
				}
			}
			else if (_loadBin)
			{
				m_ifs.open(m_binPath, std::ios::binary);
				if (!m_ifs.is_open())
				{
					ENGINE_ERROR("Not Found Binary : %s", m_binPath.c_str());
				}
			}
			break;
		}
		default:
			break;
		}
	}

	//======================================================================================
	// Auto のときの読み込み形式
	//
	// ・Shipping        : 必ずバイナリ(.ob)。JSON は開発中の編集用なので製品には持ち込まない。
	// ・Debug/Development : .oj があればそちらを優先する。
	//
	// JSON を優先するのは、バイナリが「フィールドを書いた順にそのまま並べるだけ」の
	// 形式で、キーを持たないため。コンポーネントやアセットにフィールドを1つ足すと
	// それ以降の読み出しが全部ずれ、保存済みの .ob* は作り直すまで使えなくなる。
	// JSON はキー付きなので、知らないキーは飛ばし、無いキーは既定値のまま残る。
	// 開発中はこちらを読んでおけば、フィールドを足しても既存データが壊れない。
	//
	// ただし重いデータ(モデル・メッシュ・アニメーション)は除く。中身は頂点や行列の
	// 羅列で、手で書き換えることも無いのに JSON にすると読み込みが桁違いに遅くなる。
	//
	// .oj が無ければバイナリへ落ちる。両方を書き出すのは Save 側(Auto)なので、
	// 片方しか無いのは「バイナリだけ配ったアセット」か「まだ保存し直していないもの」。
	//======================================================================================
	bool Archive::IsHeavyDataExtension(const std::string& a_ext)
	{
		return a_ext == "mdl" || a_ext == "mesh" || a_ext == "anim";
	}

	bool Archive::ShouldLoadJson(const std::string& a_ext) const
	{
		// 製品ビルドはバイナリのみ
		if (MainEngine::Instance().GetBuildMode() == EBuildConfiguration::Shipping) return false;

		// 重いデータはモードによらずバイナリ
		if (IsHeavyDataExtension(a_ext)) return false;

		// JSON が無ければバイナリへ
		std::error_code _ec;
		return std::filesystem::exists(m_jsonPath, _ec);
	}

	//======================================================================================
	// メモリ上のJSONだけを相手にするアーカイブ
	//
	// ファイルを開かないので、m_ofs / m_ifs はどちらも閉じたまま。
	// 各 Field はストリームが開いているかを見てから書くので、JSON側だけが動く
	//======================================================================================
	Archive::Archive(Mode a_mode, nlohmann::json& a_json)
	{
		m_upJson = std::make_unique<nlohmann::json>();

		m_mode = a_mode;
		m_format = ArchiveFormat::Json;
		m_isMemory = true;

		if (a_mode == Mode::Save)
		{
			m_pMemoryJson = &a_json;
			*m_upJson = nlohmann::json::object();
		}
		else
		{
			*m_upJson = a_json;
		}
	}

	Archive::~Archive()
	{
		// メモリ相手のときはファイルへ書き出さない
		if (m_isMemory)
		{
			if (IsSaving() && m_pMemoryJson) *m_pMemoryJson = std::move(*m_upJson);
			return;
		}

		if (m_ofs.is_open()) m_ofs.close();
		if (m_ifs.is_open()) m_ifs.close();

		// JSONの書き出し
		if (IsSaving() && !m_upJson->is_null())
		{
			std::ofstream _ofs(m_jsonPath);
			_ofs << m_upJson->dump(4); // インデント付きで綺麗に出力
		}
	}
	//======================================================================================
	// JSON への1フィールドの読み書き
	//
	// Archive.h のテンプレートから呼ばれる入口。
	// nlohmann::json に触るのをこのファイルだけに閉じ込めるために分けてある
	//======================================================================================
	bool Archive::HasJson() const
	{
		return m_upJson && !m_upJson->is_null();
	}

	nlohmann::json& Archive::CurrentNode()
	{
		return m_jsonNodeStack.empty() ? *m_upJson : *m_jsonNodeStack.top();
	}

	void Archive::JsonWriteBool(const std::string& a_name, bool a_value)
	{
		CurrentNode()[a_name] = a_value;
	}
	void Archive::JsonWriteInt(const std::string& a_name, int64_t a_value)
	{
		CurrentNode()[a_name] = a_value;
	}
	void Archive::JsonWriteUInt(const std::string& a_name, uint64_t a_value)
	{
		CurrentNode()[a_name] = a_value;
	}
	void Archive::JsonWriteFloat(const std::string& a_name, double a_value)
	{
		CurrentNode()[a_name] = a_value;
	}
	void Archive::JsonWriteString(const std::string& a_name, const std::string& a_value)
	{
		CurrentNode()[a_name] = a_value;
	}
	void Archive::JsonWriteFloats(const std::string& a_name, const float* a_pValues, size_t a_count)
	{
		// Vector や Matrix は要素を並べた配列で持つ(並びは従来の .oj* と同じ)
		nlohmann::json _array = nlohmann::json::array();
		for (size_t _i = 0; _i < a_count; ++_i)
		{
			_array.push_back(a_pValues[_i]);
		}
		CurrentNode()[a_name] = std::move(_array);
	}

	//--------------------------------------------------------------------------------------
	// 読み込み側
	//
	// 見つからない・型が合わないときは false を返し、呼び出し側の値には触らない。
	// (既定値のまま残す。フィールドを足したときに既存データが壊れないようにするため)
	//--------------------------------------------------------------------------------------
	namespace
	{
		// 名前で引いた値を返す。無ければ nullptr
		const nlohmann::json* FindField(nlohmann::json& a_node, const std::string& a_name)
		{
			if (!a_node.is_object()) return nullptr;

			auto _it = a_node.find(a_name);
			if (_it == a_node.end()) return nullptr;

			return &(*_it);
		}
	}

	bool Archive::JsonReadBool(const std::string& a_name, bool& a_outValue)
	{
		const nlohmann::json* _pValue = FindField(CurrentNode(), a_name);
		if (!_pValue || !_pValue->is_boolean()) return false;

		a_outValue = _pValue->get<bool>();
		return true;
	}
	bool Archive::JsonReadInt(const std::string& a_name, int64_t& a_outValue)
	{
		const nlohmann::json* _pValue = FindField(CurrentNode(), a_name);
		if (!_pValue || !_pValue->is_number()) return false;

		a_outValue = _pValue->get<int64_t>();
		return true;
	}
	bool Archive::JsonReadUInt(const std::string& a_name, uint64_t& a_outValue)
	{
		const nlohmann::json* _pValue = FindField(CurrentNode(), a_name);
		if (!_pValue || !_pValue->is_number()) return false;

		a_outValue = _pValue->get<uint64_t>();
		return true;
	}
	bool Archive::JsonReadFloat(const std::string& a_name, double& a_outValue)
	{
		const nlohmann::json* _pValue = FindField(CurrentNode(), a_name);
		if (!_pValue || !_pValue->is_number()) return false;

		a_outValue = _pValue->get<double>();
		return true;
	}
	bool Archive::JsonReadString(const std::string& a_name, std::string& a_outValue)
	{
		const nlohmann::json* _pValue = FindField(CurrentNode(), a_name);
		if (!_pValue || !_pValue->is_string()) return false;

		a_outValue = _pValue->get<std::string>();
		return true;
	}
	bool Archive::JsonReadFloats(const std::string& a_name, float* a_pOutValues, size_t a_count)
	{
		const nlohmann::json* _pValue = FindField(CurrentNode(), a_name);
		if (!_pValue || !_pValue->is_array()) return false;

		// 足りない分は呼び出し側の値をそのまま残す
		const size_t _readCount = (std::min)(a_count, _pValue->size());
		for (size_t _i = 0; _i < _readCount; ++_i)
		{
			const nlohmann::json& _element = (*_pValue)[_i];
			if (_element.is_number()) a_pOutValues[_i] = _element.get<float>();
		}
		return true;
	}

	void Archive::StringField(const std::string& a_name, std::string& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// Json処理
			if (!m_upJson->is_null()) CurrentNode()[a_name] = a_data;
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::WriteString(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// Json処理
			if (!m_upJson->is_null() && CurrentNode().contains(a_name))
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
			if (!m_upJson->is_null())
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
			if (!m_upJson->is_null())
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
			if (!m_upJson->is_null())
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
			if (!m_upJson->is_null())
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
			if (!m_upJson->is_null())
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
			if (!m_upJson->is_null())
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
			if (!m_upJson->is_null()) CurrentNode()[a_name] = a_guid.String();
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