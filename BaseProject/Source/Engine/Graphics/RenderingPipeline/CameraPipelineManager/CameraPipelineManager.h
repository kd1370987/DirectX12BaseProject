#pragma once
#include "../../CBData.h"
#include "../../GraphicCommon.h"

namespace Engine::ECS
{
	class World;
}

namespace Engine::Resource
{
	class Texture;
}

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderContext;

	namespace Pipeline
	{
		class GraphicsPipeline;
		class RenderingPipelineAsset;
		class RenderGraph;
		class Pass;
	}

	//==========================================================================================
	// カメラ1台ぶんの描画要求
	//
	// 毎フレーム積む。積まれなかったカメラは消えたものとして、
	// フレームの終わりに実行インスタンスごと捨てる
	//==========================================================================================
	struct CameraSubmitDesc
	{
		// このカメラを識別する鍵。
		// エンティティの添字はワールドごとに振り直されるので、ワールドと組で持つ
		const ECS::World* pWorld = nullptr;
		uint32_t entity = 0;

		// 使う描画構成(設計図)
		Handle<Pipeline::RenderingPipelineAsset> pipelineHandle = {};

		Math::Matrix worldMat = Math::Matrix::Identity();
		Math::Matrix projMat = Math::Matrix::Identity();

		// 0 なら画面の描画解像度に追従する
		UINT viewportWidth = 0;
		UINT viewportHeight = 0;

		int order = 0;			// 小さいものから回す
		bool isMain = false;	// 画面に出るカメラか
	};

	//==========================================================================================
	// カメラごとの描画構成
	//
	// 描画構成(パイプライン)を持つカメラは、それぞれ自分の最終出力テクスチャへ描く。
	// 画面へ出すのはメインカメラの絵だけで、それ以外はモニターなどから引いて使う。
	// 描画構成を持たないカメラは何も描かない。
	//
	// フレームの流れ(GraphicsEngine から呼ばれる)
	//   BeginFrame … 設計図が変わったカメラを組み直し、モデルを受け取るパスの一覧を作る
	//   Execute    … 積まれたカメラを順に回す
	//   PresentTo  … メインカメラの絵をバックバッファへ写す
	//   EndFrame   … 積まれなかったカメラを捨てる
	//==========================================================================================
	class CameraPipelineManager
	{
	public:

		// パスが出力先として使うリソース名。
		// この名前で出力スロットを宣言したパスが、カメラの最終出力へ描くことになる
		static constexpr const char* kCameraOutputName = "CameraOutput";

		// 持ち主(描画まわりの共有物を引く先)を受け取る
		void Init(GraphicsEngine* a_pGraphicsEngine);
		void Release();

		//--------------------------------------------------------------------------------------------
		// 外から使うもの
		//--------------------------------------------------------------------------------------------
		// 毎フレーム積む。積まなかったカメラはフレームの終わりに捨てられる
		void SubmitCamera(const CameraSubmitDesc& a_desc);

		// このカメラが描いた絵。モニターへ映したいときはこれを引く
		const Resource::Texture* GetCameraFinalTexture(const ECS::World* a_pWorld, uint32_t a_entity) const;

		//--------------------------------------------------------------------------------------------
		// 設計図のパスに対応する、実際に動いている実行インスタンスのパスを引く
		//
		// エディターが触っているのは設計図(RenderingPipelineAsset)側のパスで、
		// これは一度も実行されない = リソースの実体を持たない。
		// モニターのように「今フレーム流れている中身」をノードに出すパスは、
		// ここを通して実行インスタンス側の自分を借りてくる。
		//
		// GUIDは BuildFrom の複製で引き継がれるので、これが設計図と実行を結ぶ鍵になる。
		// 同じ設計図を複数のカメラが使っていればメインカメラのものを返す
		//--------------------------------------------------------------------------------------------
		Pipeline::Pass* FindPipelinePass(const Engine::GUID& a_passGUID) const;

		//--------------------------------------------------------------------------------------------
		// 直近に画面を作っていたカメラの描画構成
		//
		// ゲームのシーンを止めて別のワールドを描く画面(エフェクトエディター)が、
		// 「本番と同じ絵作り」で見るために借りる。
		// 一度もカメラが積まれていなければ無効ハンドル
		//--------------------------------------------------------------------------------------------
		const Handle<Pipeline::RenderingPipelineAsset>& GetLastMainPipelineHandle() const { return m_lastMainPipelineHandle; }

		//--------------------------------------------------------------------------------------------
		// エディター用 : 今フレーム回っているカメラのグラフを順に見る
		//
		// リソースの中身を覗くパネルが使う。
		// 実行インスタンスはカメラごとにあるので、どのカメラのものかが分かるよう
		// 表示用の名前を添えて返す
		//--------------------------------------------------------------------------------------------
		struct PipelineGraphView
		{
			std::string name = {};								// 表示名(設計図の名前 + メインかどうか)
			const Pipeline::RenderGraph* pGraph = nullptr;		// そのカメラの実行グラフ
		};
		std::vector<PipelineGraphView> CollectPipelineGraphs() const;

		//--------------------------------------------------------------------------------------------
		// 画面へ出す絵ができているか
		//
		// 画面に出るカメラに描画構成が設定されていて、組み上がっているときだけ true。
		// カメラが描画構成を持たなければ何も描かれない
		//--------------------------------------------------------------------------------------------
		bool IsPipelinePresentActive() const;

		// 画面へ出す絵。組み上がっていなければ nullptr
		const Resource::Texture* GetPresentTexture() const;

		//--------------------------------------------------------------------------------------------
		// フレームの流れ(GraphicsEngine から呼ぶ)
		//--------------------------------------------------------------------------------------------
		void BeginFrame();
		void Execute(RenderContext* a_pRenderContext);
		void PresentTo(D3D12::GraphicsCommandList* a_pCmdList);
		void EndFrame();

		//--------------------------------------------------------------------------------------------
		// モデルを受け取るパスの一覧(全カメラぶん)
		//
		// 描画アイテムはサブセット1つごとにパスの数だけ積むので、この一覧は
		// 1フレームに何万回も引かれる(DrawSubmitter)。毎回集め直すと積み込みが
		// それだけで重くなるため、フレームの頭で1回作って引くだけにする
		//--------------------------------------------------------------------------------------------
		const std::vector<Pipeline::Pass*>& GetGeometryPasses(EGeometryQueue a_queue) const;

	private:

		//--------------------------------------------------------------------------------------------
		// 設計図が変わったカメラの実行インスタンスを組み直す(フレームの頭)
		//
		// 必ず「描画アイテムを1つも積んでいないうち」に通すこと。
		// 組み直すとパス番号が配り直されるので、積んだ後にやると
		// アイテムのパス番号と食い違い、他所のパスのPSOで描いてデバイスが飛ぶ。
		//
		// a_isNewOnly を立てるとフレームの途中でも呼べる。
		// まだ実行インスタンスを持っていないカメラ(= このフレームのアイテムが
		// 1つも向いていないカメラ)だけを組むので、既存の番号を動かさない
		//--------------------------------------------------------------------------------------------
		void Rebuild(bool a_isNewOnly);

		// 積まれなかったカメラを捨てる(フレームの終わり)
		void Prune();

		// モデルを受け取るパスへパス番号を配り直す。
		// 取りっぱなしにすると組み直しのたびに番号が枯れるので、
		// どこか1つでも組み直したら全カメラぶんをまとめて配る
		void AssignPassIndices();

		// モデルを受け取るパスの一覧を作り直す
		void RefreshGeometryPassCache();

	private:

		//--------------------------------------------------------------------------------------------
		// カメラ1台ぶんの描画データ
		//
		// パイプラインアセット(設計図)はリソースマネージャーが持ち、カメラはハンドルで参照する。
		// 実行用のグラフ(upRenderGraph)はカメラごとに1つ作る :
		// 設計図をそのまま回すと、同じアセットを指した2台が GBuffer も定数バッファも
		// 取り合って壊れるため。RenderGraph::BuildFrom() で設計図から複製する。
		//
		// 最終出力はどのカメラも自前のテクスチャへ描き、
		// メインカメラのものだけを最後にバックバッファへコピーする。
		// こうしておくとパス側は「メインかどうか」を知らなくてよく、
		// 同じアセットをメインにもモニターにも使い回せる
		//--------------------------------------------------------------------------------------------
		struct CameraPipelineData
		{
			// unique_ptr の中身(GraphicsPipeline)がこのヘッダーでは不完全型なので、
			// 生成と破棄は .cpp 側(完全型が見える場所)に置く
			CameraPipelineData();
			~CameraPipelineData();

			// このカメラを識別する鍵
			const ECS::World* pWorld = nullptr;
			uint32_t entity = 0;

			// このカメラ専用の定数バッファ用の置き場(今はどのパスも読んでいない)
			CameraData cpuData = {};
			CameraData gpuData = {};

			// 使用するパイプラインのハンドル(設計図・所有しない)
			Handle<Pipeline::RenderingPipelineAsset> pipelineHandle = {};

			// このカメラ専用の実行インスタンス。
			// pipelineHandle の中身が差し替わったら作り直す
			std::unique_ptr<Pipeline::GraphicsPipeline> upPipeline = nullptr;

			// このカメラの最終出力
			std::unique_ptr<Resource::Texture> upFinalTex = nullptr;

			// 組み直しの判定用。
			// 設計図をエディターで触ると版が上がるので、そのときだけ作り直す
			uint32_t builtStructureVersion = 0;

			// 組めなかったことを知らせた版。
			// 失敗すると毎フレーム組み直しに来るので、同じ版で何度も言わないための印
			uint32_t reportedFailVersion = 0;
			uint32_t builtParamVersion = 0;
			UINT builtWidth = 0;
			UINT builtHeight = 0;

			// 今フレーム積まれたか。積まれなかったものは捨てる
			bool isSubmitted = false;

			// 描画する順番 : モニターに映すカメラを先に回してから本編を描く、といった並べ替え用
			int order = 0;
			bool isMain = false;
		};

		// 持ち主(借り物)。デバイス・ヒープ・リソース・パスの型情報・バックバッファを引く
		GraphicsEngine* m_pGraphicsEngine = nullptr;

		// 描画するパイプラインたち。
		// unique_ptr で持つのは、m_sortedCameras と m_pMainCamera が実体を指しているため。
		// 値のまま vector に入れると、カメラを1台足しただけで再確保が起きて
		// 保持しているポインタが全部ダングリングする
		std::vector<std::unique_ptr<CameraPipelineData>> m_cameras = {};

		// 使う順番などによりカメラをソートした配列
		std::vector<CameraPipelineData*> m_sortedCameras = {};

		// バックバッファに描画するもの
		CameraPipelineData* m_pMainCamera = nullptr;

		// モデルを受け取るパスの一覧(フレームの頭で作り直す)。
		// 実体はカメラのグラフが持っているので、ここは参照を並べるだけ
		std::vector<Pipeline::Pass*> m_pipelineOpaquePassVec = {};
		std::vector<Pipeline::Pass*> m_pipelineTransparentPassVec = {};

		// 直近にメインだったカメラの描画構成。
		// カメラが1台も積まれないフレーム(ゲームを止めているとき)でも、
		// 最後に画面を作っていた構成を借りられるように残しておく
		Handle<Pipeline::RenderingPipelineAsset> m_lastMainPipelineHandle = {};
	};
}
