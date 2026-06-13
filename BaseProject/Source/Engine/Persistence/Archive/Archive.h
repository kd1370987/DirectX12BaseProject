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
		void StringField(const std::string& a_name,std::string& a_data);

		// ベクターのシリアライズ
		template<typename T>
		void VectorField(const std::string& a_name,std::vector<T>& a_data);

		// GUID用
		void GUIDField(const std::string& a_name,Engine::GUID& a_guid);
		void GUIDVectorField(const std::string& a_name, std::vector<Engine::GUID>& a_guid);

	private:
		// 実行モード
		Mode m_mode;

		// 本番用ストリーム
		std::ofstream m_ofs;
		std::ifstream m_ifs;

		// テスト時データ
		nlohmann::json m_json = {};

		// パス
		std::string m_fileDir;		// ディレクトリ
		std::string m_binPath;
		std::string m_jsonPath;

	};
	template<typename T>
	inline void Archive::Field(const std::string& a_name, T& a_data)
	{
		static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>,
			"Only arithmetic or enum types are allowed");

		// 保存処理
		if (IsSaving())
		{
			// テスト時データ
			m_json[a_name] = a_data;
			// 本番時(バイナリ)
			if (m_ofs.is_open())
			{
				BinaryHelper::Write(m_ofs,a_data);
			}
		}
		// 読み込み処理
		else
		{
			// テスト時
			if (m_json.contains(a_name))
			{
				a_data = m_json[a_name].get<T>();
			}
			// 本番時
			if (m_ifs.is_open())
			{
				BinaryHelper::Read(m_ifs,a_data);
			}
		}
	}
	template<typename T>
	inline void Archive::VectorField(const std::string& a_name, std::vector<T>& a_data)
	{
		// 保存処理
		if (IsSaving())
		{
			// 要素数の書き込み
			size_t _size = a_data.size();
			Field(a_name + "_size",_size);

			// 各要素のシリアライズ
			for (size_t _i = 0; _i < _size; ++_i)
			{
				Field(a_name + "[" + std::to_string(_i) + "]", a_data[_i]);
			}
		}
		// 読み込み
		else
		{
			// 要素数でリサイズ
			size_t _size = 0;
			Field(a_name + "_size",_size);
			a_data.resize(_size);

			for (size_t _i = 0; _i < _size; ++_i)
			{
				Field(a_name + "[" + std::to_string(_i) + "]", a_data[_i]);
			}
		}
	}

	template<>
	inline void Archive::Field(
		const std::string& a_name,
		DirectX::XMFLOAT4X4& a_data)
	{
		if (IsSaving())
		{
			m_json[a_name] =
			{
				a_data._11,a_data._12,a_data._13,a_data._14,
				a_data._21,a_data._22,a_data._23,a_data._24,
				a_data._31,a_data._32,a_data._33,a_data._34,
				a_data._41,a_data._42,a_data._43,a_data._44
			};

			if (m_ofs.is_open())
			{
				BinaryHelper::Write(
					m_ofs,
					a_data);
			}
		}
		else
		{
			if (m_json.contains(a_name))
			{
				auto& j = m_json[a_name];

				a_data =
				{
					j[0],j[1],j[2],j[3],
					j[4],j[5],j[6],j[7],
					j[8],j[9],j[10],j[11],
					j[12],j[13],j[14],j[15]
				};
			}

			if (m_ifs.is_open())
			{
				BinaryHelper::Read(
					m_ifs,
					a_data);
			}
		}
	}
	template<>
	inline void Archive::Field(
		const std::string& a_name,
		DirectX::XMFLOAT3& a_data)
	{
		if (IsSaving())
		{
			m_json[a_name] =
			{
				a_data.x,a_data.y,a_data.z
			};

			if (m_ofs.is_open())
			{
				BinaryHelper::Write(
					m_ofs,
					a_data);
			}
		}
		else
		{
			if (m_json.contains(a_name))
			{
				auto& j = m_json[a_name];

				a_data = { j[0],j[1],j[2] };
			}

			if (m_ifs.is_open())
			{
				BinaryHelper::Read(
					m_ifs,
					a_data);
			}
		}
	}
	template<>
	inline void Archive::Field(
		const std::string& a_name,
		DirectX::XMFLOAT4& a_data)
	{
		if (IsSaving())
		{
			m_json[a_name] =
			{
				a_data.x,a_data.y,a_data.z,a_data.w
			};

			if (m_ofs.is_open())
			{
				BinaryHelper::Write(
					m_ofs,
					a_data);
			}
		}
		else
		{
			if (m_json.contains(a_name))
			{
				auto& j = m_json[a_name];

				a_data = { j[0],j[1],j[2],j[3]};
			}

			if (m_ifs.is_open())
			{
				BinaryHelper::Read(
					m_ifs,
					a_data);
			}
		}
	}

	template<>
	inline void Archive::Field(
		const std::string& a_name,
		DirectX::XMFLOAT2& a_data)
	{
		if (IsSaving())
		{
			m_json[a_name] =
			{
				a_data.x,a_data.y
			};

			if (m_ofs.is_open())
			{
				BinaryHelper::Write(
					m_ofs,
					a_data);
			}
		}
		else
		{
			if (m_json.contains(a_name))
			{
				auto& j = m_json[a_name];

				a_data = { j[0],j[1] };
			}

			if (m_ifs.is_open())
			{
				BinaryHelper::Read(
					m_ifs,
					a_data);
			}
		}
	}
}