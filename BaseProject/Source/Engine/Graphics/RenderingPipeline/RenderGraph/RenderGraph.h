#pragma once
//==========================================================================================
//
// RenderGraph (Engine::Graphics::Pipeline)
//
// パスの実体と、パス同士のつなぎを持ち、そこから実行順とリソースまで解決するクラス。
// 「何を持っているか」だけを担当し、ImGui / ImNodes には一切触らない。
// 編集UIは RenderingPipelineAsset 側の責務。
//
// 仮想リソース / 物理リソースの配列もここが持つ(マネージャークラスは置かない)。
//   Compile()           : 実行順の解決 + 仮想リソースの構築(GPU不要)
//   AllocateResources() : 仮想リソースの要件どおりに物理リソースを作る(GPU必要)
//
// 既存の Engine::Graphics::RenderGraph とは別物。
// 作り直し中のこちらは Engine::Graphics::Pipeline 名前空間に入っている。
//
//==========================================================================================
#include "../RenderingPipeline.h"
#include "VirtualResource/VirtualResource.h"
#include "PhysicalResource/PhysicalResource.h"

namespace Engine::Graphics::Pipeline
{
	class PassMetaRegistry;

	// =====================================================================================
	// リソースバリア(コンパイル時に計算済みのもの)
	// =====================================================================================
	struct ResourceBarrier
	{
		ResourceHandle handle = {};								// どの仮想リソースか

		// このバリアが触るスライス([0]=Current/書く側 [1]=Previous/読む側)。
		// Temporal でないリソースは常に 0
		uint32_t slice = 0;

		// 実体 : AllocateResources 後に埋まる。
		// Temporal は偶数フレームと奇数フレームで別の物理を触るので、両方を焼き込んでおく
		D3D12::GPUResource* pResource[2] = { nullptr, nullptr };

		D3D12_RESOURCE_STATES before = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_COMMON;

		// UAV -> UAV : ステートは変わらないが、前の書き込みの完了を待たせる必要がある
		bool isUAVBarrier = false;
	};

	// =====================================================================================
	// コンパイル済みのパス
	//
	// 実行順に一本で並ぶ。前から順に「バリアを張ってからパスを回す」だけでよい形にする
	// =====================================================================================
	struct CompiledPass
	{
		Pass* pPass = nullptr;							// 実体は m_passes が持つ
		std::vector<ResourceBarrier> preBarriers = {};	// このパスの直前に張るバリア

		//----------------------------------------------------------------------------------
		// 焼き込み済みの出力先
		//
		// 物理リソースが決まったところで引いておく。
		// 実行時にスロットから引き直すと、毎フレーム同じ探索を繰り返すことになる
		//----------------------------------------------------------------------------------
		// Temporal を含むと出力先がフレームで変わるので、偶数/奇数の2セットを焼き込む。
		// clear の対象や DSV の有無は並びが同じなので1つで足りる
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles[2] = {};
		std::vector<size_t> clearRtvIndices = {};		// rtvHandles のうちクリアするもの

		// 上と同じ並びのクリア色。
		// スロットが宣言した色(=生成時のクリアバリュー)をそのまま使わないと、
		// 透明で消したいレイヤーが不透明黒で塗られる(UIだけを描いた板の背景が真っ黒になる)
		std::vector<Math::Color> clearRtvColors = {};

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle[2] = { { 0 }, { 0 } };
		bool hasDSV = false;
		bool isDepthClear = false;

		//----------------------------------------------------------------------------------
		// 焼き込み済みのバインド
		//
		// スロットが指定したルートパラメータ番号ごとに、
		// ディスクリプタを連続領域へ並べておく。
		// 実行時は範囲を渡すだけで張れる
		//----------------------------------------------------------------------------------
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> descriptorTable[2] = {};
		std::vector<PassBind> binds = {};
	};

	// =====================================================================================
	// グラフの検証結果
	//
	// コンパイルの前に、繋ぎ方だけで分かる不備をまとめて拾う。
	// エラーが1つでもあるとコンパイルは通さない(中途半端な状態で走らせない)
	// =====================================================================================
	struct ValidationIssue
	{
		enum class ELevel
		{
			Warning,	// 走らせられるが意図と違う可能性がある
			Error		// このままでは走らせられない
		};

		ELevel level = ELevel::Error;
		Engine::GUID passGUID = {};		// 問題のあるパス(グラフ全体の話なら空)
		std::string message = "";
	};

	class RenderGraph
	{
	public:

		RenderGraph() = default;
		~RenderGraph() = default;

		// パスの実体を抱えるのでコピー禁止
		RenderGraph(const RenderGraph&) = delete;
		RenderGraph& operator=(const RenderGraph&) = delete;

		// パス・つなぎ・コンパイル結果をすべて捨てる
		void Clear();

		//----------------------------------------------------------------------------------
		// 設計図 / 実行インスタンス
		//
		// 同じクラスを2つの役で使う。
		//   設計図       : RenderingPipelineAsset が1つ持つ。編集と保存の対象
		//   実行インスタンス : カメラが1つずつ持つ。リソースの実体と実行順を持つ
		//
		// カメラごとにパスの実体を分けないと、2台が同じアセットを指したときに
		// GBuffer も定数バッファも取り合って壊れる
		//----------------------------------------------------------------------------------
		// 設計図のグラフから実行用のグラフを組み立てる。
		// パスは型IDから作り直して Archive 経由で中身を写すので、実体は別物になる
		bool BuildFrom(const RenderGraph& a_source, const PassMetaRegistry& a_registry);

		// 設計図側のパスのパラメータだけを写す。
		// グラフの形は変わらないので、組み直さずに済ませたいときに使う
		void SyncParamsFrom(const RenderGraph& a_source);

		// パス構成と配線の保存・読込。
		// 読込側はレジストリから型IDでパスを作り直す
		void Archive(Persistence::Archive& a_arch, const PassMetaRegistry& a_registry);

		//----------------------------------------------------------------------------------
		// パス
		//----------------------------------------------------------------------------------
		// レジストリの型IDから生成して並べる。
		// GUID発行・スロット宣言・ノード/ピンIDの確保までここで済ませる
		Pass* AddPass(const PassMetaRegistry& a_registry, ID<Pass> a_typeID);

		// 出入りするつなぎごと消す
		void RemovePass(const Engine::GUID& a_passGUID);

		Pass* FindPass(const Engine::GUID& a_passGUID);
		Pass* FindPassByNodeID(int a_nodeID);

		// ピンIDからパスを引く : 見つけたスロットと、それが入力側かどうかも返す
		Pass* FindPassByPinID(int a_pinID, Slot** a_ppOutSlot, bool* a_pOutIsInput);

		const std::vector<std::unique_ptr<Pass>>& GetPasses() const { return m_passes; }

		//----------------------------------------------------------------------------------
		// つなぎ
		//----------------------------------------------------------------------------------
		// 出力スロット -> 入力スロット をつなぐ。
		// 入力スロットは1本しか受けられないので、先に張られていた線は外してから張り直す。
		// 自分自身へのつなぎ・存在しないスロット・型やアクセスが噛み合わない組は張らずに false を返す
		bool Link(
			const Engine::GUID& a_srcPassGUID, uint32_t a_srcSlotID,
			const Engine::GUID& a_dstPassGUID, uint32_t a_dstSlotID);

		// 線1本を消す
		void RemoveLink(int a_linkID);

		// 指定した入力スロットに入っている線を外す
		void DisconnectInputSlot(const Engine::GUID& a_dstPassGUID, uint32_t a_dstSlotID);

		// キーは接続元(出力側)パスのGUID
		const std::unordered_map<Engine::GUID, std::vector<Connection>>& GetConnections() const { return m_connectionMap; }

		// つなぎの情報から各パスの入力スロットを張り直す。
		// 「今つながっている線」だけが正なので、一度全部外してから引き直す
		void ApplyLinks();

		//----------------------------------------------------------------------------------
		// リソース
		//----------------------------------------------------------------------------------
		// 描画解像度 : スロットの width / height が 0 のときの土台になる。
		// 実サイズは 「ここの値 × Slot::scale」 で決まる
		void SetViewportSize(UINT64 a_width, UINT a_height);
		UINT64 GetViewportWidth() const { return m_viewportWidth; }
		UINT GetViewportHeight() const { return m_viewportHeight; }

		// グラフの外で作られたリソースを名前で差し込む(バックバッファ・フレームリソースなど)。
		// 同じ名前で呼び直すと差し替えになる。Compile を通しても登録は残る
		void ImportResource(
			const std::string& a_name,
			D3D12::GPUResource* a_pResource,
			D3D12_RESOURCE_STATES a_initialState = D3D12_RESOURCE_STATE_COMMON,
			EPassSlotType a_type = EPassSlotType::Texture);

		// 差し込んでいた外部リソースの登録を外す
		void RemoveImportedResource(const std::string& a_name);
		void ClearImportedResources();

		// 仮想リソースの要件どおりに物理リソースを作る(または作り直す)。
		// Compile() の後に呼ぶこと
		bool AllocateResources(GraphicsEngine* a_pGraphicsEngine, D3D12::Device* a_pDevice);

		// 物理リソースの実体を破棄する。
		// ディスクリプタヒープにビューを持つので、
		// DescriptorHeapManager の解放より前に呼ぶこと
		void ReleaseResources();

		// 名前から仮想リソースを引く : 無ければ無効ハンドル
		ResourceHandle FindResource(const std::string& a_name) const;

		const VirtualResource* GetVirtualResource(ResourceHandle a_handle) const;
		VirtualResource* RefVirtualResource(ResourceHandle a_handle);

		// 割り当てられた実体 : まだ AllocateResources を通していなければ nullptr。
		// a_slice は [0]=Current(書く側) / [1]=Previous(読む側)
		PhysicalResource* RefPhysicalResource(ResourceHandle a_handle, uint32_t a_slice = 0) const;
		D3D12::GPUResource* RefGPUResource(ResourceHandle a_handle, uint32_t a_slice = 0) const;

		// スロットから、今のフレームで触るべき実体を引く。
		// Temporal なら出力は Current、入力は Previous を返す
		D3D12::GPUResource* RefGPUResource(const Slot& a_slot) const;

		//----------------------------------------------------------------------------------
		// フレーム
		//----------------------------------------------------------------------------------
		// 今のフレームの偶奇。Temporal の役割の入れ替えに使う
		uint32_t GetFrameParity() const { return m_frameIndex & 1u; }

		// このグラフに Temporal リソースが1つでもあるか
		bool HasTemporalResource() const;

		const std::vector<VirtualResource>& GetVirtualResources() const { return m_virtualResourceVec; }
		const std::vector<std::unique_ptr<PhysicalResource>>& GetPhysicalResources() const { return m_physicalResourceVec; }

		//----------------------------------------------------------------------------------
		// 検証
		//----------------------------------------------------------------------------------
		// 繋ぎ方だけで分かる不備を洗い出す。エラーが無ければ true。
		// Compile() の頭で必ず通るが、エディターから単体で呼んで結果を出してもよい
		bool Validate(std::vector<ValidationIssue>* a_pOutIssueVec = nullptr) const;

		// 直近の Validate / Compile が拾った不備
		const std::vector<ValidationIssue>& GetValidationIssues() const { return m_validationIssueVec; }

		// この出力スロットとこの入力スロットを繋いでよいか。
		// Link() が繋ぐ前に、エディターが繋ぎながら弾くのにも使う
		static bool IsConnectable(const Slot& a_srcSlot, const Slot& a_dstSlot, std::string* a_pOutReason = nullptr);

		// アクセスタイプが書き込み側か / 読み取り側か
		static bool IsWriteAccess(EAccessType a_accessType);
		static bool IsReadAccess(EAccessType a_accessType);

		//----------------------------------------------------------------------------------
		// 実行
		//----------------------------------------------------------------------------------
		// 入出力スロットから実行順を決めて仮想リソースを組み直し、
		// バリアを積んで、各パスの Compile まで通す。
		// 循環していたら false(そのときコンパイル結果は空になる)
		bool Compile();

		// コンパイル済みの順にパスを回す。
		// バリアの発行・レンダーターゲット切り替え・クリアまでここが面倒を見る
		void Execute(GraphicsEngine* a_pGraphicsEngine, RenderContext* a_pRenderContext);

		// 実行順に並んだパス
		const std::vector<CompiledPass>& GetCompiledPasses() const { return m_compilePasses; }

		// 全パスを通した後に張るバリア。
		// 各リソースをフレーム入口のステートへ戻すためのもので、
		// これを張らないと次のフレームのバリアの before がずれる
		const std::vector<ResourceBarrier>& GetEndBarriers() const { return m_endBarriers; }

		//----------------------------------------------------------------------------------
		// ノード/ピン/線で共有する連番ID
		// 保存済みのつなぎがピンIDで結ばれているので、振り直さないこと
		//----------------------------------------------------------------------------------
		int GenerateID() { return ++m_idCounter; }

	private:

		// 外部から差し込まれたリソースの控え。
		// Compile() が仮想リソース配列を作り直すので、元の情報はこちらに残しておく
		struct ImportedResource
		{
			std::string name = "";
			EPassSlotType type = EPassSlotType::Texture;
			D3D12::GPUResource* pResource = nullptr;
			D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
		};

		// パスの入出力スロットから仮想リソースを組み直す。
		// 名前ごとに1つ起こして、その名前を触っている全スロットの要件を足し込む
		void BuildVirtualResources();

		// 実行順が決まったあとに、各リソースの生存区間(最初/最後のパス)を出す
		void BuildResourceLifetimes();

		// 名前で引いて、無ければ作る
		VirtualResource& FindOrCreateVirtual(const std::string& a_name, const Slot& a_outputSlot);

		// スロットへ、割り当てた仮想リソースの参照を書き戻す
		void WriteBackSlotHandles();

		// 出力スロットの StoreOp を決める。
		// 後続のどこかが読んでいれば Store、誰も読まなければ DontCare
		void ResolveStoreOps();

		// 型IDからパスを作り直して、アーカイブから中身を流し込む。
		// 保存の読込と、設計図からの複製の両方がここを通る
		std::unique_ptr<Pass> CreatePassFromArchive(
			const PassMetaRegistry& a_registry, ID<Pass> a_typeID, Persistence::Archive& a_arch);

		//----------------------------------------------------------------------------------
		// バリア構築フェーズ
		//----------------------------------------------------------------------------------
		// 並んだパスを先頭から見て、各リソースのステート遷移を積む。
		// GPUには触らないので、実体(pResource)はここでは埋めない
		void BuildBarriers();

		// パス1つぶんのバリアを積む。
		// 読みを先に、書きを後に見て、ステートが変わるところで1本ずつ積んでいく
		void BuildPassBarriers(CompiledPass& a_compiledPass);

		// 積んであるバリアへ物理リソースのポインタを埋める(AllocateResources の後)
		void ResolveBarrierResources();

		// 宣言から焼き込んだヒープ・ルートシグネチャ・PSO・テーブルを張る
		void ApplyStaticBindings(RenderContext* a_pRenderContext, const CompiledPass& a_compiledPass, uint32_t a_parity);

		// 各パスの出力先(RTV/DSV)とクリア指定を焼き込む(AllocateResources の後)。
		// Temporal のぶんは偶数フレーム用と奇数フレーム用の両方を作る
		void ResolveDescriptors();

		// 割り当て直後の Temporal リソースを一度クリアしておく。
		// 1フレーム目は前フレームが無く、未初期化のメモリを読むことになるため
		void ClearTemporalResources(RenderContext* a_pRenderContext);

		// 全パスの Compile() を実行順に呼ぶ(リソースが揃ってから)
		void CompilePasses(GraphicsEngine* a_pGraphicsEngine);

		//----------------------------------------------------------------------------------
		// エイリアシング計算
		//----------------------------------------------------------------------------------
		void CalcHeapSize();			// ヒープのサイズを計算する

	private:

		// パスのインスタンス配列 : 実体はここが持つ
		std::vector<std::unique_ptr<Pass>> m_passes = {};

		// 接続線 : キーは接続元(出力側)パスのGUID
		std::unordered_map<Engine::GUID, std::vector<Connection>> m_connectionMap = {};

		// コンパイル後のパス配列(m_passes の中身を指すだけ)とバリア
		std::vector<CompiledPass> m_compilePasses = {};
		std::vector<ResourceBarrier> m_endBarriers = {};

		// ---- リソース ----

		// 仮想リソース : Compile のたびに組み直す
		std::unordered_map<std::string, uint32_t> m_resourceNameMap = {};
		std::vector<VirtualResource> m_virtualResourceVec = {};

		// 物理リソース : 今は仮想1つにつき1つなので、添字は仮想側と一致する
		std::vector<std::unique_ptr<PhysicalResource>> m_physicalResourceVec = {};

		// 外部から差し込まれたリソース
		std::vector<ImportedResource> m_importedResourceVec = {};

		// 描画解像度 : スロットのサイズが 0 のときの土台
		UINT64 m_viewportWidth = 0;
		UINT m_viewportHeight = 0;

		// 直近の検証結果
		mutable std::vector<ValidationIssue> m_validationIssueVec = {};

		// 実行したフレーム数 : Temporal の役割を入れ替えるのに使う
		uint32_t m_frameIndex = 0;

		// 割り当て直後に一度だけ Temporal をクリアする
		bool m_isTemporalClearPending = false;

		// ノード/ピン/線に配る連番
		int m_idCounter = 0;

		// エイリアシング
		uint32_t m_maxNum = 0;		// 最大同時使用数
		size_t m_maxSize = 0;		// 幅 * 高さ * フォーマットのサイズを足し合わせた最大数

		struct Aliasing
		{
			size_t maxSize = 0;			// 使われていた際の最大サイズ
			bool isUsed = false;		// 使われているかどうか
		};

		std::vector<Aliasing> m_aliasingVec = {};

	};
}
