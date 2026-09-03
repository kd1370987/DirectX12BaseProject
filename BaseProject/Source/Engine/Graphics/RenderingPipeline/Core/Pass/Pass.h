#pragma once
//==========================================================================================
//
// Pass (Engine::Graphics::Pipeline)
//
// グラフの構成要素 : 一つのシェーダーで一回のみの処理となる最小単位。
// 入出力スロット(ピン)を宣言し、コンパイルと実行の中身を継承先で書く。
//
// パスを1つ足すときに要るのはこのヘッダーだけ。
// つなぎ(Connection)も編集UI(RenderingPipelineAsset)もパス側は知らなくてよい
//
//==========================================================================================
// このヘッダーは ResourceManager 経由で EngineCommon の途中から取り込まれるため、
// GraphicCommon より先に読まれることがある。必要なものは明示的に含める
#include "../../../GraphicCommon.h"
#include "../../../ShadingPipelineBuilder/ShadingPipelineBuilder.h"

#include "../PipelineEnums.h"
#include "../ResourceID.h"
#include "../Slot.h"
#include "../PassContext.h"

namespace Engine::Graphics::Pipeline
{
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
}
