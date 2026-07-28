#pragma once
//==========================================================================================
//
// ObjectMetaRegistry (クラスメタマネージャー)
//
// ECS外の GameObject(BaseObject派生) の「クラス情報」を一元管理するレジストリ。
// ComponentMetaRegistry のオブジェクト版にあたる。
//
//   - GameManager::Init で Instance() を生成し、各クラスを RegisterType で登録する。
//   - 登録したクラスは「タイプインデックス(ObjectTypeID)」で引ける。
//   - シーン保存時は各オブジェクトの ObjectTypeID / GUID / データ を書き出し、
//     読み込み時は ObjectTypeID からクラス情報(ファクトリ)を引いてインスタンスを復元する。
//
//==========================================================================================

#include "../BaseObject/BaseObject.h"

namespace Engine::GameObject
{
	// ランタイム用のクラスID(登録順に振られる)。
	// シーンファイルにはこの値を書き出すため、登録順(GameManager::Init)を変えると
	// 既存シーンとの互換が崩れる点に注意(コンポーネントのタイプIDと同じ運用)。
	using ObjectTypeID = uint32_t;

	// 無効値
	inline constexpr ObjectTypeID INVALID_OBJECT_TYPE_ID = 0xFFFFFFFF;

	// クラスのメタ情報
	struct ObjectMeta
	{
		std::string name = "none";	// 表示名 / シリアライズ補助
	};

	// クラスに付随する処理
	struct ObjectFunc
	{
		// 既定コンストラクタでインスタンスを1つ生成するファクトリ
		std::function<std::unique_ptr<BaseObject>()> create;
	};

	class ObjectMetaRegistry
	{
	public:

		// シングルトン取得(GameManager::Init で最初に触れて生成する)
		static ObjectMetaRegistry& Instance()
		{
			static ObjectMetaRegistry _instance;
			return _instance;
		}

		// クラスの登録 : メタ情報とファクトリを同時に登録し、タイプIDを返す
		template<typename T>
		ObjectTypeID RegisterType(const std::string& a_name);

		// タイプIDの取得
		template<typename T>
		ObjectTypeID GetTypeID() const;								// C++型から
		ObjectTypeID GetTypeID(const std::string& a_name) const;	// クラス名から
		ObjectTypeID GetTypeID(const std::type_index& a_index) const;	// タイプインデックスから

		// メタ情報取得
		const ObjectMeta& GetMeta(ObjectTypeID a_id) const;

		// 登録したファクトリで実体を生成する(未登録なら nullptr)
		std::unique_ptr<BaseObject> Create(ObjectTypeID a_id) const;

		// 全クラス情報(エディターの AddObject 一覧用)
		const std::unordered_map<ObjectTypeID, ObjectMeta>& GetAllMeta() const { return m_metaMap; }

		// 登録済みか
		bool IsValid(ObjectTypeID a_id) const { return m_metaMap.find(a_id) != m_metaMap.end(); }

	private:

		ObjectMetaRegistry() = default;

		// C++型 / 名前 → タイプID
		std::unordered_map<std::type_index, ObjectTypeID>	m_typeIndexMap;
		std::unordered_map<std::string, ObjectTypeID>		m_nameMap;

		// タイプID → 情報
		std::unordered_map<ObjectTypeID, ObjectMeta> m_metaMap;
		std::unordered_map<ObjectTypeID, ObjectFunc> m_funcMap;
	};

	template<typename T>
	inline ObjectTypeID ObjectMetaRegistry::GetTypeID() const
	{
		return GetTypeID(std::type_index(typeid(T)));
	}

	template<typename T>
	inline ObjectTypeID ObjectMetaRegistry::RegisterType(const std::string& a_name)
	{
		// BaseObject 派生であることを保証
		static_assert(std::is_base_of_v<BaseObject, T>, "T は BaseObject を継承している必要があります");

		// すでに登録済みなら既存IDを返す
		std::type_index _typeIdx = typeid(T);
		if (ObjectTypeID _existing = GetTypeID(_typeIdx); _existing != INVALID_OBJECT_TYPE_ID)
		{
			return _existing;
		}

		// 新しいタイプIDを登録順で発行
		ObjectTypeID _typeID = static_cast<ObjectTypeID>(m_metaMap.size());

		// メタ情報
		ObjectMeta _meta = {};
		_meta.name = a_name;

		// ファクトリ
		ObjectFunc _func = {};
		_func.create = []() -> std::unique_ptr<BaseObject> { return std::make_unique<T>(); };

		// 各対応表へ登録
		m_typeIndexMap.emplace(_typeIdx, _typeID);
		m_nameMap.emplace(a_name, _typeID);
		m_metaMap.emplace(_typeID, _meta);
		m_funcMap.emplace(_typeID, _func);

		return _typeID;
	}
}
