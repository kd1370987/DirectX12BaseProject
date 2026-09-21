#pragma once

namespace Engine::ECS
{
	struct EngineServices;

	class World;
	class ComponentMetaRegistry;

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

	//==========================================================================================
	// 型ごとのタイプIDの置き場所
	//
	// GetTypeID<T>() はここを1回読むだけなので、typeid やハッシュの検索が要らない。
	// 値を書くのは ComponentMetaRegistry::RegisterType だけ(番号の出所を1つにする)。
	// 自分でカウンタを回すと、最初に呼ばれた順とレジストリの登録順の2つの番号ができて
	// シグネチャのビットとメタ情報(名前)がずれる
	//==========================================================================================
	template<typename T>
	struct ComponentTypeSlot
	{
		inline static ComponentTypeID				id		= Limits::INVALID_COMPONENTTYPEID;
		inline static const ComponentMetaRegistry*	pOwner	= nullptr;	// 番号を書いたレジストリ
	};

	//==========================================================================================
	// コンポーネントの型情報
	//
	// プロセスに1つだけ置く(持ち主は MainEngine)。どのワールドも同じものを借りるので、
	// 同じ型はどのワールドでも同じタイプIDになる。
	//
	// タイプIDは 0 から詰めて振る。そのまま
	//   ・シグネチャのビットの位置
	//   ・メタ情報 / 関数の配列の添え字
	// として使うので、別の番号への変換はどこにも無い。
	//
	// タイプIDは登録順で決まる「実行中だけ通じる番号」。ファイルには書かないこと。
	// 保存と読み込みはコンポーネント名で行い、読み込み時に名前から番号を引き直す。
	// そのため、登録の順番を変えたりコンポーネントを足したりしても、保存データは壊れない
	//
	// 引き方は3つだけ
	//   型から   : GetTypeID<T>()     システム・クエリ・ゲームコード
	//   名前から : GetTypeID(name)    保存データの読み込み
	//   番号から : GetMetaData(id) / GetFunc(id)   エディター・構築と解放のフック
	//==========================================================================================
	class ComponentMetaRegistry
	{
	public:

		ComponentMetaRegistry() = default;
		~ComponentMetaRegistry();

		// 型ごとの置き場所に番号を書くので、複製すると持ち主が2人になる
		ComponentMetaRegistry(const ComponentMetaRegistry&) = delete;
		ComponentMetaRegistry& operator=(const ComponentMetaRegistry&) = delete;

		// コンポーネントの登録
		// メタ情報と関数も同時に登録する。
		// 登録済みの型なら何もせず今の番号を返す(ワールドを作るたびに呼ばれるため)
		template<typename Comp>
		ComponentTypeID RegisterType(const std::string& a_name);

		// コンポーネントタイプIDの取得 : 未登録なら INVALID_COMPONENTTYPEID
		template<typename Comp>
		static ComponentTypeID GetTypeID();							// 型から(置き場所を読むだけ)
		ComponentTypeID GetTypeID(const std::string& a_name) const;	// コンポーネント名から

		// 登録済みの数(= 有効なタイプIDは 0 〜 GetCount()-1)
		ComponentTypeID GetCount() const { return static_cast<ComponentTypeID>(m_metaVec.size()); }

		// メタ情報取得
		const ComponentMeta& GetMetaData(const ComponentTypeID& a_id) const;

		// 関数情報取得
		const ComponentFunc& GetFunc(const ComponentTypeID& a_id) const;

		// 全コンポーネントの情報 : 添え字がそのままタイプID
		const std::vector<ComponentMeta>& GetAllMetaData() const { return m_metaVec; }

	private:

		// 名前からタイプIDへ(保存データの読み込みで使う)
		std::unordered_map<std::string, ComponentTypeID> m_compNameMap;

		// コンポーネントに付随するデータ : 添え字がタイプID
		std::vector<ComponentMeta> m_metaVec;	// 型の情報
		std::vector<ComponentFunc> m_funcVec;	// 関数情報

		// 型ごとの置き場所を未登録へ戻す処理(レジストリが消えるときに呼ぶ)
		std::vector<void(*)()> m_resetSlotFuncVec;
	};

	template<typename Comp>
	inline ComponentTypeID ComponentMetaRegistry::GetTypeID()
	{
		// const 付きで引かれても同じ置き場所を見る
		return ComponentTypeSlot<std::remove_cv_t<Comp>>::id;
	}

	template<typename Comp>
	inline ComponentTypeID ComponentMetaRegistry::RegisterType(const std::string& a_name)
	{
		static_assert(!std::is_const_v<Comp> && !std::is_volatile_v<Comp>, "const / volatile を付けずに登録すること");

		// トリビアルコピー可能かつ標準レイアウトであることを確認
		// 現在はODB厳守
		static_assert(std::is_trivially_copyable_v<Comp>, "トリビアルコピー不可能");
		static_assert(std::is_standard_layout_v<Comp>, "標準レイアウトでない");

		using Slot = ComponentTypeSlot<Comp>;

		// 登録済み : ワールドを作るたびに同じ登録が流れてくるので、今の番号を返すだけ
		if (Slot::pOwner == this)
		{
			return Slot::id;
		}

		// 置き場所に番号があるのに書いたのは別のレジストリ = 番号の出所が2つになるので止める
		if (Slot::pOwner != nullptr)
		{
			assert(0 && "別の ComponentMetaRegistry がこの型を登録しています。レジストリはプロセスに1つだけ置くこと");
			return Slot::id;
		}

		// 名前は保存データのキーなので、別の型と被ってはいけない
		if (m_compNameMap.contains(a_name))
		{
			assert(0 && "同じ名前のコンポーネントが既に登録されています(名前は保存データのキー)");
			return Limits::INVALID_COMPONENTTYPEID;
		}

		// 上限チェック
		// 次に振るIDは size() なので、size() == MAX の時点でシグネチャの範囲外になる
		if (m_metaVec.size() >= Limits::MAX_COMPONENT_TYPES)
		{
			assert(0 && "登録できるコンポーネント数の上限に達しました");
			return Limits::INVALID_COMPONENTTYPEID;
		}

		// 新たなタイプID : 0 から詰めて振る(配列の添え字・シグネチャのビットと一致させる)
		const ComponentTypeID _typeID = static_cast<ComponentTypeID>(m_metaVec.size());

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

		// 登録 : 添え字 = タイプID になるよう末尾に積む
		m_compNameMap.emplace(a_name, _typeID);
		m_metaVec.push_back(std::move(_data));
		m_funcVec.push_back(std::move(_func));

		// 型ごとの置き場所へ書く(番号の出所はここだけ)
		Slot::id = _typeID;
		Slot::pOwner = this;
		m_resetSlotFuncVec.push_back([]()
			{
				Slot::id = Limits::INVALID_COMPONENTTYPEID;
				Slot::pOwner = nullptr;
			});

		return _typeID;
	}
}
