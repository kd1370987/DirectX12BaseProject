#include "GraphicEngine.h"

#include "../MainEngine.h"

// D3D関係
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../D3D12/PipelineStateManager/PipelineStateManager.h"

// グラフィックス関係
#include "RenderContext/RenderContext.h"
#include "../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../Particle/ParticleBufferManager.h"
#include "MeshBufferAllocator/MeshBufferAllocator.h"
#include "../Resource/Data/QuadPolygon/QuadPolygon.h"
#include "MouseCursor/MouseCursor.h"

// レンダリングパイプライン
#include "RenderingPipeline/Core/Pass/Pass.h"
#include "RenderingPipeline/RenderingPipelineAsset/RenderingPipelineAsset.h"
#include "RenderingPipeline/RenderGraph/RenderGraph.h"
#include "RenderingPipeline/RenderingPipelineMetaRegistry.h"
#include "RenderingPipeline/GraphicsPipeline/GraphicsPipeline.h"

// シーン
#include "../Scene/BaseScene/BaseScene.h"
#include "../Scene/SceneManager/SceneManager.h"

// ECS
#include "../ECS/World/World.h"

// オプション
#include "../Option/OptionManager.h"

// カメラに依存しない、フレームに1回のGPU処理
#include "FrameCompute/SkinningPass/SkinningPass.h"
#include "FrameCompute/UpdateBLASPass/UpdateBLASPass.h"
#include "FrameCompute/ParticleSimulation/ParticleSimulation.h"











// テスト
#include "../../Application/Game/GameManager/GameManager.h"

namespace Engine::Graphics
{
	namespace
	{
		// vector を空にしつつ、次フレームぶんの容量を確保し直す(EndFrame用)。
		template<typename T>
		void ClearAndReserve(std::vector<T>& a_vec, size_t a_reserveCount)
		{
			a_vec.clear();
			a_vec.reserve(a_reserveCount);
		}
	}

	GraphicsEngine::GraphicsEngine()
	{}
	GraphicsEngine::~GraphicsEngine()
	{}

	void GraphicsEngine::Init(
		D3D12::GraphicsCommandList* a_pCmdList,
		const GraphicsEngineDesc& a_desc
	)
	{

		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

		m_pPipelineStateManager = a_desc.pPipelineStateManager;

		// レンダーコンテキストの作成
		for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
		{
			auto _upCtx = std::make_unique<RenderContext>();

			RenderContextDesc _desc = {};
			_desc.pDevice = _pDevice;

			_desc.cbAllocatorMemSize = 32 * 1024 * 1024;
			// シーンを重ねて描くとき(ポーズ画面など)は全ワールドのボーン行列を
			// 1本のパレットへ連結するので、1ワールド分(プールの確保数)では足りない
			_desc.boneElementNum = 40000;

			_upCtx->Init(this, a_pCmdList, _desc);
			m_upRenderContextVec.push_back(std::move(_upCtx));
		}

		//------------------------------------------------------------------
		// 描画用の板ポリ
		//
		// UIもパーティクルも同じものを使い回すだけで、フレームごとに書き換えたりしない。
		// レンダーコンテキストに持たせるとフレーム数ぶん同じ頂点バッファができるので、
		// エンジンが1つずつ持って配る。
		//
		// 湾曲用は横に kCurveDivision 分割したもの。
		// 4頂点の板をいくら曲げようとしても、間に頂点が無いのでまっすぐな
		// 台形にしかならない(頂点シェーダーは頂点の位置しか動かせない)。
		// 縦は分割しない : 曲げる量はUVのx(横位置)だけで決まるので、
		// 横に割りさえすれば弧になり、縦を割っても頂点が増えるだけで形は変わらない
		//------------------------------------------------------------------
		m_upQuadPolygon = std::make_unique<Resource::QuadPolygon>();
		m_upQuadPolygon->Init();

		m_upCurvedQuadPolygon = std::make_unique<Resource::QuadPolygon>();
		m_upCurvedQuadPolygon->Init(kCurveDivision + 1, 2);

		// ライト
		// バッファは上限ぶんを固定確保する(FrameLightData::Create の中)。
		// ライトが増えるたびに作り直すと、GPUが読んでいる最中のリソースを解放することになる
		m_lightManager.Init();
		for (auto& _frameLight : m_frameLightDataArr)
		{
			_frameLight.Create(_pDevice);
		}

		//------------------------------------------------------------------------------------
		// カメラに依存しない、フレームに1回で足りるGPU処理
		//
		// スキニング・BLAS更新・パーティクルの発生と更新は、どのカメラの描画でも
		// 同じ結果を読む。パイプラインのパスにすると、カメラの数だけ同じ計算を回すことになる。
		// ここで用意して Execute() から直接呼ぶ
		//------------------------------------------------------------------------------------
		SetupSkinning(m_pPipelineStateManager);
		SetupParticleSimulation(m_pPipelineStateManager);

		// 定数バッファ初期化
		m_cbAmbient = {};
		m_cbAmbient.ambientColorScale = { 0,0,0 };

		// バッファ管理クラス
		BufferSizeDesc _bufferSizeDesc = {};
		_bufferSizeDesc.staticVertexBufferSize = 20000000;
		_bufferSizeDesc.indexBufferSize = 20000000;
		_bufferSizeDesc.animatedVertexBufferSize = 5000000;
		m_upMeshBufferAllocator = std::make_unique<MeshBufferAllocator>();
		m_upMeshBufferAllocator->Init(
			_pDevice,
			a_pCmdList,
			_bufferSizeDesc
		);

		//------------------------------------------------------------------------------------
		// 生成できるパスの一覧を作る。
		// パイプラインアセットのロードで型IDからパスを作り直すのに使うので、
		// リソースを読み始めるより前に用意しておく必要がある
		//------------------------------------------------------------------------------------
		m_upPassMetaRegistry = std::make_unique<Pipeline::PassMetaRegistry>();
		Pipeline::RegisterBuiltinPasses(*m_upPassMetaRegistry);
	}

	// RenderGraph / Texture が完全型として見えるここで生成・破棄を定義する
	GraphicsEngine::CameraPipelineData::CameraPipelineData() = default;
	GraphicsEngine::CameraPipelineData::~CameraPipelineData() = default;

	Pipeline::PassMetaRegistry* GraphicsEngine::RefPassMetaRegistry()
	{
		return m_upPassMetaRegistry.get();
	}

	//==========================================================================================
	//
	// カメラごとの描画構成
	//
	//==========================================================================================
	//======================================================================================
	// モデルを受け取るパスへパス番号を配り直す
	//
	// 番号は描画アイテムのソートキーに入り、パスはそれで自分のぶんを引く。
	//
	// 組み直しのたびに新しい番号を取って返さない作りにすると、番号が減り続けて
	// やがて別のパスと同じ番号になり、他所のアイテムを別のPSOで描き始める。
	// シーンを切り替えるたびにカメラが作り直されるので、これは必ず起きる。
	//
	// どこか1つでも組み直したら、全カメラぶんをまとめて配り直す。
	// 組み直しは構成を触ったときだけなので、毎フレームの費用にはならない
	//======================================================================================
	void GraphicsEngine::AssignPipelinePassIndices()
	{
		// ソートキーのパス番号は8bit。上から順に配る
		uint8_t _next = 255;

		for (auto& _upCamera : m_cameras)
		{
			if (!_upCamera || !_upCamera->upPipeline) continue;
			if (!_upCamera->upPipeline->IsCompiled()) continue;

			const auto* _pGraph = _upCamera->upPipeline->GetRenderGraph();
			if (!_pGraph) continue;

			for (const auto& _compiledPass : _pGraph->GetCompiledPasses())
			{
				if (!_compiledPass.pPass) continue;
				if (_compiledPass.pPass->GetGeometryQueue() == EGeometryQueue::None) continue;

				_compiledPass.pPass->SetPassIndex(_next);

				// 0 まで来たら配り切り。ここへ届く構成は組み方がおかしい
				if (_next == 0)
				{
					ENGINE_WARNING("[GraphicsEngine] モデルを受け取るパスが多すぎます。パス番号が足りません");
					return;
				}
				--_next;
			}
		}
	}

	//======================================================================================
	// モデルを受け取るパスの一覧を作り直す
	//
	// 描画アイテムはサブセット1つごとに、これらのパスの数だけ積む。
	// つまりこの一覧は1フレームに何万回も引かれるので、
	// そのたびに全カメラを走査して配列を確保していると submit がそれだけで重くなる。
	//
	// カメラとパスの顔ぶれが変わるのはフレームの境目だけなので、
	// フレームの頭で1回作って、あとは引くだけにする
	//======================================================================================
	void GraphicsEngine::RefreshPipelineGeometryPassCache()
	{
		m_pipelineOpaquePassVec.clear();
		m_pipelineTransparentPassVec.clear();

		for (const auto& _upCamera : m_cameras)
		{
			if (!_upCamera || !_upCamera->upPipeline) continue;
			if (!_upCamera->upPipeline->IsCompiled()) continue;

			const auto* _pGraph = _upCamera->upPipeline->GetRenderGraph();
			if (!_pGraph) continue;

			for (const auto& _compiledPass : _pGraph->GetCompiledPasses())
			{
				if (!_compiledPass.pPass) continue;

				switch (_compiledPass.pPass->GetGeometryQueue())
				{
				case EGeometryQueue::Opaque:		m_pipelineOpaquePassVec.push_back(_compiledPass.pPass);		break;
				case EGeometryQueue::Transparent:	m_pipelineTransparentPassVec.push_back(_compiledPass.pPass);	break;
				default: break;
				}
			}
		}
	}

	const std::vector<Pipeline::Pass*>& GraphicsEngine::GetPipelineGeometryPasses(EGeometryQueue a_queue) const
	{
		static const std::vector<Pipeline::Pass*> _empty = {};

		switch (a_queue)
		{
		case EGeometryQueue::Opaque:		return m_pipelineOpaquePassVec;
		case EGeometryQueue::Transparent:	return m_pipelineTransparentPassVec;
		default:							return _empty;
		}
	}

	void GraphicsEngine::SubmitCamera(const CameraSubmitDesc& a_desc)
	{
		// 描画構成を持たないカメラは新経路に乗らない
		if (!a_desc.pipelineHandle.IsValid()) return;

		// 同じカメラが居れば使い回す(実行インスタンスを作り直さないため)
		CameraPipelineData* _pCamera = nullptr;
		for (auto& _upCamera : m_cameras)
		{
			if (!_upCamera) continue;
			if (_upCamera->pWorld != a_desc.pWorld) continue;
			if (_upCamera->entity != a_desc.entity) continue;

			_pCamera = _upCamera.get();
			break;
		}

		if (!_pCamera)
		{
			m_cameras.push_back(std::make_unique<CameraPipelineData>());
			_pCamera = m_cameras.back().get();
			_pCamera->pWorld = a_desc.pWorld;
			_pCamera->entity = a_desc.entity;
		}

		_pCamera->pipelineHandle = a_desc.pipelineHandle;
		_pCamera->order = a_desc.order;
		_pCamera->isMain = a_desc.isMain;
		_pCamera->isSubmitted = true;

		// 行列はこのカメラ専用の定数バッファ用。
		// 従来経路のカメラ設定(SetCameraMat)とは別物なので混ぜない。
		//
		// GPUへ詰める形(viewProj や逆行列)を作るのはパスをつなぎ込む段でよいので、
		// ここではビューと射影だけ入れておく
		DirectX::XMStoreFloat4x4(&_pCamera->cpuData.viewMat, a_desc.worldMat.Invert());
		DirectX::XMStoreFloat4x4(&_pCamera->cpuData.projMat, a_desc.projMat);

		// 0 のままなら画面の描画解像度に追従する
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const UINT _width = (a_desc.viewportWidth != 0) ? a_desc.viewportWidth : static_cast<UINT>(_winOp.windowWidth);
		const UINT _height = (a_desc.viewportHeight != 0) ? a_desc.viewportHeight : static_cast<UINT>(_winOp.windowHeight);

		// サイズが変わっていたら次の実行で作り直す
		if (_pCamera->builtWidth != _width || _pCamera->builtHeight != _height)
		{
			_pCamera->builtWidth = _width;
			_pCamera->builtHeight = _height;
			_pCamera->builtStructureVersion = 0;		// 0 は「まだ組んでいない」印
		}
	}

	//======================================================================================
	// 設計図が変わったカメラの実行インスタンスを組み直す
	//
	// フレームの頭(BeginFrame)から呼ぶこと。
	//
	// 組み直すと AssignPipelinePassIndices がパス番号を配り直す。
	// 描画アイテムは積むときにそのパス番号を焼き込んでいるので、
	// アイテムを積んだ後に配り直すと、引くときに別のパスのアイテムを拾い、
	// そのアイテムが持つ他所のPSOを張ってしまう
	// (ルートシグネチャも出力フォーマットも噛み合わずデバイスが飛ぶ)。
	//
	// アイテムを1つも積んでいないフレームの頭でやれば、番号とアイテムは必ず揃う
	//======================================================================================
	void GraphicsEngine::RebuildCameraPipelines(bool a_isNewOnly)
	{
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
		auto& _resourceManager = Resource::ResourceManager::Instance();

		// 組み直したカメラがあったか。1台でもあればパス番号を配り直す
		bool _isAnyRebuilt = false;

		// GPUの完了待ちは重いので、実際に捨てにかかる直前に1回だけ
		bool _isGPUWaited = false;

		// ---- 設計図から実行インスタンスを用意する ----
		//
		// フレームの頭から呼ばれたときは、今フレームぶんの積み込み(SubmitCamera)が
		// まだ来ていない。見るのは「前フレームまでに積まれて生き残ったカメラ」= m_cameras。
		// 今フレームに初めて現れるカメラはここには居ないので、
		// そのぶんは ExecuteCameraPipelines が a_isNewOnly で拾う
		for (auto& _upCamera : m_cameras)
		{
			CameraPipelineData* _pCamera = _upCamera.get();
			if (!_pCamera) continue;

			// 設計図がまだ読めていなければ何もしない
			auto* _pAsset = _resourceManager.Ref(_pCamera->pipelineHandle);
			if (!_pAsset) continue;

			// エディターで構成を触ると版が上がる。
			// 版が違えば、この実行インスタンスは古いので組み直す
			const uint32_t _version = _pAsset->GetStructureVersion();
			const bool _isRebuild = (!_pCamera->upPipeline) || (_pCamera->builtStructureVersion != _version);

			//--------------------------------------------------------------
			// フレームの途中から呼ばれたときは、初めて組むカメラだけを見る
			//
			// すでに実行インスタンスを持っているカメラを組み直すと、そのパスの
			// 番号が変わる。今フレームの描画アイテムはもう古い番号で積まれているので、
			// 番号だけが動くと引き違いが起きる(別のパスのPSOを張って落ちる)。
			//
			// 初めて組むカメラは m_cameras の末尾に足されたばかりで、
			// パス一覧にも入っていない = そのカメラ宛のアイテムは1つも無い。
			// 番号を配り直しても先に並ぶカメラの番号は動かないので、ここは通してよい
			//--------------------------------------------------------------
			if (a_isNewOnly && _pCamera->upPipeline) continue;

			// 形は同じでパラメータだけ動いたときは、値を写すだけで済ませる。
			// 色を触るたびにグラフを組み直すと、リソースまで作り直しになってしまう
			if (!_isRebuild && _pCamera->builtParamVersion != _pAsset->GetParamVersion())
			{
				if (const auto* _pSrcGraph = _pAsset->GetRenderGraph())
				{
					_pCamera->upPipeline->RefRenderGraph()->SyncParamsFrom(*_pSrcGraph);
				}
				_pCamera->builtParamVersion = _pAsset->GetParamVersion();
			}

			if (_isRebuild)
			{
				//--------------------------------------------------------------
				// ここから先は古いパスとGPUリソースを捨てにかかる。
				//
				// 途中で失敗して continue しても、捨てたことは取り消せない。
				// 印は「組み直すと決めた時点」で立てて、
				// パス番号の配り直しと一覧の作り直しを必ず通す
				//--------------------------------------------------------------
				_isAnyRebuilt = true;

				// 最終出力テクスチャを作り直すか : 待つかどうかの判定にも使う
				const bool _isFinalTexRebuild =
					(!_pCamera->upFinalTex ||
					 _pCamera->upFinalTex->GetDesc().Width != _pCamera->builtWidth ||
					 _pCamera->upFinalTex->GetDesc().Height != _pCamera->builtHeight);

				//--------------------------------------------------------------
				// GPUが前のフレームを走らせ終わるのを待つ
				//
				// このあとテクスチャとディスクリプタをその場で解放して、
				// すぐ同じ枠を取り直す。まだ実行中のコマンドリストが
				// それらを参照していると、解放済みのリソースを読みに行ったり、
				// 使用中のディスクリプタを上書きすることになる。
				//
				// 待つのは「捨てるものがあるとき」だけ。
				// 初めて組むカメラは解放するものが何も無いので待たない。
				// ここで無条件に待つと起動時に止まる : FrameManager::Init が
				// 先頭フレームのフェンス値を 1 へ進めておく一方、実際に 1 が
				// シグナルされるのは最初の EndFrame なので、それより前に
				// WaitForAll を通すと永久に返ってこない
				//--------------------------------------------------------------
				const bool _isDestructive =
					(_pCamera->upPipeline != nullptr) ||
					(_pCamera->upFinalTex != nullptr && _isFinalTexRebuild);

				if (_isDestructive && !_isGPUWaited)
				{
					D3D12::D3D12Wrapper::Instance().WaitForFrame();
					_isGPUWaited = true;
				}

				// ---- 最終出力テクスチャ ----
				if (_isFinalTexRebuild)
				{
					if (_pCamera->upFinalTex) _pCamera->upFinalTex->Release();

					_pCamera->upFinalTex = std::make_unique<Resource::Texture>();

					Resource::TextureCreateDesc _texDesc = {};
					_texDesc.name = "CameraFinal";
					_texDesc.width = _pCamera->builtWidth;
					_texDesc.height = _pCamera->builtHeight;
					_texDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
					_texDesc.usage = Resource::TextureUsage::RTV | Resource::TextureUsage::SRV;
					_texDesc.opClerValue = DXSM::Color(0.f, 0.f, 0.f, 1.f);
					_pCamera->upFinalTex->Create(_texDesc);
				}

				// ---- 実行インスタンスを設計図から作る ----
				if (!_pCamera->upPipeline)
				{
					_pCamera->upPipeline = std::make_unique<Pipeline::GraphicsPipeline>();
				}

				//--------------------------------------------------------------
				// 組めなかったときは黙って旧経路の絵に戻る。
				//
				// 黙って戻ると「繋いでいないパスの絵が出ている」ようにしか見えないので、
				// 版が変わるたびに1回だけ理由を知らせる
				// (個々の理由は RenderGraph::Compile が並べて出す)
				//--------------------------------------------------------------
				auto _reportFail = [&]()
					{
						if (_pCamera->reportedFailVersion == _version) return;
						_pCamera->reportedFailVersion = _version;

						ENGINE_WARNING(
							"[GraphicsEngine] パイプラインを組めませんでした。旧レンダーグラフの絵に戻ります : %s",
							_pAsset->GetName().c_str());
					};

				if (!_pCamera->upPipeline->BuildFrom(*_pAsset, *m_upPassMetaRegistry)) { _reportFail(); continue; }

				_pCamera->upPipeline->SetViewportSize(_pCamera->builtWidth, _pCamera->builtHeight);

				// このカメラの最終出力を、グラフの外から差し込む。
				// パスはこの名前で出力スロットを宣言すれば画面ぶんへ描ける
				_pCamera->upPipeline->ImportResource(
					kCameraOutputName,
					_pCamera->upFinalTex.get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET);

				if (!_pCamera->upPipeline->Compile(this, _pDevice)) { _reportFail(); continue; }

				_pCamera->builtStructureVersion = _version;
				_pCamera->builtParamVersion = _pAsset->GetParamVersion();

				// パス番号はカメラをまたいで一意でないといけないので、
				// 全部組み終わってからまとめて配る(印は上で立てている)
			}
		}

		// ---- パス番号を配り直してから回す ----
		if (_isAnyRebuilt)
		{
			AssignPipelinePassIndices();

			// 組み直しで古いパスは消えている。
			// 一覧が消えたパスを指したままにならないよう作り直す
			RefreshPipelineGeometryPassCache();
		}
	}

	// 積まれたカメラの実行インスタンスを回す。
	// 組み直しは RebuildCameraPipelines がフレームの頭で済ませてある
	void GraphicsEngine::ExecuteCameraPipelines()
	{
		// 今フレームに初めて現れたカメラだけ、ここで組んでおく。
		// 待つと1フレーム何も映らないので、シーン切り替えのたびに画面が飛ぶ
		RebuildCameraPipelines(true);

		// 今フレーム積まれたものだけを順番に並べる
		m_sortedCameras.clear();
		m_pMainCamera = nullptr;

		for (auto& _upCamera : m_cameras)
		{
			if (!_upCamera || !_upCamera->isSubmitted) continue;

			m_sortedCameras.push_back(_upCamera.get());
			if (_upCamera->isMain)
			{
				m_pMainCamera = _upCamera.get();

				// ゲームを止めているあいだも借りられるよう控えておく
				m_lastMainPipelineHandle = _upCamera->pipelineHandle;
			}
		}
		if (m_sortedCameras.empty()) return;

		std::stable_sort(
			m_sortedCameras.begin(), m_sortedCameras.end(),
			[](const CameraPipelineData* a, const CameraPipelineData* b)
			{
				return a->order < b->order;
			}
		);

		auto* _pRenderContext = m_upRenderContextVec[m_currentFrameIndex].get();

		for (CameraPipelineData* _pCamera : m_sortedCameras)
		{
			if (!_pCamera->upPipeline) continue;
			_pCamera->upPipeline->Render(this, _pRenderContext);
		}

		//----------------------------------------------------------------------------------
		// 描き終わった絵を「読める状態」にしておく
		//
		// グラフは最終出力を差し込まれたときのステート(RENDER_TARGET)へ戻して終わる。
		// ところがこの絵を読むのはグラフの外 : シーンビューやエフェクトエディターのImGui、
		// モニターに映すUIで、どれもシェーダーリソースとして読む。
		// RENDER_TARGET のまま読ませると不正なアクセスになるので、ここで移しておく。
		//
		// 次のフレームでグラフが書きに来るときは、リソースが自分で持っている
		// 今のステートから遷移し直すので、ここで変えておいても食い違わない
		//----------------------------------------------------------------------------------
		auto* _pCmdList = _pRenderContext->GetCurrentCmdList();
		if (_pCmdList)
		{
			for (CameraPipelineData* _pCamera : m_sortedCameras)
			{
				if (!_pCamera->upFinalTex) continue;
				_pCamera->upFinalTex->Barrier(_pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
		}
	}

	// 積まれなかったカメラを捨てる。
	// カメラが消えたのに実行インスタンスとテクスチャが残り続けるのを防ぐ
	void GraphicsEngine::PruneCameraPipelines()
	{
		for (auto& _upCamera : m_cameras)
		{
			if (!_upCamera || _upCamera->isSubmitted) continue;

			if (_upCamera->upPipeline) _upCamera->upPipeline->Release();
			if (_upCamera->upFinalTex) _upCamera->upFinalTex->Release();
		}

		m_cameras.erase(
			std::remove_if(m_cameras.begin(), m_cameras.end(),
				[](const std::unique_ptr<CameraPipelineData>& a_upCamera)
				{ return !a_upCamera || !a_upCamera->isSubmitted; }),
			m_cameras.end());

		// 次のフレームぶんの積み直しに備える
		for (auto& _upCamera : m_cameras)
		{
			if (_upCamera) _upCamera->isSubmitted = false;
		}

		// この並びは毎フレーム作り直すので捨ててよい
		m_sortedCameras.clear();

		//----------------------------------------------------------------------------------
		// 画面に出るカメラは、消えていなければ指したままにする
		//
		// ここで必ず nullptr にすると、次のフレームの頭で回るエディターの描画から
		// 「画面に出ている絵」が引けなくなる。
		// エディターのウィジェットを組むのは BeginDraw、カメラを積み直すのは
		// そのあとの Execute なので、間はここで残した値が使われる。
		//
		// 消えたカメラを指したままにはできないので、生き残っているかだけ確かめる
		//----------------------------------------------------------------------------------
		bool _isMainAlive = false;
		for (const auto& _upCamera : m_cameras)
		{
			if (_upCamera.get() != m_pMainCamera) continue;

			_isMainAlive = true;
			break;
		}
		if (!_isMainAlive) m_pMainCamera = nullptr;
	}

	// 画面へ出せる絵ができているか。
	//
	// 画面に出るカメラに描画構成が設定されていて、組み上がっているときだけ true。
	// 組めていないパイプラインは何も描いていないので、そのまま出すと真っ黒になる
	bool GraphicsEngine::IsPipelinePresentActive() const
	{
		if (!m_pMainCamera) return false;
		if (!m_pMainCamera->upPipeline || !m_pMainCamera->upPipeline->IsCompiled()) return false;

		return m_pMainCamera->upFinalTex != nullptr;
	}

	// 画面へ出す絵。パイプライン経路が生きていなければ nullptr
	const Resource::Texture* GraphicsEngine::GetPresentTexture() const
	{
		if (!IsPipelinePresentActive()) return nullptr;
		return m_pMainCamera->upFinalTex.get();
	}

	// メインカメラのパイプラインが描いた絵をバックバッファへ写す。
	// バックバッファと最終出力はどちらも R8G8B8A8_UNORM・同じ大きさなのでそのままコピーできる
	void GraphicsEngine::PresentFromPipeline(D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!a_pCmdList) return;
		if (!IsPipelinePresentActive()) return;

		Resource::Texture* _pFinalTex = m_pMainCamera->upFinalTex.get();
		if (!_pFinalTex) return;

		auto& _d3d = D3D12::D3D12Wrapper::Instance();
		ID3D12Resource* _pBackBuffer = _d3d.GetCurrentBackBuffer();
		if (!_pBackBuffer) return;

		// バックバッファはこの時点で RENDER_TARGET。コピー先へ落とす
		D3D12::ResourceBarrier(
			a_pCmdList, _pBackBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_DEST);

		// 最終出力側はグラフが入口のステートへ戻してある
		_pFinalTex->Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

		a_pCmdList->CopyResource(_pBackBuffer, _pFinalTex->GetResource());

		// この後の描画(エディターのImGuiなど)が続くので元へ戻す
		D3D12::ResourceBarrier(
			a_pCmdList, _pBackBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		// 最終出力はこの後 ImGui が読むので、読める状態のまま置いておく
		_pFinalTex->Barrier(a_pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// 設計図のパスと同じGUIDを持つ、実行インスタンス側のパスを返す。
	//
	// メインカメラを先に見るのは、同じ設計図を複数のカメラが使っているときに
	// 「画面に出ている絵」と、ノードに出る中身を揃えるため
	Pipeline::Pass* GraphicsEngine::FindPipelinePass(const Engine::GUID& a_passGUID) const
	{
		if (!a_passGUID.IsValid()) return nullptr;

		auto _findIn = [&a_passGUID](const CameraPipelineData* a_pCamera) -> Pipeline::Pass*
			{
				if (!a_pCamera || !a_pCamera->upPipeline) return nullptr;
				if (!a_pCamera->upPipeline->IsCompiled()) return nullptr;

				// この名前空間では RenderGraph は従来経路のものを指すので、必ず修飾する
				Pipeline::RenderGraph* _pGraph = a_pCamera->upPipeline->RefRenderGraph();
				if (!_pGraph) return nullptr;

				return _pGraph->FindPass(a_passGUID);
			};

		if (Pipeline::Pass* _pPass = _findIn(m_pMainCamera)) return _pPass;

		for (const auto& _upCamera : m_cameras)
		{
			if (_upCamera.get() == m_pMainCamera) continue;
			if (Pipeline::Pass* _pPass = _findIn(_upCamera.get())) return _pPass;
		}

		return nullptr;
	}

	std::vector<GraphicsEngine::PipelineGraphView> GraphicsEngine::CollectPipelineGraphs() const
	{
		std::vector<PipelineGraphView> _result = {};
		_result.reserve(m_cameras.size());

		auto& _resourceManager = Resource::ResourceManager::Instance();

		for (const auto& _upCamera : m_cameras)
		{
			if (!_upCamera || !_upCamera->upPipeline) continue;
			if (!_upCamera->upPipeline->IsCompiled()) continue;

			const Pipeline::RenderGraph* _pGraph = _upCamera->upPipeline->GetRenderGraph();
			if (!_pGraph) continue;

			// 設計図の名前でどのカメラか分かるようにする。
			// 同じ設計図を複数のカメラが使っていることもあるので、メインには印を付ける
			std::string _name = "Pipeline";
			if (const auto* _pAsset = _resourceManager.Ref(_upCamera->pipelineHandle))
			{
				_name = _pAsset->GetName();
			}
			if (_upCamera->isMain) _name += " (Main)";

			_result.push_back({ std::move(_name), _pGraph });
		}

		return _result;
	}

	const Resource::Texture* GraphicsEngine::GetCameraFinalTexture(const ECS::World* a_pWorld, uint32_t a_entity) const
	{
		for (const auto& _upCamera : m_cameras)
		{
			if (!_upCamera) continue;
			if (_upCamera->pWorld != a_pWorld) continue;
			if (_upCamera->entity != a_entity) continue;

			return _upCamera->upFinalTex.get();
		}
		return nullptr;
	}

	void GraphicsEngine::Release()
	{
		// カメラごとのパイプラインが抱えているGPUリソースを先に手放す。
		// DescriptorHeapManager の解放より前でないとビューが残る
		for (auto& _upCamera : m_cameras)
		{
			if (!_upCamera) continue;
			if (_upCamera->upPipeline) _upCamera->upPipeline->Release();
			if (_upCamera->upFinalTex) _upCamera->upFinalTex->Release();
		}
		m_cameras.clear();
		m_sortedCameras.clear();
		m_pMainCamera = nullptr;


		// レンダーコンテキスト解放
		for (auto& _ctx : m_upRenderContextVec)
		{
			_ctx->Release();
			_ctx.reset();
		}

		// ライト解放
		// プールを空にした時点で配り済みのライトハンドルはすべて無効になる
		for (auto& _frameLight : m_frameLightDataArr)
		{
			_frameLight.Release();
		}
		m_lightManager.Release();

		// 板ポリ解放
		m_upQuadPolygon.reset();
		m_upCurvedQuadPolygon.reset();

		// 各リンク解除
		m_pPipelineStateManager = nullptr;


		m_upMeshBufferAllocator->Release();

	}

	void GraphicsEngine::BeginFrame()
	{
		// 今から使うレンダーコンテキスをクリア
		m_currentFrameIndex = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();
		m_upRenderContextVec[m_currentFrameIndex]->Clear();

		// 設計図が変わったカメラの実行インスタンスを組み直す。
		// 描画アイテムを1つも積んでいない今のうちに済ませることで、
		// 配り直したパス番号とアイテムのパス番号が食い違わないようにする
		RebuildCameraPipelines(false);

		// モデルを受け取るパスの一覧を作り直す。
		// このフレームの描画アイテムはここで作った一覧に沿って積まれる
		RefreshPipelineGeometryPassCache();
	}
	void GraphicsEngine::Execute()
	{
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
		auto* _pCmdList = D3D12::D3D12Wrapper::Instance().GetDirectCommandList();
		// GPUが実際に完了させた値 : これ以下でタグ付けされた領域だけをフリーリストに戻す
		auto _completedFence = D3D12::D3D12Wrapper::Instance().GetCompletedFenceValue();

		// レイトレ用BLAS初期化 : 初期化命令があれば走る
		ProcessInitQueue(_pDevice, _pCmdList);

		// テスト
		App::Game::GameManager::Instance().Draw();

		// 自前のマウスカーソルを最前面へ。
		// UIパスは深度を切ってあるので積んだ順がそのまま前後になる。
		// シーンのUIを全部積み終えたここで積むことで、必ず一番手前に出る。
		//
		// ゲームモード以外はエディターのImGuiが上に重なるので、あちらの
		// 最前面レイヤーへ描く(MouseCursor::DrawImGui)。ここでは積まない
		if (MainEngine::Instance().GetMode() == EAppMode::Game)
		{
			if (auto* _pCursor = MainEngine::Instance().RefMouseCursor())
			{
				_pCursor->SubmitUI(this);
			}
		}

		// メッシュバッファの更新
		m_upMeshBufferAllocator->UpdateFrame(_pCmdList, _completedFence);

		// パーティクルのバッファ更新
		MainEngine::Instance().RefParticleManager()->UploadEmitData(_pCmdList);

		// バックバッファのresourceバリア
		D3D12::ResourceBarrier(
			_pCmdList, // D3D12Wrapper側のバリア関数も引数でリストをもらうように修正してください
			D3D12::D3D12Wrapper::Instance().GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		auto _cpuHandle = Engine::D3D12::DescriptorHeapManager::Instance().GetCPU(
			D3D12::D3D12Wrapper::Instance().GetCurrentBackBufferTex().GetRTV()
		);
		float _clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f }; // 背景色
		_pCmdList->ClearRenderTargetView(_cpuHandle, _clearColor, 0, nullptr);

		// レンダーコンテキストにコマンドリストをセット
		m_upRenderContextVec[m_currentFrameIndex]->SetDirectCommandList(_pCmdList);

		// カメラの割り込み(エディターカメラなど)
		// ECS側のカメラ設定は上の GameManager::Draw() 内(PreDrawフェーズ)で行われるため、
		// 上書きするなら必ずこの位置(GPUデータ作成の直前)で行うこと。
		if (m_isCameraOverride)
		{
			SetCameraMat(m_cameraOverrideWorldMat);
			SetProjMat(m_cameraOverrideProjMat);
		}

		// GPU用カメラデータを作成
		CreateGPUCameraData();

		// バッファの更新
		// ボーン行列は上の GameManager::Draw() で描くワールドぶんだけ積まれている。
		// 「今の一番上のシーン」から引いてはいけない(ポーズ中は後ろのゲームのボーンが載らない)
		m_upRenderContextVec[m_currentFrameIndex]->UpdateBuffer(
			m_meshInstanceDataVec, m_meshMaterialDataVec,
			m_boneMatrixVec
		);
		//------------------------------------------------------------------
		// UIをレイヤー順に並べ替える
		//
		// UIパスは深度を持たない(DepthEnable(false))ので、重なりを決めるのは
		// 描く順そのもの。並べ替えないとレイヤーの値がどこにも効かない。
		//
		// 小さいものから描く = 大きいほど手前。
		// 同じ値のものは積んだ順を崩さないよう stable_sort を使う
		// (1つのUIが持つ飾りは配列順で重なっているため、崩すと絵が入れ替わる)
		//------------------------------------------------------------------
		std::stable_sort(
			m_uiDrawItemVec.begin(), m_uiDrawItemVec.end(),
			[](const UIData& a, const UIData& b)
			{
				return a.layer < b.layer;
			}
		);

		m_upRenderContextVec[m_currentFrameIndex]->UpdateUIBuffer(m_uiDrawItemVec);

		// ライトをGPUバッファへ詰め直す。
		// レンダーパスが引くのはこの結果なので、必ずレンダーグラフの実行より前に済ませる
		m_lightManager.BuildFrameData(m_frameLightDataArr[m_currentFrameIndex]);


		// 描画アイテムをソート
		std::sort(
			m_lightWeightDrawItemVec.begin(), m_lightWeightDrawItemVec.end(),
			[](const LightWeightDrawItem& a, const LightWeightDrawItem& b)
			{
				return a.sortKey.value < b.sortKey.value;
			}
		);

		//------------------------------------------------------------------
		// カメラに依存しない毎フレームの計算
		//
		// スキニングの結果とBLASは、どのカメラの描画でも同じものを読む。
		// カメラごとに回すと同じ計算を何度も走らせることになるので、
		// パイプラインより前でまとめて1回だけ通す
		//------------------------------------------------------------------
		ExecuteSkinning(this, m_upRenderContextVec[m_currentFrameIndex].get());
		ExecuteUpdateBLAS(this, m_upRenderContextVec[m_currentFrameIndex].get());

		// 発生と更新は間のUAVバリアごと1つの関数にまとめてある。
		// 分けるとバリアを挟み忘れて、空きスロットが減り続ける不具合が戻る
		ExecuteParticleSimulation(this, m_upRenderContextVec[m_currentFrameIndex].get());

		// カメラごとの描画構成(新レンダーグラフ)。
		// 従来経路とは並走していて、こちらは各カメラの最終出力テクスチャへ描くだけ。
		// バックバッファへ出すのは下の従来経路のまま
		ExecuteCameraPipelines();

		// メインカメラが描いた絵をバックバッファへ載せる
		PresentFromPipeline(_pCmdList);

		D3D12::D3D12Wrapper::Instance().SubmitDirectCommandList(_pCmdList);
		m_upRenderContextVec[m_currentFrameIndex]->SetDirectCommandList(nullptr);

	}
	void GraphicsEngine::EndFrame()
	{
		// 今フレーム積まれなかったカメラを捨てる
		PruneCameraPipelines();


		// 描画命令をクリアしてメモリ領域を確保しておく
		ClearAndReserve(m_lightWeightDrawItemVec, 10000);
		ClearAndReserve(m_uiDrawItemVec, 10000);
		ClearAndReserve(m_dynamicRayRequestVec, 1000);
		ClearAndReserve(m_skinningDispathItemVec, 1000);

		// ボーンパレットと、ワールドごとの土台の対応表
		ClearAndReserve(m_boneMatrixVec, 10000);
		m_boneBaseIndexMap.clear();

		// オブジェクトデータの消去
		ClearAndReserve(m_meshInstanceDataVec, 10000);

		// サブセット情報の消去
		ClearAndReserve(m_meshMaterialDataVec, 10000);

		//------------------------------------------------------------------
		// 画面効果は毎フレーム、アクティブカメラが設定し直す。
		//
		// ここで落としておけば、カメラが居ない/その設定を持たないフレームは
		// 前フレームの値で効き続けることがない。
		// フラグを下ろすと、パスはアセットに保存した自分の値へ戻る
		//------------------------------------------------------------------
		m_cbDoF = {};
		m_isDoFOverride = false;

		m_cbRadialBlur = {};
		m_isRadialBlurOverride = false;

		m_cbFishEye = {};
		m_isFishEyeOverride = false;

		// デバッグ用配列のクリア
		Editor::MainEditor::Instance().ClearBuffer();
	}

	const Graphics::RenderContext* GraphicsEngine::GetRenderContext() const
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();
	}
	Graphics::RenderContext* GraphicsEngine::RefRenderContext()
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();

	}
	D3D12::PipelineStateManager* GraphicsEngine::RefPipelineStateManager()
	{
		return m_pPipelineStateManager;
	}
	LightManager* GraphicsEngine::RefLightManager()
	{
		return &m_lightManager;
	}

	const FrameLightData& GraphicsEngine::GetFrameLightData() const
	{
		return m_frameLightDataArr[m_currentFrameIndex];
	}
	void GraphicsEngine::SetCameraMat(const DXSM::Matrix& a_worldMat)
	{
		// 座標を代入
		m_cbCamera.pos = { a_worldMat._41,a_worldMat._42,a_worldMat._43 ,1 };

		// ビュー行列・逆ビュー行列をセット
		m_cbCamera.viewMat = a_worldMat.Invert();
		m_cbCamera.viewInvMat = a_worldMat;
	}
	void GraphicsEngine::SetProjMat(const DXSM::Matrix& a_projMat)
	{
		m_cbCamera.projMat = a_projMat;
		m_cbCamera.projInvMat = a_projMat.Invert();
	}
	void GraphicsEngine::SetDoFData(const DoFOptionCB& a_data)
	{
		m_cbDoF = a_data;

		// 今フレームはカメラが決めた値を使う
		m_isDoFOverride = true;
	}
	const DoFOptionCB& GraphicsEngine::GetDoFData() const
	{
		return m_cbDoF;
	}
	void GraphicsEngine::SetRadialBlurData(const RadialBlurOptionCB& a_data)
	{
		m_cbRadialBlur = a_data;

		// 今フレームはカメラが決めた値を使う
		m_isRadialBlurOverride = true;
	}
	const RadialBlurOptionCB& GraphicsEngine::GetRadialBlurData() const
	{
		return m_cbRadialBlur;
	}
	void GraphicsEngine::SetFishEyeData(const FishEyeOptionCB& a_data)
	{
		m_cbFishEye = a_data;

		// 今フレームはカメラが決めた値を使う
		m_isFishEyeOverride = true;
	}
	const FishEyeOptionCB& GraphicsEngine::GetFishEyeData() const
	{
		return m_cbFishEye;
	}
	void GraphicsEngine::SetCameraOverride(const DXSM::Matrix& a_worldMat, const DXSM::Matrix& a_projMat)
	{
		m_isCameraOverride = true;
		m_cameraOverrideWorldMat = a_worldMat;
		m_cameraOverrideProjMat = a_projMat;
	}
	void GraphicsEngine::ClearCameraOverride()
	{
		m_isCameraOverride = false;
	}
	const CameraData& GraphicsEngine::GetCameraData() const
	{
		return m_cbGPUCamera;
	}
	const CameraData& GraphicsEngine::GetGPUCameraData() const
	{
		return m_cbGPUCamera;
	}
	const CameraData& GraphicsEngine::GetCPUCameraData() const
	{
		return m_cbCamera;
	}
	void GraphicsEngine::SetAmbientData(const AmbientData& a_data)
	{
		m_cbAmbient = a_data;
	}
	const AmbientData& GraphicsEngine::GetAmbientData() const
	{
		return m_cbAmbient;
	}
	AmbientData& GraphicsEngine::RefAmbientData()
	{
		return m_cbAmbient;
	}
	void GraphicsEngine::SetSkyData(const SkyData& a_data)
	{
		m_cbSky = a_data;
	}
	const SkyData& GraphicsEngine::GetSkyData() const
	{
		return m_cbSky;
	}
	SkyData& GraphicsEngine::RefSkyData()
	{
		return m_cbSky;
	}
	void GraphicsEngine::SetSkyTexture(const Handle<Resource::Texture>& a_handle)
	{
		m_skyTexHandle = a_handle;
	}
	const Handle<Resource::Texture>& GraphicsEngine::GetSkyTexture() const
	{
		return m_skyTexHandle;
	}
	//======================================================================================
	// ボーンパレットへこのワールドのボーン行列を積む
	//--------------------------------------------------------------------------------------
	// ボーン行列はワールド(シーン)ごとの RangePool に入っていて、その添字も
	// ワールドごとに 0 から始まる。GPU側のボーンパレットは1本しかないので、
	// 複数のシーンを重ねて描くときは連結したうえで土台を足してやる必要がある。
	//
	// ポーズ画面はゲームのシーンへ重ねて出す(Push)ので、描くワールドが2つになる。
	// ここで両方を積んでおかないと、片方のキャラのボーンが単位行列でも他人のものでもない
	// 場所を指し、頂点が一点に潰れて消えたように見える。
	//
	// 呼ばれるのは描画フェーズ(GameManager::Draw)の中で、ボーン行列自体は
	// それより前の Animation フェーズで確定しているので、この時点の値で正しい。
	//======================================================================================
	uint32_t GraphicsEngine::AcquireBoneBaseIndex(ECS::World& a_world)
	{
		// 同じワールドをこのフレームで既に積んでいればその位置を返す
		auto _it = m_boneBaseIndexMap.find(&a_world);
		if (_it != m_boneBaseIndexMap.end()) return _it->second;

		const uint32_t _baseIndex = static_cast<uint32_t>(m_boneMatrixVec.size());

		if (a_world.HasResource<Pool::RangePool<Resource::BoneMatrix>>())
		{
			auto& _boneMatPool = a_world.GetResource<Pool::RangePool<Resource::BoneMatrix>>();

			// プールは最初から10000要素ぶん確保されているので、丸ごと積むと
			// ワールドを2つ重ねただけでGPU側のボーンパレットが溢れる。
			// 実際に使われている末尾までで足りる(ハンドルの添字は必ずこの内側)
			const auto& _data = _boneMatPool.GetAllData();
			const size_t _usedCount = (std::min)(
				static_cast<size_t>(_boneMatPool.GetUsedCount()), _data.size());

			m_boneMatrixVec.insert(m_boneMatrixVec.end(), _data.begin(), _data.begin() + _usedCount);
		}

		m_boneBaseIndexMap.emplace(&a_world, _baseIndex);
		return _baseIndex;
	}

	void GraphicsEngine::SubmitSkinning(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const Handle<Raytracing::DynamicRaytracingData> dynamicHandle,
		const RangeHandle<Resource::NodePoseMatrix> nodePoseHnandle,
		const RangeHandle<Resource::BoneMatrix> boneHandle
	)
	{
		// このワールドのボーン行列をパレットへ積み、GPU上の土台を得る
		const uint32_t _boneBaseIndex = AcquireBoneBaseIndex(a_world);

		const auto& _drawCmdVec = a_pModel->GetDrawCommandVec();
		for (const auto& _cmd : _drawCmdVec)
		{
			// マテリアル取得
			auto* _pMaterial = Engine::Resource::ResourceManager::Instance().Get(_cmd.materialHandle);
			if (!_pMaterial) continue;

			// メッシュ取得
			auto* _pMesh = Engine::Resource::ResourceManager::Instance().Get(_cmd.meshHandle);
			if (!_pMesh) continue;

			// レイトレ用データを持たないメッシュはスキニング登録できない
			if (!_pMesh->HasRtData()) continue;

			// マテリアルからシェーディングモデルを取得
			auto* _pShadingModel = Engine::Resource::ResourceManager::Instance().Get(_pMaterial->shadingModelHandle);
			if (!_pShadingModel) continue;

			SkinningDispatchItem _item = {};
			_item.pWorld = &a_world;
			_item.staticVertexHandle = _pMesh->GetRtData().vertexHandle;
			_item.staticIndexHandle = _pMesh->GetRtData().indexHandle;
			_item.nodePoseMat = nodePoseHnandle;
			_item.animHandle = dynamicHandle;
			_item.boneHandle = boneHandle;

			// スキニングのコンピュートが読むのはGPU上の位置なので土台を足す
			_item.boneBufferStart = _boneBaseIndex + boneHandle.startIndex;

			auto& _pool = a_world.GetResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>();
			auto* _data = _pool.Get(dynamicHandle);
			if (!_data) continue;

			for (auto& _meshData : _data->meshDataVec)
			{
				if (_cmd.meshHandle == _meshData.meshHandle)
				{
					_item.animatedHandle = _meshData.animatedVertexHandle;
				}
			}

			m_skinningDispathItemVec.push_back(_item);
		}
	}
	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const DXSM::Matrix& a_worldMatrix,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd
	)
	{
		SubmitModel(
			a_world,
			a_pModel,
			a_worldMatrix,
			a_worldMatrix,
			a_albedoScale,
			a_emissiveScale,
			a_emissiveAdd
		);
	}

	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const DXSM::Matrix& a_worldMatrix,
		const DXSM::Matrix& a_prevMatrix,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd
	)
	{
		if (!a_pModel) return;

		// モデルが持っている描画コマンド（サブセット）を展開
		const auto& _drawCmdVec = a_pModel->GetDrawCommandVec();

		for (const auto& _cmd : _drawCmdVec)
		{
			// -----------------------------------------------------
			// リソースの取得と検証
			// -----------------------------------------------------
			const Resource::Mesh* _pMesh = nullptr;
			const Resource::Material* _pMaterial = nullptr;
			if (!FetchDrawResources(_cmd, _pMesh, _pMaterial)) continue;

			// -----------------------------------------------------
			// 行列計算
			// -----------------------------------------------------
			DXSM::Matrix _nodeTransMat(a_pModel->GetOriginalNodeVec()[_cmd.nodeIndex].worldTransform);
			DXSM::Matrix _mat = _nodeTransMat * a_worldMatrix;
			DXSM::Matrix _prevMat = _nodeTransMat * a_prevMatrix;

			// -----------------------------------------------------
			// PermutationFlags の構築
			// -----------------------------------------------------
			uint32_t _flags = (uint32_t)Engine::Graphics::EShaderPermutationFlags::None;

			bool _isAnimation = false; // ボーンがあるか等で判定
			_flags |= (uint32_t)(_isAnimation ?
				Engine::Graphics::EShaderPermutationFlags::Skinned :
				Engine::Graphics::EShaderPermutationFlags::Static);

			if (_cmd.alphaMode == Engine::Resource::Alpha::Mask) {
				_flags |= (uint32_t)Engine::Graphics::EShaderPermutationFlags::AlphaMasked;
			}

			Engine::Graphics::PSOKey _psoKey = {};
			_psoKey.shadingModelTableHandle = _pMaterial->shadingModelHandle;
			_psoKey.permutationFlags = _flags;

			// -----------------------------------------------------
			// 各パスへの描画アイテム登録(共通処理)
			// -----------------------------------------------------
			RegisterDrawCommandToPasses(
				_cmd, _pMesh, _pMaterial,
				_mat, _prevMat,
				_isAnimation, 0 /*animatedVertexStart*/,
				a_albedoScale, a_emissiveScale, a_emissiveAdd, _psoKey);
		}
	}

	void GraphicsEngine::SubmitModel(
		ECS::World& a_world,
		const Resource::Model* a_pModel,
		const DXSM::Matrix& a_worldMatrix,
		const DXSM::Matrix& a_prevMatrix,
		const RangeHandle<Resource::BoneMatrix>& a_boneHandle,
		const RangeHandle<Resource::NodePoseMatrix>& a_nodePoseHandle,
		const Handle<Raytracing::DynamicRaytracingData>& a_animData,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd
	)
	{
		// ノード行列取得
		auto& _nodePosePool = a_world.GetResource<Pool::RangePool<Resource::NodePoseMatrix>>();
		const auto& _nodePoseMatVec = _nodePosePool.GetRange(a_nodePoseHandle);

		// アニメーション後データ (※1つのモデルに対して共通ならループ外で取得・チェックすると効率的です)
		auto& _pool = a_world.GetResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>();
		auto* _data = _pool.Get(a_animData);
		if (!_data) return;

		// このワールドのボーン行列をパレットへ積む。
		// 戻り値の土台位置はここでは使わない(頂点をスキニングするのはコンピュートの
		// スキニングパスで、描画側はその結果の頂点バッファを読むだけ)。
		// ただし積むこと自体はそのパスに要るので、呼び出しを外してはいけない
		AcquireBoneBaseIndex(a_world);

		// モデルが持っている描画コマンド（サブセット）を展開
		const auto& _drawCmdVec = a_pModel->GetDrawCommandVec();
		for (const auto& _cmd : _drawCmdVec)
		{
			// メッシュ・マテリアルの取得と検証
			const Resource::Mesh* _pMesh = nullptr;
			const Resource::Material* _pMaterial = nullptr;
			if (!FetchDrawResources(_cmd, _pMesh, _pMaterial)) continue;

			// -----------------------------------------------------
			// アニメーション用頂点オフセットの検索
			// -----------------------------------------------------
			uint32_t _animatedVertexStart = 0;
			if (_data)
			{
				for (const auto& _meshData : _data->meshDataVec)
				{
					if (_cmd.meshHandle == _meshData.meshHandle)
					{
						_animatedVertexStart = _meshData.animatedVertexHandle.startIndex;
						break; // 見つかったらループを抜ける
					}
				}
			}

			// ノードのワールド行列を確定
			// モデル差し替え直後などで描画コマンドとポーズ領域のサイズが食い違った場合は
			// クラッシュさせずこのコマンドの描画をスキップする
			if (_cmd.nodeIndex >= _nodePoseMatVec.size()) continue;
			DXSM::Matrix _nodeTransMat(_nodePoseMatVec[_cmd.nodeIndex].world);
			DXSM::Matrix _mat = _nodeTransMat * a_worldMatrix;

			DXSM::Matrix _prevMat = _nodeTransMat * a_prevMatrix;

			// =========================================================
			// PermutationFlags を構築
			// =========================================================
			uint32_t _flags = (uint32_t)Engine::Graphics::EShaderPermutationFlags::None;

			// アニメーション判定
			bool _isAnimation = (a_boneHandle.count > 0);
			_flags |= (uint32_t)(_isAnimation ?
				Engine::Graphics::EShaderPermutationFlags::Skinned :
				Engine::Graphics::EShaderPermutationFlags::Static);

			// アルファモード判定
			if (_cmd.alphaMode == Engine::Resource::Alpha::Mask) {
				_flags |= (uint32_t)Engine::Graphics::EShaderPermutationFlags::AlphaMasked;
			}

			// PSOKey作成
			Engine::Graphics::PSOKey _psoKey = {};
			_psoKey.shadingModelTableHandle = _pMaterial->shadingModelHandle;
			_psoKey.permutationFlags = _flags;

			// =========================================================
			// 各パスへ描画アイテムを投げる(共通処理)
			// =========================================================
			RegisterDrawCommandToPasses(
				_cmd, _pMesh, _pMaterial,
				_mat, _prevMat,
				_isAnimation, _animatedVertexStart,
				a_albedoScale, a_emissiveScale, a_emissiveAdd, _psoKey);
		}
	}

	void GraphicsEngine::SubmitModel(const DXSM::Matrix& a_worldMat, const DXSM::Vector4& a_colorScale, const DXSM::Vector3& a_emissiveScale, const Engine::Handle<Raytracing::DynamicRaytracingData> dynamicHandle, const Engine::Handle<Resource::NodePoseMatrix> nodePoseHnandle, const DXSM::Vector3& a_emissiveAdd)
	{

		m_dynamicRayRequestVec.push_back(
			{ a_worldMat,a_colorScale,a_emissiveScale,a_emissiveAdd,dynamicHandle,nodePoseHnandle }
		);
	}

	void GraphicsEngine::SubmitUI(const Handle<Resource::Texture>& a_texHandle, const Math::Vector2& a_screenPos, const Math::Vector2& a_screenRect, const Math::Color& a_color, float a_rotation, float a_layer, const Math::Vector2& a_uvOffset, const Math::Vector2& a_pivot, const Math::Vector2& a_uvScale, float a_curveK, float a_curveOffsetX)
	{
		auto& _resMgr = Resource::ResourceManager::Instance();

		// 読み込みが終わっていないものは、そのフレームは描かない。
		// 非同期ロード中のスロットには空の実体が入っているため、
		// ポインタのnullチェックだけでは弾けない
		if (!_resMgr.IsReady(a_texHandle)) return;

		auto* _pTex = _resMgr.Get(a_texHandle);
		if (!_pTex) return;

		// サイズは呼び出し側の指定値をそのまま使う
		PushUIData(_pTex->GetSRV().GetIndex(), a_screenPos, a_screenRect, a_color, a_rotation, a_layer, a_uvOffset, a_pivot, a_uvScale, a_curveK, a_curveOffsetX);
	}

	void GraphicsEngine::SubmitUI(const Handle<Resource::Texture>& a_texHandle, const Math::Vector2& a_screenPos, float a_scale, const Math::Color& a_color, float a_rotation, float a_layer, const Math::Vector2& a_uvOffset, const Math::Vector2& a_pivot, float a_curveK, float a_curveOffsetX)
	{
		auto& _resMgr = Resource::ResourceManager::Instance();

		// 読み込み中のものは描かない : 空の実体のサイズを掛けても意味がない
		if (!_resMgr.IsReady(a_texHandle)) return;

		auto* _pTex = _resMgr.Get(a_texHandle);
		if (!_pTex) return;

		// テクスチャの元サイズにスケールを掛けたものを表示サイズにする
		Math::Vector2 _size = { _pTex->GetDesc().Width * a_scale, _pTex->GetDesc().Height * a_scale };
		PushUIData(_pTex->GetSRV().GetIndex(), a_screenPos, _size, a_color, a_rotation, a_layer, a_uvOffset, a_pivot, {1.0f,1.0f}, a_curveK, a_curveOffsetX);
	}

	UINT GraphicsEngine::SetInstanceData(const MeshInstanceData& a_instanceData)
	{
		UINT _index = static_cast<UINT>(m_meshInstanceDataVec.size());
		m_meshInstanceDataVec.push_back(a_instanceData);
		return _index;
	}
	UINT GraphicsEngine::SetMeshMaterialData(const MeshMaterial& a_subsetData)
	{
		UINT _index = static_cast<UINT>(m_meshMaterialDataVec.size());
		m_meshMaterialDataVec.push_back(a_subsetData);
		return _index;
	}
	void GraphicsEngine::AddItem(const LightWeightDrawItem& a_item)
	{
		// アイテム配列に追加
		m_lightWeightDrawItemVec.push_back(a_item);
	}
	std::span<const LightWeightDrawItem> GraphicsEngine::GetPassItems(uint8_t a_passIndex)
	{
		// 探したいパスのキーの最小値と最大値を求める
		uint64_t _minKey = static_cast<uint64_t>(a_passIndex) << 56;
		uint64_t _maxKey = _minKey | 0x00FFFFFFFFFFFFFFull; // 下位56ビットをすべて1にする

		// ソート済み配列から開始位置を見つける
		auto _itStart = std::lower_bound(
			m_lightWeightDrawItemVec.begin(),
			m_lightWeightDrawItemVec.end(),
			_minKey,
			[](const LightWeightDrawItem& a_item, uint64_t a_value)
			{
				return a_item.sortKey.value < a_value;
			}
		);

		// ソート済み配列から終了位置を見つける
		auto _itEnd = std::upper_bound(
			_itStart,		// 開始位置から探す
			m_lightWeightDrawItemVec.end(),
			_maxKey,
			[](uint64_t a_value, const LightWeightDrawItem& a_item)
			{
				return a_value < a_item.sortKey.value;
			}
		);

		return std::span<const LightWeightDrawItem>(_itStart, _itEnd);
	}

	void GraphicsEngine::BindPSO(Graphics::RenderContext* a_pCtx, const Handle<ID3D12PipelineState>& a_handle)
	{
		if (!a_pCtx) return;
		a_pCtx->SetGraphicPSO(a_handle);
	}

	void GraphicsEngine::BindPSO(Graphics::RenderContext* a_pCtx, uint8_t a_psoIndex)
	{
		auto* _pPSO = m_pPipelineStateManager->GetPSO(a_psoIndex);
		if (!_pPSO) return;
		a_pCtx->SetGraphicPSO(_pPSO);
	}

	void GraphicsEngine::CreateGPUCameraData()
	{
		// リセット
		m_cbGPUCamera = {};

		// スクリーン座標を取得
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const auto& _renderingOp = Option::OptionManager::GetInstance().GetRenderingOption();

		// ジッターオフセット計算
		float _jitterX = 0.0f;
		float _jitterY = 0.0f;

		// ジッターオンオフ(グラフィックオプションで切り替え可能。OFFならジッター0でTAAはブレンドのみ)
		if (_renderingOp.useJitter)
		{
			// ハルトンシーケンスのテーブル（ピクセル中心地からのオフセット値 -0.5f ～ 0.5f）
			static const float _sHaltonX[16] = {
				0.000000f, -0.250000f,  0.250000f, -0.375000f,
				0.125000f, -0.125000f,  0.375000f, -0.437500f,
				0.062500f, -0.187500f,  0.312500f, -0.312500f,
				0.187500f, -0.062500f,  0.437500f, -0.468750f
			};
			static const float _sHaltonY[16] = {
				0.000000f,  0.166667f, -0.166667f,  0.500000f,
			   -0.500000f, -0.277778f,  0.055556f,  0.388889f,
			   -0.388889f, -0.055556f,  0.277778f,  0.444444f,
			   -0.222222f,  0.111111f, -0.444444f,  0.222222f
			};
			uint32_t _sampleIndex = m_totlaFrameCount % 16;

			// プロジェクション空間（NDC）のサイズに変換 : NDCは幅が２(-1～1)だから2倍
			_jitterX = (_sHaltonX[_sampleIndex] / (float)_winOp.windowWidth) * 2.0f;
			_jitterY = (_sHaltonY[_sampleIndex] / (float)_winOp.windowHeight) * 2.0f;
		}

		// カメラの行列を一時的に取得
		DXSM::Matrix _viewMat = m_cbCamera.viewMat;
		DXSM::Matrix _projMat = m_cbCamera.projMat;
		DXSM::Matrix _invViewMat = m_cbCamera.viewInvMat;
		DXSM::Matrix _invProjMat = m_cbCamera.projInvMat;

		// モーションベクター用のジッターなしViewProjを計算
		DXSM::Matrix _nonJitteredViewProj = _viewMat * _projMat;
		DXSM::Matrix _nonJitteredInvViewProj = _nonJitteredViewProj.Invert();

		// 描画用のジッターあり投影行列を作成
		DXSM::Matrix _jitteredProjMat = _projMat;
		_jitteredProjMat._31 += _jitterX;
		_jitteredProjMat._32 += _jitterY;

		// 描画用のジッターありViewProjとその逆行列を計算
		DXSM::Matrix _jitteredViewProj = _viewMat * _jitteredProjMat;
		DXSM::Matrix _invJitteredProj = _jitteredProjMat.Invert();
		DXSM::Matrix _invJitteredViewProj = _jitteredViewProj.Invert();

		// GPU転送用バッファへの詰め込み
		m_cbGPUCamera.pos = m_cbCamera.pos;

		// 通常の描画（SV_Positionの計算）にはジッターありを使う
		m_cbGPUCamera.viewMat = _viewMat.Transpose();
		m_cbGPUCamera.projMat = _jitteredProjMat.Transpose();
		m_cbGPUCamera.viewInvMat = _invViewMat.Transpose();
		m_cbGPUCamera.projInvMat = _invJitteredProj.Transpose();
		m_cbGPUCamera.viewProjMat = _jitteredViewProj.Transpose();
		m_cbGPUCamera.invViewProjMat = _invJitteredViewProj.Transpose();

		// モーションベクターの計算にはジッターなしを使う
		m_cbGPUCamera.nonJitteredProj = _projMat.Transpose();
		m_cbGPUCamera.nonJitteredViewProj = _nonJitteredViewProj.Transpose();
		m_cbGPUCamera.nonJitteredInvViewProj = _nonJitteredInvViewProj.Transpose();

		// 過去フレームのジッターなし行列の処理
		m_cbGPUCamera.prevView = m_prevViewMat.Transpose();
		m_cbGPUCamera.prevProj = m_prevProjMat.Transpose();
		m_cbGPUCamera.prevViewProj = m_prevNonJitteredViewProj.Transpose();

		// 次のフレームのためにジッターなしデータを保存
		m_prevViewMat = _viewMat;
		m_prevProjMat = _projMat;
		m_prevNonJitteredViewProj = _nonJitteredViewProj;

		// 完成したデータから、フラスタム平面を求める
		m_cbGPUCamera.ExtractFrustumPlanes(_nonJitteredViewProj);

		// フレームカウントを進める
		m_totlaFrameCount++;
	}
	void GraphicsEngine::ProcessInitQueue(D3D12::Device* a_pDevice, D3D12::GraphicsCommandList* a_pCmdList)
	{
		// ワールドと必須リソースの存在チェック
		auto* _pCurrentWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pCurrentWorld) return;

		if (!_pCurrentWorld->HasResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>()) return;
		if (!_pCurrentWorld->HasResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>()) return;

		auto& _initRequestVec = _pCurrentWorld->GetResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();
		if (_initRequestVec.empty()) return;

		auto& _dynamicPool = _pCurrentWorld->GetResource<Pool::ItemPool<Raytracing::DynamicRaytracingData>>();

		// モデルのリソースからBLASと頂点バッファをコピー
		for (auto& _initReq : _initRequestVec)
		{
			// ターゲットとなるインスタンスデータと、ソースとなるモデルデータの取得
			auto* _pData = _dynamicPool.Ref(_initReq.dynamicInstanceHandle);
			auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_initReq.modelHandle);
			if (!_pData || !_pModel) continue;

			// モデル内の各メッシュごとに動的BLASを構築
			for (auto& _meshHandle : _pModel->GetMeshHandles())
			{
				// メッシュの有効性チェック
				auto* _pMesh = Resource::ResourceManager::Instance().Get(_meshHandle);
				if (!_pMesh || !_pMesh->HasRtData()) continue;

				// メッシュデータの追加と参照の取得
				_pData->meshDataVec.emplace_back();
				auto& _targetMeshData = _pData->meshDataVec.back();

				// インスタンス専用のアニメーション用頂点バッファ領域をメガバッファから割り当て
				UINT _vertexCount = _pMesh->GetRtData().vertexHandle.count;
				//_targetMeshData.animatedVertexHandle = m_animatedVertexBuffer.Allocate(_vertexCount);
				_targetMeshData.animatedVertexHandle = m_upMeshBufferAllocator->AllocateAnimatedVertex(_vertexCount);

				// サブメッシュ（マテリアル単位）ごとのジオメトリ情報を構築
				std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> _descVec = {};
				_descVec.reserve(_pMesh->GetMetaData().subsets.size());

				// レイトレーシング用データ作成
				for (auto& _subset : _pMesh->GetMetaData().subsets)
				{
					// ジオメトリ記述作成
					D3D12_RAYTRACING_GEOMETRY_DESC _desc = {};
					_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
					// 頂点バッファ
					_desc.Triangles.VertexBuffer.StartAddress =
						//m_animatedVertexBuffer.GetGPUVirtualAddress() +
						m_upMeshBufferAllocator->GetAnimatedVertexBuffer().GetGPUVirtualAddress() +
						(_targetMeshData.animatedVertexHandle.startIndex * sizeof(Resource::MeshVertexFloat));
					_desc.Triangles.VertexBuffer.StrideInBytes = sizeof(Resource::MeshVertexFloat);
					_desc.Triangles.VertexCount = _vertexCount;
					_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

					// インデックスバッファ
					_desc.Triangles.IndexBuffer =
						//m_meshIndexBuffer.GetGPUVirtualAddress() +
						m_upMeshBufferAllocator->GetIndexBuffer().GetGPUVirtualAddress() +
						sizeof(uint32_t) * (_subset.faceStart * 3 + _pMesh->GetRtData().indexHandle.startIndex);
					_desc.Triangles.IndexCount = _subset.faceCount * 3;
					_desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

					_descVec.push_back(_desc);
				}
				_pData->meshDataVec.back().instanceBLAS.CreateDynamic(
					a_pDevice,
					a_pCmdList,
					_descVec
				);
				_pData->meshDataVec.back().meshHandle = _meshHandle;
			}



		}

		// 処理が終われば命令を解放
		_initRequestVec.clear();
	}
	int GraphicsEngine::GetSRVIndexFromTextureHandle(const Handle<Resource::Texture>& a_texHandle)
	{
		auto* _pTex = Resource::ResourceManager::Instance().Get(a_texHandle);
		ENGINE_ERRLOG(_pTex,"テクスチャが見つかりません");

		return static_cast<int>(_pTex->GetSRV().GetIndex());
	}

	// シェーディングモデルはもう引かない。
	// 以前はここで解決できないと描画コマンドごと捨てていたので、
	// シェーディングモデルを持たないマテリアルは何も描かれなかった
	bool GraphicsEngine::FetchDrawResources(
		const Resource::ModelDrawCommand& a_cmd,
		const Resource::Mesh*& a_pOutMesh,
		const Resource::Material*& a_pOutMaterial)
	{
		auto& _resManager = Resource::ResourceManager::Instance();

		// メッシュ
		a_pOutMesh = _resManager.Get(a_cmd.meshHandle);
		if (!a_pOutMesh) return false;

		// マテリアル
		a_pOutMaterial = _resManager.Get(a_cmd.materialHandle);
		if (!a_pOutMaterial) return false;

		return true;
	}

	MeshMaterial GraphicsEngine::BuildMeshMaterial(
		const Resource::Material* a_pMaterial,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd)
	{
		MeshMaterial _meshMaterial = {};
		_meshMaterial.baseColor = a_pMaterial->baseColor * a_albedoScale;
		_meshMaterial.emissive = a_pMaterial->emissive * a_emissiveScale;
		_meshMaterial.emissiveAdd = a_emissiveAdd;
		_meshMaterial.metallic = a_pMaterial->metallic;
		_meshMaterial.roughness = a_pMaterial->roughness;
		_meshMaterial.albedoIndex = GetSRVIndexFromTextureHandle(a_pMaterial->baseColorTex);
		_meshMaterial.metaRoughnessIndex = GetSRVIndexFromTextureHandle(a_pMaterial->metaRoughTex);
		_meshMaterial.emissiveIndex = GetSRVIndexFromTextureHandle(a_pMaterial->emissiveTex);
		_meshMaterial.normalIndex = GetSRVIndexFromTextureHandle(a_pMaterial->normalTex);
		return _meshMaterial;
	}

	void GraphicsEngine::RegisterDrawCommandToPasses(
		const Resource::ModelDrawCommand& a_cmd,
		const Resource::Mesh* a_pMesh,
		const Resource::Material* a_pMaterial,
		const DXSM::Matrix& a_mat,
		const DXSM::Matrix& a_prevMat,
		bool a_isAnimation,
		uint32_t a_animatedVertexStart,
		const DXSM::Color& a_albedoScale,
		const DXSM::Vector3& a_emissiveScale,
		const DXSM::Vector3& a_emissiveAdd,
		PSOKey a_psoKey)
	{
		// マテリアルの透明モードで、どちらのキューへ流すかを決める。
		// Mask はアルファで抜くだけで前後関係は不透明と同じ扱いなので Opaque
		const EGeometryQueue _queue = (a_pMaterial->alphaMode == Resource::Alpha::Blend)
			? EGeometryQueue::Transparent
			: EGeometryQueue::Opaque;

		// 新パイプライン側のパスへも同じアイテムを流す。
		// パスごとにPSOもパス番号も違うので、パスの数だけ登録することになる
		for (auto* _pPipelinePass : GetPipelineGeometryPasses(_queue))
		{
			if (!_pPipelinePass) continue;

			MeshMaterial _meshMaterial = BuildMeshMaterial(a_pMaterial, a_albedoScale, a_emissiveScale, a_emissiveAdd);
			const auto& _msData = a_pMesh->GetMeshShaderData();

			MeshInstanceData _meshInstanceData = {};
			_meshInstanceData.worldMat = a_mat.Transpose();
			_meshInstanceData.prevWorldMat = a_prevMat.Transpose();
			_meshInstanceData.materialOffset = SetMeshMaterialData(_meshMaterial);
			_meshInstanceData.meshletOffset = _msData.meshletHandle.startIndex + _msData.subsetMeshlets[a_cmd.subIdx].meshletOffset;
			_meshInstanceData.vertexOffset = a_pMesh->GetRtData().vertexHandle.startIndex;
			_meshInstanceData.uviOffset = _msData.uniqueVertexIndicesHandle.startIndex;
			_meshInstanceData.primitiveOffset = _msData.primitiveIndicesHandle.startIndex;
			_meshInstanceData.cullStart = _msData.cullDataHandle.startIndex + _msData.subsetMeshlets[a_cmd.subIdx].cullOffset;
			_meshInstanceData.meshletCount = _msData.subsetMeshlets[a_cmd.subIdx].meshletCount;
			_meshInstanceData.animatedVertexStart = a_animatedVertexStart;
			_meshInstanceData.isAnimated = a_isAnimation ? 1 : 0;

			PSOKey _pipelineKey = a_psoKey;
			_pipelineKey.permutationFlags |= (uint32_t)Engine::Graphics::EShaderPermutationFlags::MeshShader;
			_pipelineKey.psHandle = _pPipelinePass->GetDefaultPSHandle();

			auto _psoHandle = _pPipelinePass->RefPipelineBuilder().Request(_pipelineKey, m_pPipelineStateManager);

			// ソートキーのPSO番号は8bitしかない。
			// 収まらない番号を入れると、描くときにまったく別のPSOを引いてしまうので積まない。
			// (無効ハンドルは 0xFFFF なのでここで弾かれる)
			if (_psoHandle.GetIndex() > 0xFF)
			{
				ENGINE_WARNING(
					"[GraphicsEngine] PSO番号がソートキーに収まりません(%u) : %s",
					_psoHandle.GetIndex(), _pPipelinePass->GetName().c_str());
				continue;
			}

			Engine::Graphics::LightWeightDrawItem _item = {};
			_item.meshHandle = a_cmd.meshHandle;
			_item.materialHandle = a_cmd.materialHandle;
			_item.sortKey.bits.meshID = a_cmd.meshHandle.GetIndex();
			_item.sortKey.bits.materialID = a_cmd.materialHandle.GetIndex();
			_item.isAnimation = a_isAnimation;
			_item.subIndex = a_cmd.subIdx;
			_item.meshInstanceIndex = SetInstanceData(_meshInstanceData);
			_item.subsetMeshletCount = _msData.subsetMeshlets[a_cmd.subIdx].meshletCount;
			_item.sortKey.bits.psoID = static_cast<uint8_t>(_psoHandle.GetIndex());
			_item.sortKey.bits.passIndex = _pPipelinePass->GetPassIndex();

			AddItem(_item);
		}

	}

	void GraphicsEngine::PushUIData(
		uint32_t a_texIndex,
		const Math::Vector2& a_pixelPos,
		const Math::Vector2& a_pixelSize,
		const Math::Color& a_color,
		float a_rotationDeg,
		float a_layer,
		const Math::Vector2& a_uvOffset,
		const Math::Vector2& a_pivot,
		const Math::Vector2& a_uvScale,
		float a_curveK,
		float a_curveOffsetX
	)
	{
		// スクリーン解像度(px)
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);
		if (_w <= 0.0f || _h <= 0.0f) return;

		// 回転(度→ラジアン)。回転はピクセル空間(等方)で行い、そのあとNDCへ変換する。
		// こうしないと、NDC空間(x,yで縮尺が違う)で回転させたときに斜めで画像が歪む。
		const float _rad = DirectX::XMConvertToRadians(a_rotationDeg);
		const float _cos = std::cos(_rad);
		const float _sin = std::sin(_rad);

		// 半サイズ(px)と、ピボット(正規化[0,1])からクアッド中心までのオフセット(px)。
		// (0.5 - pivot) * size がクアッド中心のピボットからのずれ。
		const DXSM::Vector2 _halfPx = { a_pixelSize.x * 0.5f, a_pixelSize.y * 0.5f };
		const DXSM::Vector2 _pivotOffPx = {
			(0.5f - a_pivot.x) * a_pixelSize.x,
			(0.5f - a_pivot.y) * a_pixelSize.y
		};

		// クアッド頂点 q∈[-1,1] に対し、ピクセル空間での最終座標は
		//   finalPx = centerPx + q.x*axisXpx + q.y*axisYpx
		// 回転はピボットを中心に行うので、centerPx = ピボット位置 + R*ピボットオフセット。
		//
		// ベースクアッドのUVは q.x=+1 がテクスチャ右、q.y=+1 がテクスチャ上(v=0)。
		// よって未回転時、ローカル+Xは画面右(+pixelX)、ローカル+Yは画面上(-pixelY)を向く。
		// この基底(+X=右, +Y=上)を回転行列 R(θ) で回す。
		//   axisXpx = R*( halfX,      0) = ( halfX*cos, halfX*sin)
		//   axisYpx = R*(     0, -halfY) = ( halfY*sin,-halfY*cos)
		const DXSM::Vector2 _centerPx = {
			a_pixelPos.x + (_pivotOffPx.x * _cos - _pivotOffPx.y * _sin),
			a_pixelPos.y + (_pivotOffPx.x * _sin + _pivotOffPx.y * _cos)
		};
		const DXSM::Vector2 _axisXpx = { _halfPx.x * _cos,  _halfPx.x * _sin };
		const DXSM::Vector2 _axisYpx = { _halfPx.y * _sin, -_halfPx.y * _cos };

		// ピクセル(左上原点/Y下向き) → NDC(中心原点/Y上向き)。
		// 点は原点シフトあり、方向ベクトルはスケールのみ(Yは符号反転)。
		UIData _data = {};
		_data.pos   = { _centerPx.x / _w * 2.0f - 1.0f, 1.0f - _centerPx.y / _h * 2.0f };
		_data.axisX = { _axisXpx.x * 2.0f / _w, -_axisXpx.y * 2.0f / _h };
		_data.axisY = { _axisYpx.x * 2.0f / _w, -_axisYpx.y * 2.0f / _h };
		_data.uvOffset = a_uvOffset;
		_data.uvScale = a_uvScale;
		_data.color = Math::DX::ToVector4(a_color);
		_data.layer = a_layer;
		_data.texIndex = a_texIndex;
		// 湾曲。
		// 反りは「弧の中心からの横ずれ(px)」で決まるので、シェーダーが px へ戻せるように
		// このクアッドの実寸(半分の大きさ)も一緒に送る。
		// NDCの基底(axisX/axisY)からは画面解像度なしにpxを復元できないため
		_data.curveK = a_curveK;
		_data.curveOffsetX = a_curveOffsetX;
		_data.curveHalfWidth = _halfPx.x;
		_data.curveInvHalfHeight = (_halfPx.y > 0.0f) ? (1.0f / _halfPx.y) : 0.0f;

		m_uiDrawItemVec.push_back(_data);
	}
}