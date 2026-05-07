#include "TestPass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"


namespace Engine::Graphics
{
	void Engine::Graphics::TestPass::Excute(RenderContext* a_pCtx)
	{
		Begine(a_pCtx);


		//for (auto& _pso : m_psoHandle)
		//{
		//	// PSOのセット
		//	a_pCtx->SetGraphicPSO(_pso.first);

		//	// 指定タイプの命令キューを取得
		//	auto& _draws = a_pCtx->GetItemVec(_pso.second);
		//	if (_draws.size() <= 0) continue;

		//	for (auto& _item : _draws)
		//	{
		//		// オブジェクト情報セット
		//		DXSM::Vector2 _uv = { 0,0 };
		//		DXSM::Vector2 _tile = { 1,1 };
		//		a_pCtx->BindObuje(_uv, _tile);

		//		// メッシュのバインド
		//		a_pCtx->BindMesh(_item.pMesh, _item.worldMat);

		//		a_pCtx->BindIndex({ 
		//			(float)Resource::TextureManager::Instance().GetTexture(_item.pMaterial->baseColorTex).GetSRV().idx,
		//			(float)Resource::TextureManager::Instance().GetTexture(_item.pMaterial->metaRoughTex).GetSRV().idx,
		//			(float)Resource::TextureManager::Instance().GetTexture(_item.pMaterial->emissiveTex).GetSRV().idx,
		//			(float)Resource::TextureManager::Instance().GetTexture(_item.pMaterial->normalTex).GetSRV().idx
		//		});

		//		// 描画
		//		a_pCtx->Draw(_item.pMesh, _item.subIdx);
		//	}
		//}
		End(a_pCtx);
	}
	void TestPass::CreatePass()
	{
		auto& _sPso = AddPSODesc(ERenderType::Static, RenderQueueType::Opaque);
		_sPso.SetName("Test");

		SetInputLayout(ERenderType::Static, D3D12::Input::StaticLayout);
		SetVS(ERenderType::Static, "Asset/Shader/Source/TestShader/TestVS.cso");

		SetPS("Asset/Shader/Source/TestShader/TestPS.cso");
		SetRootSig("TestSig");
		

		AddRead("Depth", AccessType::Depth_Write, LoadOp::Load, StoreOp::Store);
		AddWrite("Test", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}