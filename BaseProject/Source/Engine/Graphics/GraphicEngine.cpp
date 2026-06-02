#include "GraphicEngine.h"

#include "../MainEngine.h"

// D3D関係
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../D3D12/PipelineStateManager/PipelineStateManager.h"

// グラフィックス関係
#include "RenderContext/RenderContext.h"
#include "RenderContext/ShapeDraw/ShapeDraw.h"
#include "RenderGraph/RenderGraph.h"
#include "../Animation/AnimationMatrixManager/AnimationMatrixManager.h"
#include "../Resource/Manager/ResourceManager/ResourceManager.h"


namespace Engine::Graphics
{
	GraphicsEngine::GraphicsEngine()
	{}
	GraphicsEngine::~GraphicsEngine()
	{}

	void GraphicsEngine::Init(const GraphicsEngineDesc& a_desc)
	{

		m_pPipelineStateManager = a_desc.pPipelineStateManager;

		// 形状描画クラス構築
		m_upShapeRender = std::make_unique<ShapeRenderer>();

		// レンダーコンテキストの作成
		for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
		{
			auto _upCtx = std::make_unique<RenderContext>();

			RenderContextDesc _desc = {};
			_desc.pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
			_desc.pShapeRender = m_upShapeRender.get();

			_desc.cbAllocatorMemSize = 32 * 1024 * 1024;
			_desc.boneElementNum = 10000;

			_upCtx->Init(_desc);
			m_upRenderContextVec.push_back(std::move(_upCtx));
		}

		// レンダーグラフの構築
		m_upRenderGraph = std::make_unique<RenderGraph>();
		m_upRenderGraph->Init(m_pPipelineStateManager);

		// アニメーション用メモリ領域作成
		auto* _pDev = D3D12::D3D12Wrapper::Instance().GetDevice();
		auto* _CmdList = D3D12::D3D12Wrapper::Instance().GetCmdList();
		Animation::AnimationMatrixManager::Instance().Init(_pDev, *_CmdList, 10000);

		// 定数バッファ初期化
		m_cbAmbient = {};
		m_cbAmbient.ammbientColorScale = { 0,0,0 };
		m_cbAmbient.dlDir = { 0.5f,-1.0f,0.5f };
		m_cbAmbient.dlColor = { 4.0f,4.0f,4.0f };
	}

	void GraphicsEngine::Release()
	{}

	void GraphicsEngine::BegineFrame()
	{
		m_currentFrameIndex = D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex();

		FrameDesc _desc;
		_desc.pCmdList = D3D12::D3D12Wrapper::Instance().GetCommandList();
		_desc.pCmdListClass = D3D12::D3D12Wrapper::Instance().GetCmdList();
		m_upRenderContextVec[m_currentFrameIndex]->Begine(_desc);

	}
	void GraphicsEngine::Excute()
	{
		// GPU用カメラデータを作成
		CreateGPUCameraData();

		// バッファの更新
		m_upRenderContextVec[m_currentFrameIndex]->UpdateBuffer(m_instanceDataVec, m_subSetDataVec);

		// 描画アイテムをソート
		std::sort(
			m_lightWeightDrawItemVec.begin(), m_lightWeightDrawItemVec.end(),
			[](const LightWeightDrawItem& a, const LightWeightDrawItem& b)
			{
				return a.sortKey.value < b.sortKey.value;
			}
		);

		// レンダーパス実行
		m_upRenderGraph->Excute(this,m_upRenderContextVec[m_currentFrameIndex].get());

	}
	void GraphicsEngine::EndFrame()
	{
		Engine::Editor::MainEditor::Instance().AddLog("DrawItemSize : %d\n",static_cast<int>(m_lightWeightDrawItemVec.size()));

		// 描画命令をクリアしてメモリ領域を確保しておく
		m_lightWeightDrawItemVec.clear();
		m_lightWeightDrawItemVec.reserve(10000);

		// オブジェクトデータの消去
		m_instanceDataVec.clear();
		m_instanceDataVec.reserve(10000);

		// サブセット情報の消去
		m_subSetDataVec.clear();
		m_subSetDataVec.reserve(10000);

		// カメラの前フレームの更新

	}

	const Graphics::RenderContext* GraphicsEngine::GetRenderContext() const
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();
	}
	Graphics::RenderContext* GraphicsEngine::RefRenderContext()
	{
		return m_upRenderContextVec[m_currentFrameIndex].get();
		
	}
	RenderGraph* GraphicsEngine::RefRenderGraph()
	{
		return m_upRenderGraph.get();
	}
	void GraphicsEngine::SetCameraMat(const DXSM::Matrix& a_worldMat)
	{
		// 座標を代入
		m_cbCamera.pos = { a_worldMat._41,a_worldMat._42,a_worldMat._43 ,1};

		// ビュー行列・逆ビュー行列をセット
		m_cbCamera.viewMat = a_worldMat.Invert();
		m_cbCamera.viewInvMat = a_worldMat;
	}
	void GraphicsEngine::SetProjMat(const DXSM::Matrix & a_projMat)
	{
		m_cbCamera.projMat = a_projMat;
		m_cbCamera.projInvMat = a_projMat.Invert();
	}
	const CameraData& GraphicsEngine::GetCameraData() const
	{
		return m_cbGPUCamera;
	}
	const CameraData& GraphicsEngine::GetGPUCameraData() const
	{
		return m_cbGPUCamera;
	}
	void GraphicsEngine::SetAmbientData(const AmbientData& a_data)
	{
		m_cbAmbient = a_data;
	}
	const AmbientData& GraphicsEngine::GetAmbientData() const
	{
		return m_cbAmbient;
	}
	UINT GraphicsEngine::SetInstanceData(const InstanceData& a_instanceData)
	{
		UINT _index = static_cast<UINT>(m_instanceDataVec.size());
		m_instanceDataVec.push_back(a_instanceData);
		return _index;
	}
	UINT GraphicsEngine::SetSubSetData(const SubSetData& a_subsetData)
	{
		UINT _index = static_cast<UINT>(m_subSetDataVec.size());
		m_subSetDataVec.push_back(a_subsetData);
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
	void GraphicsEngine::CreateGPUCameraData()
	{
		// リセット
		m_cbGPUCamera = {};

		// スクリーン座標を取得
		const auto& _config = Engine::MainEngine::Instance().GetEngineConfig();

		// ジッターオフセット計算
		float _jitterX = 0.0f;
		float _jitterY = 0.0f;
		if (false)
		{
			// ハルトンシーケンスのテーブル（ピクセル中心地からのオフセット値 -0.5f ～ 0.5f）
			static const float _sHaltonX[16] = { 0.0f, -0.25f, 0.25f, -0.375f, 0.125f, -0.125f, 0.375f, -0.4375f, 0.0625f, -0.1875f, 0.3125f, -0.3125f, 0.1875f, -0.0625f, 0.4375f, -0.46875f };
			static const float _sHaltonY[16] = { 0.0f, 0.166667f, -0.166667f, 0.5f, -0.5f, 0.222222f, -0.222222f, 0.888889f, -0.888889f, 0.388889f, -0.388889f, 0.055556f, -0.055556f, 0.722222f, -0.722222f };

			uint32_t _sampleIndex = m_totlaFrameCount % 16;

			// プロジェクション空間（NDC）のサイズに変換 : NDCは幅が２(-1～1)だから2倍
			_jitterX = (_sHaltonX[_sampleIndex] / (float)_config.GetRuntimeConfig().windowWidth) * 2.0f;
			_jitterY = (_sHaltonY[_sampleIndex] / (float)_config.GetRuntimeConfig().windowHeight) * 2.0f;
		}

		// カメラの行列を一時的に取得
		DXSM::Matrix _viewMat = m_cbCamera.viewMat;
		DXSM::Matrix _projMat = m_cbCamera.projMat;
		DXSM::Matrix _invViewMat = m_cbCamera.viewInvMat;
		DXSM::Matrix _invProjMat = m_cbCamera.projInvMat;
		DXSM::Matrix _viewProj = _viewMat * _projMat;
		DXSM::Matrix _invViewProj = _viewProj.Invert();

		// モーションベクター用のジッターなしViewProjを計算
		DXSM::Matrix _nonJitteredViewProj = _viewMat * _projMat;

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
		m_cbGPUCamera.viewMat			= _viewMat.Transpose();
		//m_cbGPUCamera.projMat			= _projMat.Transpose();
		m_cbGPUCamera.projMat			= _jitteredProjMat.Transpose();
		m_cbGPUCamera.viewInvMat		= _invViewMat.Transpose();
		//m_cbGPUCamera.projInvMat		= _invProjMat.Transpose();
		m_cbGPUCamera.projInvMat		= _invJitteredProj.Transpose();
		//m_cbGPUCamera.invViewProjMat	= _invViewProj.Transpose();
		m_cbGPUCamera.invViewProjMat	= _invJitteredViewProj.Transpose();

		// モーションベクターの計算にはジッターなしを使う
		m_cbGPUCamera.nonJitteredProj = _projMat.Transpose();
		m_cbGPUCamera.nonJitteredViewProj = _nonJitteredViewProj.Transpose();

		// 過去フレームのジッターなし行列の処理
		m_cbGPUCamera.prevView = m_prevViewMat.Transpose();
		m_cbGPUCamera.prevProj = m_prevProjMat.Transpose();
		m_cbGPUCamera.prevViewProj = m_prevNonJitteredViewProj.Transpose();

		// 次のフレームのためにジッターなしデータを保存
		m_prevViewMat = _viewMat;
		m_prevProjMat = _projMat;
		m_prevNonJitteredViewProj = _nonJitteredViewProj;

		// フレームカウントを進める
		m_totlaFrameCount++;
	}
}