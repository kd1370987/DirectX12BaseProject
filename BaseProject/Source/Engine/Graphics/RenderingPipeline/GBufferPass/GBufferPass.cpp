#include "GBufferPass.h"

#include "../../GraphicEngine.h"
#include "../../RenderContext/RenderContext.h"
#include "../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics::Pipeline
{
	void GBufferPass::SetupSlots()
	{
		// 不透明のモデルを受け取る
		m_geometryQueue = EGeometryQueue::Opaque;

		// ZPre が前に居るなら、その深度をそのまま受け取って描く。
		// 繋がっていなければ自分でクリアして書く(OnLinksResolved で切り替える)
		DeclareInput("PreDepth", EAccessType::Depth_Write, EPassSlotType::Texture, false);

		//----------------------------------------------------------------------------------
		// 出力
		//
		// Albedo は sRGB。ライティングは線形の値で計算するので、
		// ここを UNORM にするとガンマのかかった値をそのまま線形として扱うことになり、
		// 暗く濁った絵になる
		//----------------------------------------------------------------------------------
		auto _declareRT = [this](const char* a_pin, const char* a_name, DXGI_FORMAT a_format) -> Slot&
			{
				Slot& _slot = DeclareOutput(a_pin, a_name, a_format, EAccessType::RTV);
				_slot.loadOp = ELoadOp::Clear;
				return _slot;
			};

		_declareRT("Albedo",	"GBufferAlbedo",	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		_declareRT("Normal",	"GBufferNormal",	DXGI_FORMAT_R16G16_FLOAT);
		_declareRT("Material",	"GBufferMaterial",	DXGI_FORMAT_R8G8B8A8_UNORM);
		_declareRT("Emissive",	"GBufferEmissive",	DXGI_FORMAT_R11G11B10_FLOAT);
		_declareRT("Velocity",	"GBufferVelocity",	DXGI_FORMAT_R16G16_FLOAT);

		// 深度 : あとで SRV としても読めるように TYPELESS で確保する
		Slot& _depth = DeclareOutput(
			"Depth", "SceneDepth", DXGI_FORMAT_R32_TYPELESS, EAccessType::Depth_Write);
		_depth.loadOp = ELoadOp::Clear;
	}

	// 前段(ZPre)から深度をもらっているならクリアしない。
	// クリアしてしまうと前段が書いた深度が消えて、ZPre を置いた意味が無くなる
	void GBufferPass::OnLinksResolved()
	{
		const Slot* _pPreDepth = FindInputSlot(MakeSlotID("PreDepth"));
		const bool _isPreDepth = (_pPreDepth && _pPreDepth->IsConnected());

		Slot* _pDepth = FindOutputSlot(MakeSlotID("Depth"));
		if (!_pDepth) return;

		_pDepth->loadOp = _isPreDepth ? ELoadOp::Load : ELoadOp::Clear;
	}

	void GBufferPass::Compile(const PassContext& a_context)
	{
		if (!a_context.pGraphicsEngine) return;

		auto* _pPSOManager = a_context.pGraphicsEngine->RefPipelineStateManager();
		if (!_pPSOManager) return;

		auto& _assetDB = Resource::AssetDatabase::Instance();
		auto& _resManager = Resource::ResourceManager::Instance();

		// ---- メッシュシェーダー / 増幅シェーダー ----
		// スキニングの有無で切り替わるが、今は同じものを両方へ登録している(旧版と同じ)
		const auto _guidMS = _assetDB.GetGUIDFromFilePath("Asset/Shader/Source/Geometry/MeshShader/UberMS.cso");
		const auto _msHandle = _resManager.LoadImmediate<Resource::Shader>(_guidMS);
		m_pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Static, _msHandle);
		m_pipelineBuilder.RegisterMeshShader(EShaderPermutationFlags::Skinned, _msHandle);

		const auto _guidAS = _assetDB.GetGUIDFromFilePath("Asset/Shader/Source/Geometry/MeshShader/TestAS.cso");
		const auto _asHandle = _resManager.LoadImmediate<Resource::Shader>(_guidAS);
		m_pipelineBuilder.RegisterAmplificationShader(EShaderPermutationFlags::Static, _asHandle);
		m_pipelineBuilder.RegisterAmplificationShader(EShaderPermutationFlags::Skinned, _asHandle);

		// ---- ピクセルシェーダー ----
		// 旧版はシェーディングモデルが持っていたぶん。PSはパスの都合なのでこちらで持つ
		const auto _guidPS = _assetDB.GetGUIDFromFilePath("Asset/Shader/Source/Geometry/GBuffer/MeshGBufferPS.cso");
		m_defaultPSHandle = _resManager.LoadImmediate<Resource::Shader>(_guidPS);

		// ---- ルートシグネチャ ----
		m_rootSigHandle = _pPSOManager->Request("Asset/Shader/Source/Geometry/MeshShader/UberMS.cso");

		//----------------------------------------------------------------------------------
		// 深度テスト
		//
		// ZPre が前に居るなら、深度はもう埋まっている。
		// 書かずに EQUAL で「ZPreと完全に一致するピクセルだけ」描く(旧版と同じ)。
		// 単体で置かれたときだけ自分で書くので LESS_EQUAL にする
		//----------------------------------------------------------------------------------
		const Slot* _pPreDepth = FindInputSlot(MakeSlotID("PreDepth"));
		const bool _isPreDepth = (_pPreDepth && _pPreDepth->IsConnected());

		if (_isPreDepth)	m_pipelineBuilder.SetDepthConfig(true, false, D3D12_COMPARISON_FUNC_EQUAL);
		else				m_pipelineBuilder.SetDepthConfig(true, true, D3D12_COMPARISON_FUNC_LESS_EQUAL);
	}

	void GBufferPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		if (!_pCtx) return;

		// レンダーターゲットの切り替えとクリアはグラフが済ませてある
		_pCtx->BindCopyHeapAndSumplerBindLess();
		_pCtx->SetGraphicsRootSignature(m_rootSigHandle);

		_pCtx->BindCamera();
		_pCtx->BindMeshInstance();
		_pCtx->BindMeshlet();

		// 自分のパス番号で積まれた描画アイテムだけを引く
		_pCtx->DrawQueueDispathMesh(GetPassIndex());
	}

	EPassEditResult GBufferPass::EditUpdate()
	{
		ImGui::TextDisabled("不透明モデルをGBufferへ描きます");
		ImGui::Text("PassIndex : %d", static_cast<int>(GetPassIndex()));
		return EPassEditResult::None;
	}

	void GBufferPass::EditNode()
	{}

	void GBufferPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
