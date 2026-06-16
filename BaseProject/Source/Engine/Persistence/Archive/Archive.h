#pragma once

#include "../../Utility/BinaryHelper/BinaryHelper.h"

namespace Engine::Persistence
{
	// テスト時にはjson、本番時にはバイナリでデータを管理できるクラス
	// アーカイブ方式で作成
	class Archive
	{
	public:

		// フォーマット指定用の列挙型
		enum class ArchiveFormat
		{
			Auto,	// 基本設定（ビルドモードに依存）
			Binary,	// 強制的にバイナリ(.ob)を使用
			Json	// 強制的にJSON(.oj)を使用
		};

		enum class Mode
		{
			Save,		// 書き込み
			Load		// 読み込み
		};

		// モード、ファイルディレクトリを指定して開く
		Archive(
			Mode a_mode, 
			const std::string& a_fileDir,
			const std::string& a_fileName,
			const std::string& a_ext,
			ArchiveFormat a_format = ArchiveFormat::Auto
		);
		// クローズ処理を実行
		~Archive();

		// 現在の実行モード
		bool IsSaving() const { return m_mode == Mode::Save; }
		bool IsLoading() const { return m_mode == Mode::Load; }

		// 基本型のシリアライズ : 構造体はしないように
		template<typename T>
		void Field(const std::string& a_name,T& a_data);

		// 文字列型のシリアライズ
		void StringField(const std::string& a_name, std::string& a_data);

		// ベクターのシリアライズ
		template<typename T>
		void VectorField(const std::string& a_name,std::vector<T>& a_data);

		// 構造体のグループ化（JSON の {} ）
		bool BeginGroup(const std::string& a_name);
		void EndGroup();

		// 配列の管理（JSON の [] ）
		bool BeginArray(const std::string& a_name, size_t& a_size);
		void EndArray();

		// 配列の中の1要素としてのオブジェクト（名前なしの {} ）
		bool BeginObject(size_t a_index = 0);
		void EndObject();

		// GUID用
		void GUIDField(const std::string& a_name,Engine::GUID& a_guid);
		void GUIDVectorField(const std::string& a_name, std::vector<Engine::GUID>& a_guid);

	private:
		// 実行モード
		Mode m_mode;

		ArchiveFormat m_format = ArchiveFormat::Auto;

		// 本番用ストリーム
		std::ofstream m_ofs;
		std::ifstream m_ifs;

		// テスト時データ
		nlohmann::json m_json = {};

		// パス
		std::string m_fileDir;		// ディレクトリ
		std::string m_binPath;
		std::string m_jsonPath;

		// 現在注目しているJSONノードのポインタ（参照）をスタックで管理する
		std::stack<nlohmann::json*> m_jsonNodeStack;

		nlohmann::json& CurrentNode()
		{
			return m_jsonNodeStack.empty() ? m_json : *m_jsonNodeStack.top();
		}

	};

	// =========================================================================
	// フィールド処理
	// =========================================================================
	template<typename T>
	inline void Archive::Field(const std::string& a_name, T& a_data)
	{
		static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>,
			"Only arithmetic or enum types are allowed");

		if (IsSaving())
		{
			CurrentNode()[a_name] = a_data; // m_json から CurrentNode() に変更

			if (m_ofs.is_open())
			{
				BinaryHelper::Write(m_ofs, a_data);
			}
		}
		else
		{
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				a_data = CurrentNode()[a_name].get<T>();
			}

			if (m_ifs.is_open())
			{
				BinaryHelper::Read(m_ifs, a_data);
			}
		}
	}
	// 特殊化
	// 行列
	template<>
	inline void Archive::Field(const std::string& a_name, DirectX::XMFLOAT4X4& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// json処理
			CurrentNode()[a_name] =
			{
				a_data._11,a_data._12,a_data._13,a_data._14,
				a_data._21,a_data._22,a_data._23,a_data._24,
				a_data._31,a_data._32,a_data._33,a_data._34,
				a_data._41,a_data._42,a_data._43,a_data._44
			};

			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data =
				{
					j[0],j[1],j[2],j[3],
					j[4],j[5],j[6],j[7],
					j[8],j[9],j[10],j[11],
					j[12],j[13],j[14],j[15]
				};
			}

			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	template<>
	inline void Archive::Field(const std::string& a_name, DXSM::Matrix& a_data)
	{
		if (IsSaving())
		{
			CurrentNode()[a_name] =
			{
				a_data._11,a_data._12,a_data._13,a_data._14,
				a_data._21,a_data._22,a_data._23,a_data._24,
				a_data._31,a_data._32,a_data._33,a_data._34,
				a_data._41,a_data._42,a_data._43,a_data._44
			};

			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		else
		{
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data =
				{
					j[0],j[1],j[2],j[3],
					j[4],j[5],j[6],j[7],
					j[8],j[9],j[10],j[11],
					j[12],j[13],j[14],j[15]
				};
			}

			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	// float3
	template<>
	inline void Archive::Field(const std::string& a_name, DirectX::XMFLOAT3& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// json処理
			CurrentNode()[a_name] = { a_data.x, a_data.y, a_data.z };
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data = { j[0], j[1], j[2] };
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	template<>
	inline void Archive::Field(const std::string& a_name, DXSM::Vector3& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// json処理
			CurrentNode()[a_name] = { a_data.x, a_data.y, a_data.z };
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data = { j[0], j[1], j[2] };
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	// float4
	template<>
	inline void Archive::Field(const std::string& a_name, DirectX::XMFLOAT4& a_data)
	{	
		// セーブ時
		if (IsSaving())
		{
			// json処理
			CurrentNode()[a_name] = { a_data.x, a_data.y, a_data.z, a_data.w };
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data = { j[0], j[1], j[2], j[3] };
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	template<>
	inline void Archive::Field(const std::string& a_name, DXSM::Vector4& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// json処理
			CurrentNode()[a_name] = { a_data.x, a_data.y, a_data.z, a_data.w };
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data = { j[0], j[1], j[2], j[3] };
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	template<>
	inline void Archive::Field(const std::string& a_name, DXSM::Quaternion& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// json処理
			CurrentNode()[a_name] = { a_data.x, a_data.y, a_data.z, a_data.w };
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data = { j[0], j[1], j[2], j[3] };
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	// float2
	template<>
	inline void Archive::Field(const std::string& a_name, DirectX::XMFLOAT2& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// json処理
			CurrentNode()[a_name] = { a_data.x, a_data.y };
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data = { j[0], j[1] };
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	template<>
	inline void Archive::Field(const std::string& a_name, DXSM::Vector2& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// json処理
			CurrentNode()[a_name] = { a_data.x, a_data.y };
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data);
		}
		// ロード時
		else
		{
			// json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				auto& j = CurrentNode()[a_name];
				a_data = { j[0], j[1] };
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data);
		}
	}
	// 文字列
	template<>
	inline void Archive::Field(const std::string& a_name, std::string& a_data)
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
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				a_data = CurrentNode()[a_name].get<std::string>();
			}
			// binary処理
			if (m_ifs.is_open()) a_data = BinaryHelper::ReadString(m_ifs);
		}
	}
	// GUID
	template<>
	inline void Archive::Field(const std::string& a_name, Engine::GUID& a_data)
	{
		// セーブ時
		if (IsSaving())
		{
			// Json処理
			if (!m_json.is_null()) CurrentNode()[a_name] = a_data.String();
			// binary処理
			if (m_ofs.is_open()) BinaryHelper::Write(m_ofs, a_data.value);
		}
		// ロード時
		else
		{
			// Json処理
			if (CurrentNode().is_object() && CurrentNode().contains(a_name))
			{
				a_data.FromString(CurrentNode()[a_name].get<std::string>());
			}
			// binary処理
			if (m_ifs.is_open()) BinaryHelper::Read(m_ifs, a_data.value);
		}
	}

	template<typename T>
	inline void Archive::VectorField(const std::string& a_name, std::vector<T>& a_data)
	{
		size_t _size = a_data.size();

		// 新しい BeginArray と BeginObject を使ったスマートな配列シリアライズ
		if (BeginArray(a_name, _size))
		{
			a_data.resize(_size);
			for (size_t _i = 0; _i < _size; ++_i)
			{
				if (BeginObject(_i))
				{
					// JSONでは [ {"v": 10}, {"v": 20} ] のように綺麗に格納される
					// バイナリモードでは BeginObject は何もしないので、単純に連続したデータとして書き込まれる
					Field("v", a_data[_i]);
					EndObject();
				}
			}
			EndArray();
		}
	}
}