#pragma once
namespace Engine::Graphics::Pipeline
{
	class Pass;

	// パスのメタ情報
	struct PassMeta
	{
		std::string name = "none";		// 表示名

		// グラフの出口かどうか。
		// 立っているパスは1つのグラフに必ず1つだけ常駐し、
		// エディターの追加一覧には出さず、削除もさせない
		bool isFinalPass = false;

		//----------------------------------------------------------------------------------
		// シェーディングモデル表を引くときの鍵(モデルを受け取らないパスは空)
		//
		// 表はこの名前ごとにピクセルシェーダーを持つ。
		// 表を編集する画面が「どのパス名を並べればよいか」を知るために、
		// 登録のときに型から1つ作って読み出しておく
		//----------------------------------------------------------------------------------
		std::string shadingPassName = {};
	};

	// パスに付随する関数
	struct PassFunc
	{
		std::function<std::unique_ptr<Pass>()> factory = nullptr;
	};

	// レンダリングパイプラインを登録する場所
	// インスタンスは保持しない。あくまで、クラスの型情報を保持して、作成時に渡す用
	class PassMetaRegistry
	{
	public:

		// パスの登録 : メタ情報とファクトリを作成して、タイプIDを返す。
		// a_isFinalPass を立てると「グラフの出口」として扱われる(常駐・削除不可)
		template<typename T>
		ID<Pass> RegisterType(const std::string& a_name, bool a_isFinalPass = false);

		// タイプIDの取得
		template<typename T>
		ID<Pass> GetTypeID() const;									// C++型から
		ID<Pass> GetTypeID(const std::string& a_name) const;		// 名前から
		ID<Pass> GetTypeID(const std::type_index& a_index) const;	// タイプインデックスから

		// メタ情報取得
		const PassMeta* GetMeta(ID<Pass> a_id) const;

		// 登録したファクトリで実体を生成する(未登録なら nullptr)
		std::unique_ptr<Pass> Create(ID<Pass> a_id) const;

		// 全クラス情報(エディターの AddObject 一覧用)
		const std::unordered_map<ID<Pass>, PassMeta>& GetAllMeta() const { return m_metaMap; }

		// グラフの出口として登録されているパスの型ID : 無ければ無効値
		ID<Pass> GetFinalPassTypeID() const;

		// この型がグラフの出口か
		bool IsFinalPassType(ID<Pass> a_id) const;

	private:

		// C++型 / 名前 ->タイプID
		std::unordered_map<std::type_index, ID<Pass>> m_typeIndexMap = {};
		std::unordered_map<std::string, ID<Pass>> m_nameMap = {};

		// タイプID -> 情報
		std::unordered_map<ID<Pass>, PassMeta> m_metaMap = {};
		std::unordered_map<ID<Pass>, PassFunc> m_funcMap = {};

	};
	template<typename T>
	inline ID<Pass> PassMetaRegistry::RegisterType(const std::string& a_name, bool a_isFinalPass)
	{
		// Passの派生であることを保証する
		static_assert(std::is_base_of_v<Pass,T>,"T は Pass を継承している必要があります");

		// すでに登録済みなら既存のIDを返す
		std::type_index _typeIdx = typeid(T);
		if (ID<Pass>  _existing = GetTypeID(_typeIdx); _existing.IsValid())
		{
			return _existing;
		}

		// 登録名からタイプIDを作成
		// ID<> のコンストラクタは explicit なので直接初期化で作る
		const ID<Pass> _typeID{ static_cast<uint32_t>(Engine::String::ToHash(a_name)) };

		// 名前チェック
		// IsValid() が false = 内部値が uint32_t の最大値(無効値)
		if (_typeID.value == 0 || !_typeID.IsValid())
		{
			ENGINE_WARNING("[PassMetaRegistry] タイプIDが無効値になりました。登録名を変えてください : %s", a_name.c_str());
			assert(0 && "PassTypeID が無効値 : 登録名を変えること");
			return ID<Pass>();
		}
		if (auto _it = m_metaMap.find(_typeID); _it != m_metaMap.end())
		{
			ENGINE_WARNING("[PassMetaRegistry] タイプIDが衝突しました : %s <-> %s", a_name.c_str(), _it->second.name.c_str());
			assert(0 && "PassTypeID の衝突 : どちらかの登録名を変えること");
			return ID<Pass>();
		}

		// メタ情報
		PassMeta _meta = {};
		_meta.name = a_name;
		_meta.isFinalPass = a_isFinalPass;

		// ファクトリ
		PassFunc _func = {};
		_func.factory = []() -> std::unique_ptr<Pass> {return std::make_unique<T>(); };

		// シェーディングモデル表の鍵は型ごとに決まっている。
		// 固定文字列を返すだけの関数なので、スロット宣言を通していない実体からでも読める
		if (std::unique_ptr<Pass> _upProbe = _func.factory())
		{
			if (const char* _pShadingName = _upProbe->GetShadingPassName())
			{
				_meta.shadingPassName = _pShadingName;
			}
		}

		// 各対応表へ登録
		m_typeIndexMap.emplace(_typeIdx, _typeID);
		m_nameMap.emplace(a_name, _typeID);
		m_metaMap.emplace(_typeID, _meta);
		m_funcMap.emplace(_typeID, _func);

		return _typeID;
	}
	template<typename T>
	inline ID<Pass> PassMetaRegistry::GetTypeID() const
	{
		return GetTypeID(std::type_index(typeid(T)));
	}

	// エンジン標準のパスをまとめて登録する
	// これを通していないと、ノードエディタの AddPass 一覧が空のままになる
	void RegisterBuiltinPasses(PassMetaRegistry& a_registry);
}