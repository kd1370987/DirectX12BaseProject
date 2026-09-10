#pragma once
//==========================================================================================
//
// CompositeNode (Engine::Editor::Inspector)
//
// 「エディター上は1ノード、実体は複数パス」のまとまり。
//
// デノイズの反復もブルームの縮小段も、ランタイムはパスを並べて表す方針になっている。
// そのままノードにすると5個も10個も並んで配線が読めないので、
// エディターだけがまとめて1ノードに見せる。
//
// グラフには一切手を入れない。パスも線も並べたままで、
// 「同じ札(EditorGroup)を持つものを1つに見せる」だけ。
// 札が消えても個別のノードとして見えるようになるだけで、絵は変わらない
//
//==========================================================================================
namespace Engine::Graphics::Pipeline
{
	class Pass;
	class RenderGraph;
	class RenderingPipelineAsset;
	struct Slot;
	enum class EPassEditResult : uint8_t;
}

namespace Engine::Editor::Inspector
{
	class PassEditorRegistry;

	//======================================================================================
	// まとまり1つぶん
	//
	// 毎フレームその場で組む写し。グラフが正で、こちらは保持しない
	//======================================================================================
	struct CompositeGroup
	{
		Engine::GUID guid = {};			// まとまりの識別子
		std::string typeName = "";		// どの合成ノードが面倒を見るか

		// 段順(EditorGroupIndex 順)に並んだメンバー。
		// 先頭が代表で、ノードIDと座標はこのパスのものを使う
		std::vector<Graphics::Pipeline::Pass*> members = {};

		Graphics::Pipeline::Pass* GetHead() const { return members.empty() ? nullptr : members.front(); }
		Graphics::Pipeline::Pass* GetTail() const { return members.empty() ? nullptr : members.back(); }

		bool Contains(const Engine::GUID& a_passGUID) const;
	};

	//======================================================================================
	// グラフを、まとまりごとに分けた表
	//
	// 描くたびに組み直す。グラフが変わっても取り残しが出ない
	//======================================================================================
	struct CompositeGroupTable
	{
		std::vector<CompositeGroup> groups = {};

		// パスGUID -> groups の添字。まとまりに属さないパスは載らない
		std::unordered_map<Engine::GUID, size_t> passToGroup = {};

		const CompositeGroup* Find(const Engine::GUID& a_passGUID) const;
	};

	// グラフのパスを札ごとに分ける。
	// 札が1つしか無いまとまりも作る(段数を1へ減らしたときも合成ノードのまま扱うため)
	CompositeGroupTable BuildCompositeGroups(Graphics::Pipeline::RenderGraph& a_graph);

	//======================================================================================
	// ノードから出た「グラフを触ってほしい」という要求
	//
	// ノードを描いている最中にパスを増減させると、
	// パス配列を回している最中に配列が変わることになって反復が壊れる。
	// 描くほうは要求を置くだけにして、実際に触るのは描き終わってから
	//======================================================================================
	struct CompositeNodeRequest
	{
		static constexpr int kNoResize = -1;

		int resizeCount = kNoResize;	// 0以上なら段数をこの数へ変える

		bool IsEmpty() const { return resizeCount == kNoResize; }
	};

	//======================================================================================
	// 合成ノード1種類ぶんの受け口
	//
	// 実体は種類ごとに1つで、状態を持たない(対象は毎回渡される)
	//======================================================================================
	class ICompositeNode
	{
	public:

		virtual ~ICompositeNode() = default;

		// 追加メニューに出す名前
		virtual const char* GetDisplayName() const = 0;

		// ノードの見出し
		virtual std::string MakeTitle(const CompositeGroup& a_group) const = 0;

		//----------------------------------------------------------------------------------
		// 外へ見せるピン
		//
		// ここに挙げなかったピンは「中でつながっているもの」として扱われ、
		// ノードにも線にも出ない。
		// 隠したピンにも線は要る(Depth や Normal は全段が要る)ので、
		// そちらは SyncInternalLinks が配る
		//----------------------------------------------------------------------------------
		virtual void CollectVisibleSlots(
			const CompositeGroup& a_group,
			std::vector<Graphics::Pipeline::Slot*>& a_outInputVec,
			std::vector<Graphics::Pipeline::Slot*>& a_outOutputVec) const;

		// ノードの中に出す操作(段数など)。
		// グラフを触りたくなったら a_outRequest へ置くこと
		virtual Graphics::Pipeline::EPassEditResult DrawNode(
			const CompositeGroup& a_group, CompositeNodeRequest& a_outRequest) = 0;

		// 選択したときの詳細
		virtual Graphics::Pipeline::EPassEditResult DrawDetail(
			const CompositeGroup& a_group,
			PassEditorRegistry& a_passEditorRegistry,
			CompositeNodeRequest& a_outRequest) = 0;

		// 段数を変える。描き終わってから呼ばれる
		virtual bool Resize(
			Graphics::Pipeline::RenderingPipelineAsset& a_asset,
			const CompositeGroup& a_group,
			int a_count) { (void)a_asset; (void)a_group; (void)a_count; return false; }

		//----------------------------------------------------------------------------------
		// 中の配線を整える
		//
		// 見せているピンへ外から線が引かれたときに、隠している段へも配り直す。
		// 段数を変えた直後にも通る
		//----------------------------------------------------------------------------------
		virtual void SyncInternalLinks(
			Graphics::Pipeline::RenderGraph& a_graph, const CompositeGroup& a_group) { (void)a_graph; (void)a_group; }

		// 一式を新しく作る。
		//
		// 返すのは代表(段0)のパス : 作れなければ nullptr。
		// 呼んだ側がノード座標を ImNodes へ配るのに要る
		virtual Graphics::Pipeline::Pass* Build(Graphics::Pipeline::RenderingPipelineAsset& a_asset) = 0;
	};

	//======================================================================================
	// まとまりの種類 -> 合成ノード
	//======================================================================================
	class CompositeNodeRegistry
	{
	public:

		CompositeNodeRegistry() = default;
		~CompositeNodeRegistry() = default;

		CompositeNodeRegistry(const CompositeNodeRegistry&) = delete;
		CompositeNodeRegistry& operator=(const CompositeNodeRegistry&) = delete;

		template<class TNode, class... TArgs>
		void Register(const std::string& a_typeName, TArgs&&... a_args)
		{
			static_assert(std::is_base_of_v<ICompositeNode, TNode>,
				"TNode は ICompositeNode を継承している必要があります");

			m_nodeMap.insert_or_assign(a_typeName, std::make_unique<TNode>(std::forward<TArgs>(a_args)...));
			m_typeNameVec.push_back(a_typeName);
		}

		// 種類名から引く : 登録が無ければ nullptr
		ICompositeNode* Find(const std::string& a_typeName) const;

		// 追加メニュー用 : 登録順の種類名
		const std::vector<std::string>& GetTypeNames() const { return m_typeNameVec; }

	private:

		std::unordered_map<std::string, std::unique_ptr<ICompositeNode>> m_nodeMap = {};
		std::vector<std::string> m_typeNameVec = {};
	};

	//--------------------------------------------------------------------------------------
	// エンジン標準の合成ノードをまとめて登録する
	//--------------------------------------------------------------------------------------
	void RegisterBuiltinCompositeNodes(CompositeNodeRegistry& a_registry);

	//--------------------------------------------------------------------------------------
	// 合成ノードの実装から使う小道具
	//--------------------------------------------------------------------------------------
	namespace CompositeUtil
	{
		// この入力ピンへ線を引いている相手を探す : 無ければ nullptr
		Graphics::Pipeline::Pass* FindLinkSource(
			Graphics::Pipeline::RenderGraph& a_graph,
			const Engine::GUID& a_dstPassGUID,
			uint32_t a_dstSlotID,
			uint32_t* a_pOutSrcSlotID);

		// 同じ入力ピンへ、元と同じ相手を繋ぎ直す(段を増やしたときに共有入力を配る)
		void CopyInputLink(
			Graphics::Pipeline::RenderGraph& a_graph,
			const Graphics::Pipeline::Pass& a_srcPass,
			Graphics::Pipeline::Pass& a_dstPass,
			const std::string& a_pinName);
	}
}
