#include "GraphicEngine.h"

// D3D関係
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"

// リソース関係
#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

// グラフィックス関係
#include "RenderContext/RenderContext.h"
#include "RenderContext/ShapeDraw/ShapeDraw.h"
#include "RenderGraph/RenderGraph.h"


namespace Engine::Graphics
{
	GraphicsEngine::GraphicsEngine()
	{}
	GraphicsEngine::~GraphicsEngine()
	{}

	void GraphicsEngine::Init(const GraphicsEngineDesc& a_desc)
	{
		// マネージャー構築
		CreateManager();
		
		// ルートシグネチャの定義
		RootSigDefinition();

		// 形状描画クラス構築
		m_upShapeRender = std::make_unique<ShapeRenderer>();

		// レンダーコンテキストの作成
		m_upRenderContext = std::make_unique<RenderContext>();
		m_upRenderContext->Init(
			m_upShaderManager.get(),
			m_upRootSignatureManager.get(),
			m_upGrahicsPSOManager.get(),
			m_upShapeRender.get()
		);

		// レンダーグラフの構築
		m_upRenderGraph = std::make_unique<RenderGraph>();
		m_upRenderGraph->Init(
			m_upRenderContext.get(),
			m_upShaderManager.get(),
			m_upRootSignatureManager.get(),
			m_upGrahicsPSOManager.get()
		);
	}

	void GraphicsEngine::Release()
	{}

	void GraphicsEngine::ExcuteDrawCmd()
	{
		m_upRenderContext->Excute(m_upRenderGraph.get());
		
		m_upRenderContext->ClearCmd();
	}

	const Graphics::RenderContext* GraphicsEngine::GetRenderContext() const
	{
		return m_upRenderContext.get();
	}
	Graphics::RenderContext* GraphicsEngine::RefRenderContext()
	{
		return m_upRenderContext.get();
	}
	void GraphicsEngine::CreateManager()
	{
		// デバイス取得
		auto* _pDevice = D3D12Wrapper::Instance().GetDevice();

		// シェーダーマネージャー
		m_upShaderManager = std::make_unique<Resource::ShaderManager>();

		// グラフィックスパイプラインステートマネージャー構築
		m_upGrahicsPSOManager = std::make_unique<D3D12::GraphicsPSOManager>();
		m_upGrahicsPSOManager->Init(_pDevice);

		// ルートシグネチャマネージャー
		m_upRootSignatureManager = std::make_unique<RootSignatureManager>();
		m_upRootSignatureManager->Init(10);
	}
	void GraphicsEngine::RootSigDefinition()
	{
		if (!m_upRootSignatureManager) return;

		// ルートシグネチャの定義
		m_upRootSignatureManager->CreateRootSig(
			"BaseRootSig",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::ObjectCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MaterialCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
				{RootParameterType::DescriptorTable,
				{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
				RootSigSemantic::MaterialSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"ForwardLithingPass",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::ObjectCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MaterialCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::AmbientCB,true},
				{RootParameterType::DescriptorTable,
				{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
				RootSigSemantic::MaterialSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"QuadRendering",
			{
				{RootParameterType::DescriptorTable,{RangeType::SRV},
				RootSigSemantic::PostScreenSRV,false},
				{RootParameterType::DescriptorTable,{RangeType::SRV},
				RootSigSemantic::PostScreenSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"DeferredLighting",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::AmbientCB,true},
				{RootParameterType::DescriptorTable,
				{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
				RootSigSemantic::PostScreenSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"2DRootSig",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::UICB,true},
				{RootParameterType::DescriptorTable,{RangeType::SRV},RootSigSemantic::MaterialSRV,false}
			}
		);
		m_upRootSignatureManager->CreateRootSig(
			"DebugLine",
			{
				{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
				{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
				//{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
			}
		);
	}
}