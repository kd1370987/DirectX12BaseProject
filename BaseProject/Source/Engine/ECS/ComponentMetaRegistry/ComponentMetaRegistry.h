#pragma once

namespace Engine::ECS
{
	class World;

	struct ComponentTypeInfo
	{
		std::string name = "None";
		UUID guid = {};
	};

	struct ComponentMeta
	{
		std::string name = "none";

		size_t compSize = 0;		// サイズ
		size_t compAlign = 0;		// アライメント
		size_t compAlignSize = 0;	// アライメントサイズ
	};

	class ComponentMetaRegistry
	{
	public:

		// 型情報からのIDの取得
		template<typename Comp>
		ComponentTypeID GetTypeID();
		// タイプインデックスから取得
		ComponentTypeID GetTypeID(const std::type_index& a_index) const;

		// メタ情報取得
		const ComponentMeta& GetMetaData(const ComponentTypeID& a_id) const;
		const ComponentMeta& GetMetaData(const std::type_index& a_index) const;
		const std::unordered_map<ComponentTypeID, ComponentMeta>& GetAllMetaData() const;

		/// <summary>
		/// コンポーネントを登録し、メタ情報を記憶する
		/// </summary>
		/// <typeparam name="Comp">コンポーネント構造体</typeparam>
		/// <param name="a_name">保存名</param>
		/// <returns>登録ID</returns>
		template<typename Comp>
		ComponentTypeID RegisterType(const std::string& a_name);

	private:

		// 安定したものからランタイム時に使う高速なIDに変換
		//std::unordered_map<UUID, ComponentTypeID> m_guidMap;					// GUIDから
		std::unordered_map<std::type_index, ComponentTypeID> m_typeIndexMap;	// C++型から

		// ランタイム時に使用されるデータ
		// コンポーネントと、その構造体情報を結ぶ
		std::unordered_map<ComponentTypeID, ComponentMeta> m_compTypeMap;

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
		if (Limits::INVALID_COMPONENTTYPEID != GetTypeID(typeid(Comp)))
		{
			Engine::Editor::MainEditor::Instance().AddLog("すでに登録済みです : %s\n", a_name.c_str());
			return GetTypeID(typeid(Comp));
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

		// 型情報を取得
		std::type_index _idx = typeid(Comp);

		// 登録
		auto _it = m_typeIndexMap.find(_idx);
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

		// 登録
		m_typeIndexMap.emplace(_idx, _typeID);
		m_compTypeMap.emplace(_typeID, _data);

		return _typeID;
	}

}