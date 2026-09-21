#pragma once

namespace Engine::ECS
{
	struct EngineServices;

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
		std::function<void(void*)> construct;							// コンストラクタ
		std::function<void(CompEditContext&)> edit;						// エディター上から操作する処理
		std::function<void(Persistence::Archive& a_ar, void*)> archive;	// データとして保存する処理
		std::function<void(void*, const EngineServices&)> release;		// 借りているものを返す処理
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
			ENGINE_LOG("すでに登録済みです : %s\n", a_name.c_str());
			return GetTypeID(_typeIdx);
		}


		// トリビアルコピー可能かつ標準レイアウトであることを確認
		// 現在はODB厳守
		static_assert(std::is_trivially_copyable_v<Comp>, "トリビアルコピー不可能");
		static_assert(std::is_standard_layout_v<Comp>, "標準レイアウトでない");

		// 上限チェック
		// 次に振るIDは size() なので、size() == MAX の時点でシグネチャの範囲外になる
		if (m_typeIndexMap.size() >= Limits::MAX_COMPONENT_TYPES)
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
		_data.compAlignSize = Math::Alignment::Up(_data.compSize, _data.compAlign);

		// 関数登録
		ComponentFunc _func = {};
		_func.construct = [](void* a_ptr) {new (a_ptr) Comp(); };		// すでに作られたメモリ上を初期化

		// セーブロード・エディター・解放は持っているものだけ。書いていないコンポーネントは空のまま
		// (呼ぶ側は空なら飛ばす。セーブではグループを書かず、コンポーネントの有無は名前一覧で残る)
		if constexpr (requires (Persistence::Archive& a_ar, void* a_pData) { ComponentTraits<Comp>::Archive(a_ar, a_pData); })
		{
			_func.archive = ComponentTraits<Comp>::Archive;
		}
		else if constexpr (requires { &ComponentTraits<Comp>::Archive; })
		{
			// 名前はあるのに引数が合わない = 黙って保存されなくなる。気付けるように止める
			static_assert(sizeof(Comp) == 0, "ComponentTraits<T>::Archive は (Persistence::Archive&, void*) で書くこと");
		}

		if constexpr (requires (CompEditContext& a_context) { ComponentTraits<Comp>::Edit(a_context); })
		{
			_func.edit = ComponentTraits<Comp>::Edit;
		}
		else if constexpr (requires { &ComponentTraits<Comp>::Edit; })
		{
			static_assert(sizeof(Comp) == 0, "ComponentTraits<T>::Edit は (CompEditContext&) で書くこと");
		}

		if constexpr (requires (void* a_pData, const EngineServices& a_services) { ComponentTraits<Comp>::Release(a_pData, a_services); })
		{
			_func.release = ComponentTraits<Comp>::Release;
		}
		else if constexpr (requires (void* a_pData) { ComponentTraits<Comp>::Release(a_pData); })
		{
			// 旧い形(サービスを受け取らない)のまま残っていると、上の判定に掛からず
			// 解放フックが黙って登録されない = 返し漏れになる。気付けるように止める
			// (Comp に依存させておかないと、この分岐に来ないときでも評価されて止まる)
			static_assert(sizeof(Comp) == 0, "ComponentTraits<T>::Release は (void*, const EngineServices&) で書くこと");
		}

		// 登録
		m_compNameMap.emplace(a_name,_typeID);		// 名前との対応表
		m_typeIndexMap.emplace(_typeIdx, _typeID);	// タイプインデックスとの対応表

		m_compTypeMap.emplace(_typeID, _data);	// メタデータの対応表
		m_compFuncMap.emplace(_typeID, _func);	// 関数との対応表

		return _typeID;
	}
}