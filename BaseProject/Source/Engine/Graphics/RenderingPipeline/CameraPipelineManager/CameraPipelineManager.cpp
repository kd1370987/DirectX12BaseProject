#include "CameraPipelineManager.h"

#include "../../GraphicsEngine.h"
#include "../../Device/RenderDevice/RenderDevice.h"
#include "../../Device/BackBuffer/BackBuffer.h"
#include "../../Frame/RenderContext/RenderContext.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../Core/Pass/Pass.h"
#include "../RenderingPipelineAsset/RenderingPipelineAsset.h"
#include "../RenderGraph/RenderGraph.h"
#include "../RenderingPipelineMetaRegistry.h"
#include "../GraphicsPipeline/GraphicsPipeline.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics
{
	void CameraPipelineManager::Init(GraphicsEngine* a_pGraphicsEngine)
	{
		m_pGraphicsEngine = a_pGraphicsEngine;
	}

	void CameraPipelineManager::Release()
	{
		// カメラごとのパイプラインが抱えているGPUリソースを手放す。
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
		m_pipelineOpaquePassVec.clear();
		m_pipelineTransparentPassVec.clear();
	}

	//==========================================================================================
	// フレームの頭
	//
	// 設計図が変わったカメラの実行インスタンスを組み直し、モデルを受け取るパスの一覧を作り直す。
	// 描画アイテムを1つも積んでいない今のうちに済ませることで、
	// 配り直したパス番号とアイテムのパス番号が食い違わないようにする
	//==========================================================================================
	void CameraPipelineManager::BeginFrame()
	{
		Rebuild(false);
		RefreshGeometryPassCache();
	}

	// フレームの終わり : 今フレーム積まれなかったカメラを捨てる
	void CameraPipelineManager::EndFrame()
	{
		Prune();
	}

	// RenderGraph / Texture が完全型として見えるここで生成・破棄を定義する
	CameraPipelineManager::CameraPipelineData::CameraPipelineData() = default;
	CameraPipelineManager::CameraPipelineData::~CameraPipelineData() = default;

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
	void CameraPipelineManager::AssignPassIndices()
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
	void CameraPipelineManager::RefreshGeometryPassCache()
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

	const std::vector<Pipeline::Pass*>& CameraPipelineManager::GetGeometryPasses(EGeometryQueue a_queue) const
	{
		static const std::vector<Pipeline::Pass*> _empty = {};

		switch (a_queue)
		{
		case EGeometryQueue::Opaque:		return m_pipelineOpaquePassVec;
		case EGeometryQueue::Transparent:	return m_pipelineTransparentPassVec;
		default:							return _empty;
		}
	}

	void CameraPipelineManager::SubmitCamera(const CameraSubmitDesc& a_desc)
	{
		// 描画構成を持たないカメラは描かない
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

		// 行列はこのカメラ専用の定数バッファ用の置き場。
		// 今はどのパスもここを読まず、共有のカメラ(SetCameraMat / GetCameraData)を読んでいる。
		// カメラごとの定数バッファへ移すまでは、ビューと射影を控えておくだけ
		_pCamera->cpuData.viewMat = a_desc.worldMat.Invert();
		_pCamera->cpuData.projMat = a_desc.projMat;

		// 0 のままなら画面の描画解像度に追従する
		const UINT _width = (a_desc.viewportWidth != 0) ? a_desc.viewportWidth : m_pGraphicsEngine->GetRenderWidth();
		const UINT _height = (a_desc.viewportHeight != 0) ? a_desc.viewportHeight : m_pGraphicsEngine->GetRenderHeight();

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
	void CameraPipelineManager::Rebuild(bool a_isNewOnly)
	{
		auto* _pDevice = m_pGraphicsEngine->RefRenderDevice()->RefDevice();
		auto& _resourceManager = (*m_pGraphicsEngine->RefResourceManager());

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
					m_pGraphicsEngine->RefRenderDevice()->WaitForFrame();
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
					_texDesc.optClearValue = Math::Color(0.f, 0.f, 0.f, 1.f);
					_pCamera->upFinalTex->Create(m_pGraphicsEngine->RefDescriptorHeapManager(), _texDesc);
				}

				// ---- 実行インスタンスを設計図から作る ----
				if (!_pCamera->upPipeline)
				{
					_pCamera->upPipeline = std::make_unique<Pipeline::GraphicsPipeline>();
				}

				//--------------------------------------------------------------
				// 組めなかったカメラは何も描かない(画面ならクリア色のまま)。
				//
				// 黙っていると「何も映らない」ようにしか見えないので、
				// 版が変わるたびに1回だけ理由を知らせる
				// (個々の理由は RenderGraph::Compile が並べて出す)
				//--------------------------------------------------------------
				auto _reportFail = [&]()
					{
						if (_pCamera->reportedFailVersion == _version) return;
						_pCamera->reportedFailVersion = _version;

						ENGINE_WARNING(
							"[GraphicsEngine] パイプラインを組めませんでした。このカメラは描画されません : %s",
							_pAsset->GetName().c_str());
					};

				if (!_pCamera->upPipeline->BuildFrom(*_pAsset, *m_pGraphicsEngine->RefPassMetaRegistry())) { _reportFail(); continue; }

				_pCamera->upPipeline->SetViewportSize(_pCamera->builtWidth, _pCamera->builtHeight);

				// このカメラの最終出力を、グラフの外から差し込む。
				// パスはこの名前で出力スロットを宣言すれば画面ぶんへ描ける
				_pCamera->upPipeline->ImportResource(
					kCameraOutputName,
					_pCamera->upFinalTex.get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET);

				if (!_pCamera->upPipeline->Compile(m_pGraphicsEngine, _pDevice)) { _reportFail(); continue; }

				_pCamera->builtStructureVersion = _version;
				_pCamera->builtParamVersion = _pAsset->GetParamVersion();

				// パス番号はカメラをまたいで一意でないといけないので、
				// 全部組み終わってからまとめて配る(印は上で立てている)
			}
		}

		// ---- パス番号を配り直してから回す ----
		if (_isAnyRebuilt)
		{
			AssignPassIndices();

			// 組み直しで古いパスは消えている。
			// 一覧が消えたパスを指したままにならないよう作り直す
			RefreshGeometryPassCache();
		}
	}

	// 積まれたカメラの実行インスタンスを回す。
	// 組み直しは RebuildCameraPipelines がフレームの頭で済ませてある
	void CameraPipelineManager::Execute(RenderContext* a_pRenderContext)
	{
		// 今フレームに初めて現れたカメラだけ、ここで組んでおく。
		// 待つと1フレーム何も映らないので、シーン切り替えのたびに画面が飛ぶ
		Rebuild(true);

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

		auto* _pRenderContext = a_pRenderContext;

		for (CameraPipelineData* _pCamera : m_sortedCameras)
		{
			if (!_pCamera->upPipeline) continue;
			_pCamera->upPipeline->Render(m_pGraphicsEngine, _pRenderContext);
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
	void CameraPipelineManager::Prune()
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
	bool CameraPipelineManager::IsPipelinePresentActive() const
	{
		if (!m_pMainCamera) return false;
		if (!m_pMainCamera->upPipeline || !m_pMainCamera->upPipeline->IsCompiled()) return false;

		return m_pMainCamera->upFinalTex != nullptr;
	}

	// 画面へ出す絵。パイプライン経路が生きていなければ nullptr
	const Resource::Texture* CameraPipelineManager::GetPresentTexture() const
	{
		if (!IsPipelinePresentActive()) return nullptr;
		return m_pMainCamera->upFinalTex.get();
	}

	// メインカメラのパイプラインが描いた絵をバックバッファへ写す。
	// バックバッファと最終出力はどちらも R8G8B8A8_UNORM・同じ大きさなのでそのままコピーできる
	void CameraPipelineManager::PresentTo(D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!a_pCmdList) return;
		if (!IsPipelinePresentActive()) return;

		Resource::Texture* _pFinalTex = m_pMainCamera->upFinalTex.get();
		if (!_pFinalTex) return;

		auto* _pBackBuffer = m_pGraphicsEngine->RefBackBuffer();
		if (!_pBackBuffer) return;
		Resource::Texture& _backBuffer = _pBackBuffer->RefBackBuffer();
		if (!_backBuffer.GetResource()) return;

		// バックバッファはこの時点で RENDER_TARGET。コピー先へ落とす。
		// ステートはテクスチャ自身が覚えているので、生のバリアではなく Barrier() を通す
		_backBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		// 最終出力側はグラフが入口のステートへ戻してある
		_pFinalTex->Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

		a_pCmdList->CopyResource(_backBuffer.GetResource(), _pFinalTex->GetResource());

		// この後の描画(エディターのImGuiなど)が続くので元へ戻す
		_backBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// 最終出力はこの後 ImGui が読むので、読める状態のまま置いておく
		_pFinalTex->Barrier(a_pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// 設計図のパスと同じGUIDを持つ、実行インスタンス側のパスを返す。
	//
	// メインカメラを先に見るのは、同じ設計図を複数のカメラが使っているときに
	// 「画面に出ている絵」と、ノードに出る中身を揃えるため
	Pipeline::Pass* CameraPipelineManager::FindPipelinePass(const Engine::GUID& a_passGUID) const
	{
		if (!a_passGUID.IsValid()) return nullptr;

		auto _findIn = [&a_passGUID](const CameraPipelineData* a_pCamera) -> Pipeline::Pass*
			{
				if (!a_pCamera || !a_pCamera->upPipeline) return nullptr;
				if (!a_pCamera->upPipeline->IsCompiled()) return nullptr;

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

	std::vector<CameraPipelineManager::PipelineGraphView> CameraPipelineManager::CollectPipelineGraphs() const
	{
		std::vector<PipelineGraphView> _result = {};
		_result.reserve(m_cameras.size());

		auto& _resourceManager = (*m_pGraphicsEngine->RefResourceManager());

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

	const Resource::Texture* CameraPipelineManager::GetCameraFinalTexture(const ECS::World* a_pWorld, uint32_t a_entity) const
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
}
