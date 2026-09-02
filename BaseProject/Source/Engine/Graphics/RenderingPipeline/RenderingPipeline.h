#pragma once
//==========================================================================================
//
// RenderingPipeline
//
// レンダーグラフを構成する部品と、それを編集するノードエディタUI(ImNodes)。
//
// ・Pass  : 一つのシェーダーで一回のみの処理となる最小単位。入出力スロット(ピン)を宣言する
// ・Slot  : パスの入口/出口に置くリソース1本ぶんの情報
// ・Connection : 「どのパスのどの出力スロット」→「どのパスのどの入力スロット」のつなぎ
//
// パスとつなぎを持ち、実行順まで解決するのは RenderGraph の責務(RenderGraph/RenderGraph.h)。
// RenderingPipelineAsset はその RenderGraph を1つ持ち、編集UIを担当する。
//
// ノードと線の紐づけはパスごとの GUID(インスタンス固有)で行う。
// ID<Pass> はレジストリのクラス型IDなので、同じクラスを2つ置くと被る = インスタンスの識別には使えない。
//
// 名前空間を Engine::Graphics::Pipeline に分けているのは、
// 作り直し中のこちらと、既存の Engine::Graphics::RenderGraph を同じTUで共存させるため。
//
//==========================================================================================
// このヘッダーは ResourceManager 経由で EngineCommon の途中から取り込まれるため、
// GraphicCommon より先に読まれることがある。必要なものは明示的に含める
#include "../GraphicCommon.h"
#include "../ShadingPipelineBuilder/ShadingPipelineBuilder.h"

// 実行時に渡ってくるもの : このヘッダーでは中身を知らなくてよい
namespace Engine::Graphics
{
	class RenderContext;
	class GraphicsEngine;
}

namespace Engine::Graphics::Pipeline
{
	class PassMetaRegistry;
	class RenderGraph;

	// リソースのタイプ
	enum class EPassSlotType : UINT
	{
		Texture,
		Buffer
	};

	// レンダーターゲットをクリアするかどうか
	enum class ELoadOp : UINT
	{
		Load,			// クリアしない
		Clear			// クリアする
	};

	// レンダーターゲットがこのパス以降にしようされるかどうか
	// これは外部から設定はしない。基本的にDontCareで次に使われるパスがつながった際に動的にStorになる
	// まだ使用するかは未定だが、エイリアシング用に置いているため未使用
	enum class EStoreOp : UINT
	{
		Store,			// この先で使うことがある
		DontCare		// これ以上後ろで使われることがない
	};

	// アクセスタイプ : 同じリソースでも、アクセスタイプが異なる
	enum class EAccessType
	{
		None,
		SRV,
		RTV,
		Depth_Read,
		Depth_Write,
		UAV,
		CopySrc,
		CopyDst
	};

	// 仮想リソースの参照
	//
	// 中身は RenderGraph が持つ仮想リソース配列の添字だけ。
	// 実行順はピンの接続から決まるので、既存の RGResourceHandle のような
	// 世代(version)は持たせていない
	struct ResourceHandle
	{
		static constexpr uint32_t INVALID_INDEX = static_cast<uint32_t>(-1);

		uint32_t index = INVALID_INDEX;

		bool IsValid() const { return index != INVALID_INDEX; }

		bool operator==(const ResourceHandle& a_other) const { return index == a_other.index; }
		bool operator!=(const ResourceHandle& a_other) const { return !(*this == a_other); }
	};

	//======================================================================================
	// 仮想リソースの識別子
	//
	// 「どのパスのどの出力ピンが作ったリソースか」。
	//
	// 以前はリソース名(文字列)が同一性そのもので、「同じ名前 = 同じリソース」だった。
	// そのため同じパスクラスを2つ置くと出力名がぶつかり、黙って1枚のテクスチャへ
	// 相乗りしていた(ブラーの縮小段を並べるのに、エディターで名前を打ち分ける必要があった)。
	//
	// 作り手のインスタンスと出力ピンで決めれば、何個並べても勝手に別物になる。
	// パスの型ID(ID<Pass>)ではなくインスタンスのGUIDなのはそのため。
	// 型IDは同じクラスを2つ置くと被るので、インスタンスの識別には使えない。
	//
	// 名前は表示用のラベルとして残っているだけで、同一性には関わらない。
	// 例外はグラフの外から差し込まれるリソースで、
	// 差し込む側とパス側が名前で待ち合わせるしかないため、そこだけ名前から起こす
	//======================================================================================
	struct ResourceID
	{
		static constexpr uint32_t INVALID_SLOT_ID = static_cast<uint32_t>(-1);

		Engine::GUID passGUID = {};					// 作ったパス : 外部リソースなら空のまま
		uint32_t slotID = INVALID_SLOT_ID;			// その出力スロットID : 外部リソースなら名前のハッシュ

		bool IsValid() const { return slotID != INVALID_SLOT_ID; }

		bool operator==(const ResourceID& a_other) const
		{
			return slotID == a_other.slotID && passGUID == a_other.passGUID;
		}
		bool operator!=(const ResourceID& a_other) const { return !(*this == a_other); }

		// パスの出力ピンから起こす : 同じクラスを何個置いても別物になる
		static ResourceID FromOutputSlot(const Engine::GUID& a_passGUID, uint32_t a_slotID);

		// グラフの外から差し込まれるリソースから起こす。
		// 作り手のパスが居ないので GUID は空のまま。
		// パスの出力は必ず有効なGUIDを持つので、こちらとぶつかることはない
		static ResourceID FromImportName(const std::string& a_name);
	};

	// リソース、パスの入出力データ
	struct Slot
	{
		// リソースのステート
		std::string name = "";							// リソース名 : 入力側はつながって初めて埋まる
		EPassSlotType type = EPassSlotType::Texture;	// リソースタイプ
		EAccessType accessType = EAccessType::None;		// アクセスタイプ : このパスからどう触るか(自分の持ち物)
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;		// リソースフォーマット

		// リソースサイズ
		UINT64 width = 0;		// 横指定
		UINT height = 0;		// 縦指定
		float scale = 1.f;		// スケール指定 : 上記のサイズに対してかかる

		// リソースのパス間の設定
		ELoadOp loadOp = ELoadOp::Load;				// クリアは明示的に設定する必要がある
		EStoreOp storeOp = EStoreOp::DontCare;		// 後続が出れば自動で Store に変換

		// クリアする色 : loadOp == Clear のときに使う。
		// 生成時のクリアバリューにもなるので、出力スロット側で決める
		Math::Color clearColor = { 0.f, 0.f, 0.f, 1.f };

		// フレーム間でテンポラルするかどうか : パス間ではパスのつなぎ方で判別するためいらない
		bool isTemporal = false;

		// ランタイム時用
		// Compile() で、このスロットが指す仮想リソースが割り当てられる
		ResourceHandle resourceHandle = {};

		//----------------------------------------------------------------------------------
		// このスロットが指しているリソースの識別子
		//
		// 出力ピン : Compile の頭で「自分のパス + 自分のスロットID」が焼かれる。
		//            描き足すパスだけは、そのあと入力からもらったものへ差し替わる
		// 入力ピン : つながった相手の出力からそのまま写る
		//
		// name は表示用のラベル。名前が同じでも、これが違えば別のリソース
		//----------------------------------------------------------------------------------
		ResourceID resourceID = {};

		// グラフの外から差し込まれるリソースへ書き出す出力ピンなら、その名前。
		// 空でなければ識別子を名前から起こして、ImportResource 側と待ち合わせる
		std::string importName = "";

		//----------------------------------------------------------------------------------
		// スロットの識別
		//
		// 接続の保存・復元はこの slotID で行う。
		// 配列の添字を使うと、パスがスロットを1つ足しただけで
		// 保存済みの接続が別のスロットを指してしまう
		//----------------------------------------------------------------------------------
		uint32_t slotID = 0;		// pinName から作る安定ID : 並びが変わっても動かない

		//----------------------------------------------------------------------------------
		// シェーダーのどのルートパラメータへ張るか
		//
		// -1 なら張らない(RTV/DSV のように出力先として使うだけのスロット)。
		// 同じ番号を指定したスロットは、宣言した順に1つのディスクリプタテーブルへ並ぶ
		//----------------------------------------------------------------------------------
		int rootParamIndex = -1;

		// このスロットが繋がっていないとパスが成立しないか。
		// Validation で「必須Inputの未接続」として弾く
		bool isRequired = true;

		// エディター用
		bool isIn = true;			// InputSlot側かどうか
		std::string pinName = "";	// スロットの役割名(Albedo / Depth など) : 表示とID生成に使う
		int pinID = 0;				// ImNodes 上のピンID(実行には関係しない)

		// 入力ピンにリソースが流れ込んでいるか
		bool IsConnected() const { return !name.empty(); }
	};

	//======================================================================================
	// パスを編集した結果
	//
	// 設計図をエディターで触ったとき、カメラが回している実行インスタンスへ
	// どこまで反映し直す必要があるかを表す
	//======================================================================================
	enum class EPassEditResult : uint8_t
	{
		None,		// 何も変わっていない
		Param,		// パラメータだけ : 実行インスタンスへ値を写せばよい
		Structure	// リソースの要件が変わった : グラフごと組み直す
	};

	//======================================================================================
	// パスへ渡す実行コンテキスト
	//
	// パスが RenderGraph の中身を直接知らずに、自分のスロットから
	// GPUリソースへ辿り着けるようにするための入口。
	//
	// Compile 時は pGraph だけが入る(実行系はまだ無い)。
	// Update 時は pRenderContext / pCmdList も入る
	//======================================================================================
	struct PassContext
	{
		RenderGraph* pGraph = nullptr;						// 仮想/物理リソースを引く
		GraphicsEngine* pGraphicsEngine = nullptr;			// 描画アイテム・ライト・カメラを引く
		RenderContext* pRenderContext = nullptr;			// 実行時のみ
		D3D12::GraphicsCommandList* pCmdList = nullptr;		// 実行時のみ

		// スロットに割り当てられたGPUリソースを引く : 未割り当てなら nullptr
		D3D12::GPUResource* GetResource(const Slot& a_slot) const;
	};

	// パスが使うパイプラインの種類
	enum class EPassPipelineType : uint8_t
	{
		Graphics,
		Compute
	};

	// 実行前にグラフが張るディスクリプタヒープ
	enum class EPassHeapMode : uint8_t
	{
		None,					// パス側で張る
		Default,				// 通常のヒープ
		BindlessWithSampler		// バインドレス + サンプラー
	};

	// 焼き込み済みのバインド1件
	struct PassBind
	{
		enum class EType : uint8_t { SrvTable, Uav };

		EType type = EType::SrvTable;
		UINT rootIndex = 0;
		uint16_t firstHandle = 0;	// descriptorTable への開始添字
		uint16_t count = 1;
	};

	// 接続線 : 条件はなし
	// どちら側のピンにつないだかまで持たないと、パスが複数ピンを持ったときに線を引き直せない
	struct Connection
	{
		int linkID = 0;							// この接続線のID
		Engine::GUID dstPassGUID = {};			// 接続先パスのGUID
		uint32_t srcSlotID = 0;					// 出力側(線の根本)のスロットID
		uint32_t dstSlotID = 0;					// 入力側(線の先)のスロットID

		void Archive(Persistence::Archive& a_arch);						// 保存
		void EditConnection(int a_srcOutPinID, int a_dstInPinID) const;		// ImNodeへ線を登録する
	};

	// グラフの構成要素 : 一つのシェーダーで一回のみの処理となる最小単位
	// 継承先で定数バッファのデータや、アーカイブ処理、GPU処理を入れる
	class Pass
	{
	public:

		Pass() = default;
		virtual ~Pass() = default;

		// 初期化 : GUIDを振って、継承先のスロット宣言を走らせる
		void Init();

		//----------------------------------------------------------------------------------
		// シェーディングモデル表を引くときの名前
		//
		// モデルを受け取るパスは、マテリアルのシェーディングモデル表から
		// 自分用のピクセルシェーダーを引く。その鍵がこの名前。
		//
		// 表示名(GetName)とは別にしてあるのは、表示名がエディターで自由に変えられるから。
		// 表示名で引くと、ノードの名前を変えただけでシェーダーが見つからなくなり、
		// ピクセルシェーダー無しのPSOが焼かれて真っ黒になる
		//----------------------------------------------------------------------------------
		virtual const char* GetShadingPassName() const { return nullptr; }

		// 継承先で自分の入出力スロットを宣言する
		// DeclareInput / DeclareOutput をここから呼ぶ
		virtual void SetupSlots() {}

		// 配線が確定した直後(コンパイルの頭)に呼ばれる。
		// 「前段が居るかどうかで振る舞いが変わる」パスがここで自分のスロットを整える。
		// 例 : ZPre が前に居るなら GBuffer は深度をクリアしない
		virtual void OnLinksResolved() {}

		// システム上で前のパスができて、パスのアウトプットスロットから接続された際に呼ばれる
		// ピン名とデータを指定して、コンパイル時ようにためておく。
		// データは前のパスに依存するため、このパスでフォーマットなどは指定できない。
		void SetInput(uint32_t a_slotID, const Slot& a_slotData);
		void SetInput(const std::string& a_pinName, const Slot& a_slotData);	// 名前から引く版

		// 入力スロットのつなぎを外す : 線を消したときに呼ぶ
		void ClearInput(uint32_t a_slotID);
		void ClearInput(const std::string& a_pinName);

		// コンパイル : パスの設定されている情報からランタイムデータを構築する。
		// 物理リソースが割り当てられた後に呼ばれるので、ここでディスクリプタまで引ける
		virtual void Compile(const PassContext& a_context) = 0;

		// ランタイム中はこの関数のみで処理する。
		// バリア・レンダーターゲット切り替え・クリアはグラフ側が済ませてある
		virtual void Update(const PassContext& a_context) = 0;

		// エディター用
		//
		// EditUpdate は「どこまで反映し直す必要があるか」を返す。
		//   Param     : 値を写すだけでよいもの(色・強度など)
		//   Structure : フォーマットやスケールのようにリソースの要件が変わるもの
		// 返し忘れると、設計図だけ変わって画面が変わらない状態になる
		virtual EPassEditResult EditUpdate() = 0;		// パスの情報を編集する用
		virtual void EditNode() = 0;		// パスのノード情報を編集する用

		// シリアライズ
		//
		// パス1つぶんの入口。共通部分(名前/GUID/ノード・ピンID/座標)をここで処理して、
		// 継承先固有のデータは Archive() へ委譲する。
		// 保存にも、設計図から実行用パスを複製するのにも同じ経路を通る
		void ArchivePass(Persistence::Archive& a_arch);

		virtual void Archive(Persistence::Archive& a_arch) = 0;		// パス固有データの保存、読込関数

		//----------------------------------------------------------------------------------
		// コンパイル(並べ替え) / 接続用
		//----------------------------------------------------------------------------------
		// 入出力スロットの参照 : どのリソースを食って、どのリソースを吐くか
		const std::vector<Slot>& GetInputSlots() const { return m_inputSlots; }
		const std::vector<Slot>& GetOutputSlots() const { return m_outputSlots; }

		// 書き換え用 : RenderGraph が Compile 時に resourceHandle を書き戻すために使う。
		// パスの外からスロットの中身を書き換えるのはここだけにすること
		std::vector<Slot>& RefInputSlots() { return m_inputSlots; }
		std::vector<Slot>& RefOutputSlots() { return m_outputSlots; }

		// スロットIDから引く : 無ければ nullptr
		Slot* FindInputSlot(uint32_t a_slotID);
		const Slot* FindInputSlot(uint32_t a_slotID) const;
		Slot* FindOutputSlot(uint32_t a_slotID);
		const Slot* FindOutputSlot(uint32_t a_slotID) const;

		// 名前から引く(宣言側・エディター表示用の便利版)
		Slot* FindInputPin(const std::string& a_pinName);
		Slot* FindOutputPin(const std::string& a_pinName);
		const Slot* FindOutputPin(const std::string& a_pinName) const;

		// スロット名から安定IDを作る
		static uint32_t MakeSlotID(const std::string& a_pinName);

		// ImNodes のピンIDから引く : 入力側なら a_pOutIsInput に true が入る
		Slot* FindSlotByPinID(int a_pinID, bool* a_pOutIsInput = nullptr);

		//----------------------------------------------------------------------------------
		// メタ
		//----------------------------------------------------------------------------------
		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& a_name) { m_name = a_name; }

		const Engine::GUID& GetGUID() const { return m_passGUID; }		// インスタンスの識別子
		ID<Pass> GetTypeID() const { return m_passID; }					// レジストリのクラス型ID
		void SetTypeID(ID<Pass> a_id) { m_passID = a_id; }

		//----------------------------------------------------------------------------------
		// エディター
		//----------------------------------------------------------------------------------
		// ノード/ピンのIDを確保する : すでに振られているものはそのまま(ロード後の追加宣言用)
		void EnsureEditorIDs(const std::function<int()>& a_generateID);

		//----------------------------------------------------------------------------------
		// モデル描画
		//----------------------------------------------------------------------------------
		// このパスがモデルを受け取るか、受け取るならどちらのキューか。
		// 振り分けはマテリアルの透明モードで決まる(シェーディングモデルは使わない)
		EGeometryQueue GetGeometryQueue() const { return m_geometryQueue; }

		// このパスで使うピクセルシェーダー : 空なら深度だけ書くパス
		const Handle<Resource::Shader>& GetDefaultPSHandle() const { return m_defaultPSHandle; }

		// PSOの組み立て。出力フォーマットが確定したところでグラフが Init を呼ぶ
		ShadingPipelineBuilder& RefPipelineBuilder() { return m_pipelineBuilder; }

		//----------------------------------------------------------------------------------
		// グラフが実行前に張るもの
		//
		// ここに入れておくと、パスの Update() は
		// 「定数バッファを詰めて Dispatch/Draw する」だけで済む
		//----------------------------------------------------------------------------------
		EPassPipelineType GetPipelineType() const { return m_pipelineType; }
		EPassHeapMode GetHeapMode() const { return m_heapMode; }
		const Handle<ID3D12RootSignature>& GetRootSignature() const { return m_rootSigHandle; }

		// PSOはハンドルで持つ。
		//
		// 8bitの添字へ落として持つと、PSOの通し番号が256を超えたところで
		// 別のPSOへすり替わる(コンピュートのつもりでグラフィックスのPSOを張る等)。
		// パイプラインはパスの数だけPSOを作るので、旧経路と合わせるとすぐ届く
		const Handle<ID3D12PipelineState>& GetPSOHandle() const { return m_psoHandle; }

		// 描画アイテムのソートキーに入るパス番号。
		// グラフのコンパイル時に GraphicsEngine から配られる
		uint8_t GetPassIndex() const { return m_passIndex; }
		void SetPassIndex(uint8_t a_index) { m_passIndex = a_index; }

		int GetNodeID() const { return m_nodeID; }
		void SetNodeID(int a_nodeID) { m_nodeID = a_nodeID; }

		// ロード・複製でGUIDを引き継ぐ : ノードと線の紐づけがGUIDなので必ず写す
		void SetGUID(const Engine::GUID& a_guid) { m_passGUID = a_guid; }
		const Math::Vector2& GetEditorPos() const { return m_editorPos; }
		void SetEditorPos(const Math::Vector2& a_pos) { m_editorPos = a_pos; }

	protected:

		//----------------------------------------------------------------------------------
		// スロット宣言 : 継承先の SetupSlots から呼ぶ
		//----------------------------------------------------------------------------------
		// 入力ピン : リソースの中身はつながった相手からもらうので、ここでは役割だけ決める
		//
		// a_isTemporal を立てると「前のフレームの結果を読むピン」になる。
		// 実行順の辺にならないので自分自身の出力へ繋げられる(TAAの履歴など)。
		// 立てないピンは今フレームの書き込み結果を読む
		Slot& DeclareInput(
			const std::string& a_pinName,
			EAccessType a_accessType = EAccessType::SRV,
			EPassSlotType a_type = EPassSlotType::Texture,
			bool a_isRequired = true,
			int a_rootParamIndex = -1,
			bool a_isTemporal = false);

		//----------------------------------------------------------------------------------
		// 入力ピンに来たリソースを、そのまま自分の出力先として引き継ぐ
		//
		// 「どのリソースへ描くか」だけを合わせる版。繋がっていなければ何もせず false。
		//
		// リソースの同一性が名前ではなく識別子になったので、
		// 「前段と同じリソースへ描く」つもりのパスは必ずここを通すこと。
		// 出力名を前段と揃えるだけでは、もう合流しない
		//----------------------------------------------------------------------------------
		bool AliasOutputToInput(const std::string& a_inPinName, const std::string& a_outPinName);

		//----------------------------------------------------------------------------------
		// 描き足すパス用 : 入力ピンに来たリソースを、そのまま出力先にする
		//
		// 「前段の絵の上に重ねる」パスは、書く先が前段のリソースそのもの。
		// 配線が決まらないと相手が分からないので、OnLinksResolved から呼ぶ。
		// 上の引き継ぎに加えて、相手が居ないときの「まず消してから描く」まで面倒をみる
		//----------------------------------------------------------------------------------
		void FollowInputToOutput(const std::string& a_inPinName, const std::string& a_outPinName);

		//----------------------------------------------------------------------------------
		// コンピュートシェーダーの用意
		//
		// 継承先の Compile() から呼ぶ。
		// シェーダーからルートシグネチャを起こし、PSOまで作って控える
		//----------------------------------------------------------------------------------
		//----------------------------------------------------------------------------------
		// ラスタ(頂点+ピクセル)シェーダーの用意
		//
		// 出力フォーマットは、このパスのRTVスロットから引く。
		// 深度やブレンドの設定は a_configure で書き換える
		//----------------------------------------------------------------------------------
		bool SetupRasterShader(
			const PassContext& a_context,
			const std::string& a_vsPath,
			const std::string& a_psPath,
			const D3D12_INPUT_LAYOUT_DESC& a_inputLayout,
			const std::string& a_psoName,
			const std::function<void(D3D12::GraphicsPipelineDesc&)>& a_configure = nullptr,
			EPassHeapMode a_heapMode = EPassHeapMode::Default,
			// PSOの受け取り先。
			// 渡すと m_psoHandle には入れないので、グラフは自動でPSOを張らない。
			// ブレンド違いを複数持って描くときに選び分けるパス(パーティクル)で使う
			Handle<ID3D12PipelineState>* a_pOutPSOHandle = nullptr);

		bool SetupComputeShader(
			const PassContext& a_context,
			const std::string& a_csPath,
			const std::string& a_psoName,
			EPassHeapMode a_heapMode = EPassHeapMode::Default);

		//----------------------------------------------------------------------------------
		// 画面全体を回すディスパッチ
		//
		// 8x8 のタイルで割り切れないぶんを切り上げる。
		// 切り捨てると末尾のタイルが実行されず、画面の端が処理されない
		//----------------------------------------------------------------------------------
		void DispatchFullScreen(const PassContext& a_context) const;

		// 指定スロットのリソース解像度で回す。
		// 縮小段のように入出力で解像度が違うパスで使う
		void DispatchForSlot(const PassContext& a_context, const Slot& a_slot) const;

		// 出力ピン : このパスが作るリソースなので、フォーマットまでここで決める
		// a_isTemporal を立てると、このリソースはフレーム間で入れ替わる2枚組になる。
		// 前フレームの結果を読みたい履歴バッファ(TAA History など)に使う
		Slot& DeclareOutput(
			const std::string& a_pinName,
			const std::string& a_resourceName,
			DXGI_FORMAT a_format,
			EAccessType a_accessType = EAccessType::RTV,
			EPassSlotType a_type = EPassSlotType::Texture,
			bool a_isTemporal = false,
			int a_rootParamIndex = -1);

		//----------------------------------------------------------------------------------
		// グラフの外から差し込まれるリソースへ書き出す出力ピン
		//
		// 実体は RenderGraph::ImportResource() で差し込まれるので、
		// フォーマットも大きさも向こうの持ち物。
		// 識別子だけは名前から起こして、差し込む側と待ち合わせる
		//----------------------------------------------------------------------------------
		Slot& DeclareImportedOutput(
			const std::string& a_pinName,
			const std::string& a_importName,
			EAccessType a_accessType = EAccessType::CopyDst);

		// ---- パス情報 ----
		// メタ
		std::string m_name = "";
		Engine::GUID m_passGUID = {};	// インスタンス固有 : ノード/線の紐づけはこれで行う
		ID<Pass> m_passID;				// レジストリのクラス型ID

		// シェーダー
		Engine::GUID m_shaderGUID = {};
		Handle<Resource::Shader> m_shaderHandle = {};

		// リソース
		std::vector<Slot> m_inputSlots = {};		// 入力
		std::vector<Slot> m_outputSlots = {};		// 出力

		// ---- モデル描画 ----
		EGeometryQueue m_geometryQueue = EGeometryQueue::None;
		Handle<Resource::Shader> m_defaultPSHandle = {};
		ShadingPipelineBuilder m_pipelineBuilder = {};
		uint8_t m_passIndex = 255;		// 未割り当て

		// ---- グラフが実行前に張るもの ----
		EPassPipelineType m_pipelineType = EPassPipelineType::Graphics;
		EPassHeapMode m_heapMode = EPassHeapMode::None;
		Handle<ID3D12RootSignature> m_rootSigHandle = {};
		Handle<ID3D12PipelineState> m_psoHandle = {};

		// ---- エディター用情報 ----
		// ノード
		Math::Vector2 m_editorPos = {};
		int m_nodeID = 0;			// ノード自身のID
	};

	// レンダーグラフ1本ぶんを持つアセット
	// カメラごとに参照する
	//
	// パス・つなぎ・実行順は RenderGraph の持ち物で、ここは
	// 「グラフを1つ抱えて、それを編集するUIを出す」役に徹する
	class RenderingPipelineAsset
	{
	public:

		RenderingPipelineAsset();
		~RenderingPipelineAsset();

		// ImNodesEditorContext* を生ポインタで所有するためコピー禁止。
		// ムーブは許可する : ResourceManager のプールは Add(T&&) で実体をムーブ代入するので、
		// ムーブできないとプールに載せられない
		RenderingPipelineAsset(const RenderingPipelineAsset&) = delete;
		RenderingPipelineAsset& operator=(const RenderingPipelineAsset&) = delete;
		RenderingPipelineAsset(RenderingPipelineAsset&& a_other) noexcept;
		RenderingPipelineAsset& operator=(RenderingPipelineAsset&& a_other) noexcept;

		//----------------------------------------------------------------------------------
		// シリアライズ
		//----------------------------------------------------------------------------------
		// パス構成と配線を保存・読込する。
		// 読込はレジストリから型IDでパスを作り直すので、必ず SetMetaRegistry の後に呼ぶこと
		void Archive(Persistence::Archive& a_arch);

		// 拡張子なしのベースパスへ書き出す
		void Save(const std::string& a_baseFilePath);

		// 表示名 : アセットのファイル名がそのまま入る
		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& a_name) { m_name = a_name; }

		// このアセットの拡張子(.oj / .ob の後ろに付く)
		static constexpr const char* kExtension = "rpipe";

		// 生成できるパスの一覧をもらい受ける : 持ち主から渡す(シングルトン直引きはしない)
		void SetMetaRegistry(PassMetaRegistry* a_pRegistry) { m_pMetaRegistry = a_pRegistry; }

		//----------------------------------------------------------------------------------
		// グラフの出口
		//
		// FinalOutputPass はどのパイプラインにも必ず1つ常駐する。
		// 無ければここで足すので、読み込み直後と編集の入口で必ず通すこと
		//----------------------------------------------------------------------------------
		void EnsureFinalPass();

		// このパスがグラフの出口か(削除させないための判定)
		bool IsFinalPass(const Pass& a_pass) const;

		// 中身のグラフ
		RenderGraph* RefRenderGraph() { return m_upRenderGraph.get(); }
		const RenderGraph* GetRenderGraph() const { return m_upRenderGraph.get(); }

		// 並べ替える : 中身は RenderGraph 側。
		// 通すと Dirty は下りる
		void Compile();

		//----------------------------------------------------------------------------------
		// 構成が変わったかどうか
		//
		// 構成をいじれるのはエディターだけなので、いじったところで立てて
		// Compile ボタンで下ろすだけの簡単なものにしてある。
		// ランタイム中にグラフを組み替えることは想定していない
		//----------------------------------------------------------------------------------
		void SetDirty() { m_isDirty = true; ++m_structureVersion; }
		bool IsDirty() const { return m_isDirty; }

		// 構成が変わるたびに上がる版。
		// カメラごとの実行インスタンスは、これが変わったときだけ組み直す。
		// 0 は「まだ何も組んでいない」印として使うので 1 から始める
		uint32_t GetStructureVersion() const { return m_structureVersion; }

		// パラメータだけが変わるたびに上がる版。
		// カメラ側はこれが変わったら、組み直さずに値を写すだけで済む
		uint32_t GetParamVersion() const { return m_paramVersion; }


		//==================================================================================
		// エディター
		//==================================================================================

		void DrawEditor();

		//----------------------------------------------------------------------------------
		// ImNodesコンテキスト管理(グラフごとに独立して複数開けるように専用で持つ)
		//----------------------------------------------------------------------------------
		void EnsureContext();			// コンテキスト生成
		void DestroyContext();			// コンテキスト破棄

		// ロード直後に呼ぶ : メインスレッド外から障らないための遅延反映
		void RequestApplyLoadPositions() { m_applyPositions = true; }

		// セーブ直後に呼ぶ : ImNodes 上の現在座標をノードへ書き戻す
		void SyncPositions();

	private:

		//----------------------------------------------------------------------------------
		// エディター内部
		//----------------------------------------------------------------------------------
		void DrawValidation();				// 検証結果の一覧
		void DrawNodeEditor();				// ノードエディタ本体
		void DrawNode(Pass& a_pass);		// ノード1つ分の枠とピン
		void DrawAddPass();					// パス追加ボタン + ポップアップ
		void DrawSelectedPassDetail();		// 選択中パスの詳細(EditUpdate)

		void HandleCreateLink();			// 線が引かれたときの処理
		void HandleDeleteSelection();		// Delete キーでの削除

		// パスを追加して、ノード座標を ImNodes 側へ反映するところまで
		void AddPassFromEditor(ID<Pass> a_typeID);

		// 表示名
		std::string m_name = "RenderingPipeline";

		// 生成できるパスの一覧(所有しない)
		PassMetaRegistry* m_pMetaRegistry = nullptr;

		// パス・つなぎ・実行順を持つグラフ本体
		std::unique_ptr<RenderGraph> m_upRenderGraph = nullptr;

		// ---- エディター用 ----

		ImNodesEditorContext* m_context = nullptr;
		bool m_applyPositions = false;			// Load後、次のDrawで座標反映する

		// 構成が変わってからコンパイルを通していない。
		// 読み込み直後は一度通す必要があるので true から始める
		bool m_isDirty = true;

		// 構成の版
		uint32_t m_structureVersion = 1;

		// パラメータの版
		uint32_t m_paramVersion = 1;
		Engine::GUID m_pendingDeletePass = {};	// このフレーム内で削除予約されたパス
	};
}

namespace std
{
	// 仮想リソースの対応表の鍵に使う
	template<>
	struct hash<Engine::Graphics::Pipeline::ResourceID>
	{
		size_t operator()(const Engine::Graphics::Pipeline::ResourceID& a_id) const noexcept
		{
			const size_t _hash = a_id.passGUID.Hash();
			return _hash ^ (static_cast<size_t>(a_id.slotID)
				+ 0x9e3779b97f4a7c15ull + (_hash << 6) + (_hash >> 2));
		}
	};
}
