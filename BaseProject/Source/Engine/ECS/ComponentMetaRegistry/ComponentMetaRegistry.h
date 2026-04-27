#pragma once

namespace Engine::ECS
{
	class World;

	// コンポーネントの名前やサイズの情報
	struct ComponentMeta
	{
		std::string name = "none";

		size_t compSize = 0;		// サイズ
		size_t compAlign = 0;		// アライメント
		size_t compAlignSize = 0;	// アライメントサイズ
	};

	// コンポーネントに付随する特殊処理
	struct ComponentFunc
	{
		std::function<void(void*)> construct;

		std::function<void(const void*, nlohmann::json&)> serialize;
		std::function<void(void*, const nlohmann::json&)> deserialize;
		std::function<void(void*)> edit;
	};

	class ComponentMetaRegistry
	{
	public:

		// コンポーネントの登録
		// メタ情報と関数も同時に登録する
		template<typename Comp>
		ComponentTypeID RegisterType(const std::string& a_name);

		// コンポーネントタイプIDの取得
		template<typename Comp>
		ComponentTypeID GetTypeID();										// 型情報から直接取得
		ComponentTypeID GetTypeID(const std::string& a_name);				// コンポーネント名から取得
		ComponentTypeID GetTypeID(const std::type_index& a_index) const;	// タイプインデックスから取得

		// メタ情報取得
		const ComponentMeta& GetMetaData(const ComponentTypeID& a_id) const;	// タイプIDから
		const ComponentMeta& GetMetaData(const std::type_index& a_index) const;	// タイプインデックスから

		// 関数情報取得
		const ComponentFunc& GetFunc(const ComponentTypeID& a_id) const;

		// 全コンポーネントの情報を取得
		const std::unordered_map<ComponentTypeID, ComponentMeta>& GetAllMetaData() const;

	private:

		// ラインタイム用IDへの変換
		std::unordered_map<std::type_index, ComponentTypeID>	m_typeIndexMap;		// C++型から
		std::unordered_map<std::string, ComponentTypeID>		m_compNameMap;		// コンポーネント名から

		// コンポーネントに付随するデータ
		std::unordered_map<ComponentTypeID, ComponentMeta> m_compTypeMap;		// 型の情報
		std::unordered_map<ComponentTypeID, ComponentFunc> m_compFuncMap;		// 関数情報
	};

	template<typename Comp>
	inline ComponentTypeID ComponentMetaRegistry::GetTypeID()
	{
		auto _it = m_typeIndexMap.find(typeid(Comp));
		if (_it != m_typeIndexMap.end())
		{
			return _it->second;
		}
		return ECS::Limits::INVALID_COMPONENTTYPEID;
	}

	template<typename Comp>
	inline ComponentTypeID ComponentMetaRegistry::RegisterType(const std::string& a_name)
	{
		// 型情報を取得
		std::type_index _typeIdx = typeid(Comp);
		if (Limits::INVALID_COMPONENTTYPEID != GetTypeID(_typeIdx))
		{
			Engine::Editor::MainEditor::Instance().AddLog("すでに登録済みです : %s\n", a_name.c_str());
			return GetTypeID(_typeIdx);
		}


		// トリビアルコピー可能かつ標準レイアウトであることを確認
		// 現在はODB厳守
		static_assert(std::is_trivially_copyable_v<Comp>, "トリビアルコピー不可能");
		static_assert(std::is_standard_layout_v<Comp>, "標準レイアウトでない");

		// 上限チェック
		if (m_typeIndexMap.size() > Limits::MAX_COMPONENT_TYPES)
		{
			assert(0 && "登録できるコンポーネント数の上限に達しました");
			return Limits::INVALID_COMPONENTTYPEID;
		}

		// 登録
		auto _it = m_typeIndexMap.find(_typeIdx);
		if (_it != m_typeIndexMap.end())
		{
			return _it->second;
		}

		// 新たなタイプIDを生成
		ECS::ComponentTypeID _typeID = static_cast<ECS::ComponentTypeID>(m_typeIndexMap.size());

		// データの生成
		ComponentMeta _data = {};
		_data.name = a_name;
		_data.compSize = sizeof(Comp);
		_data.compAlign = alignof(Comp);
		_data.compAlignSize = Alignment::Up(_data.compSize, _data.compAlign);

		// 関数登録
		ComponentFunc _func = {};
		_func.construct = [](void* a_ptr) {new (a_ptr) Comp(); };		// すでに作られたメモリ上を初期化
		_func.serialize = &Comp::Serialize;
		_func.deserialize = &Comp::Deserialize;
		_func.edit = &Comp::Edit;

		// 登録
		m_compNameMap.emplace(a_name,_typeID);		// 名前との対応表
		m_typeIndexMap.emplace(_typeIdx, _typeID);	// タイプインデックスとの対応表

		m_compTypeMap.emplace(_typeID, _data);	// メタデータの対応表
		m_compFuncMap.emplace(_typeID, _func);	// 関数との対応表

		return _typeID;
	}
}